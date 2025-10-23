#include <bits/stdc++.h>
using namespace std;
using ptr = streampos; // usamos offsets do arquivo como "ponteiros"

// =================== PARÂMETROS AJUSTÁVEIS =====================
#ifndef M // Ordem da B+Tree (máximo de chaves por nó)
#define M 128 // valor menor para reduzir I/O por reescrita de nós muito grandes
#endif

#ifndef PAGE_CACHE_MAX
#define PAGE_CACHE_MAX 32 // páginas de índice mantidas em memória (LRU simples)
#endif

// ============= Traits para diferentes tipos de chave =============
// Trait padrão para tipos triviais (int, double, etc.)
template<typename Key>
struct KeyOps {
    static bool less(const Key& a, const Key& b) { return a < b; }
    static bool equal(const Key& a, const Key& b) { return a == b; }
    static void copy(Key& dest, const Key& src) { dest = src; }
};

// Especialização para arrays de char (strings de tamanho fixo)
template<size_t N>
struct KeyOps<char[N]> {
    static bool less(const char a[N], const char b[N]) { return strcmp(a, b) < 0; }
    static bool equal(const char a[N], const char b[N]) { return strcmp(a, b) == 0; }
    static void copy(char dest[N], const char src[N]) { strcpy(dest, src); }
};

// Comparador para algoritmos padrão (upper_bound/lower_bound)
template<typename key>
auto comp() {
    return [](const key& a, const key& b) { return KeyOps<key>::less(a,b); };
}

// ====================== Estruturas do Índice =====================

template<typename key>
struct no {
    // +2 para acomodar inserções temporárias em caso de cisão
    ptr ponteiros[M+2];
    key keys[M+1];
    int qtdKeys = 0;
    bool folha = true;
};

// --- I/O de nós (SEM flush forçado em cada operação) ---

template<typename key>
static inline void carregarNode(fstream* file, ptr pag, no<key>* out){
    file->clear();
    file->seekg(pag);
    file->read(reinterpret_cast<char*>(out), sizeof(no<key>));
}

template<typename key>
static inline void gravarNode(fstream* file, ptr pag, const no<key>* in){
    file->clear();
    file->seekp(pag);
    file->write(reinterpret_cast<const char*>(in), sizeof(no<key>));
    // sem flush aqui: deixamos para o buffer manager/flush final
}

template<typename key>
static inline ptr appendNode(fstream* file, const no<key>* in){
    file->clear();
    file->seekp(0, ios::end);
    ptr pos = file->tellp();
    file->write(reinterpret_cast<const char*>(in), sizeof(no<key>));
    return pos;
}

// ====================== Buffer Manager (LRU) =====================

template<typename Key>
class PageCache {
    struct CachedNode {
        no<Key> node{};
        ptr pos{-1};
        bool dirty{false};
        list<ptr>::iterator itLRU; // posição na lista LRU
    };

    fstream* file;
    size_t maxPages;

    unordered_map<long long, CachedNode> cache; // chave = (long long)pos
    list<ptr> lru; // frente = mais recente; fundo = mais antigo

    // métricas
    long* blocosLidos;
    long* blocosEscritos;

    void touchLRU(ptr p) {
        auto k = (long long)p;
        auto& c = cache[k];
        if (c.itLRU != lru.end()) lru.erase(c.itLRU);
        lru.push_front(p);
        c.itLRU = lru.begin();
    }

    void evictIfNeeded() {
        if (cache.size() < maxPages) return;
        // remove o menos recentemente usado (fundo da lista)
        ptr vict = lru.back();
        lru.pop_back();
        auto k = (long long)vict;
        auto it = cache.find(k);
        if (it != cache.end()) {
            if (it->second.dirty) {
                gravarNode(file, vict, &it->second.node);
                (*blocosEscritos)++;
            }
            cache.erase(it);
        }
    }

public:
    PageCache(fstream* f, size_t maxPages_, long* lidos, long* escritos)
        : file(f), maxPages(maxPages_), blocosLidos(lidos), blocosEscritos(escritos) {}

    no<Key>* get(ptr p) {
        auto k = (long long)p;
        auto it = cache.find(k);
        if (it != cache.end()) {
            touchLRU(p);
            return &it->second.node;
        }
        // carregar do disco
        evictIfNeeded();
        CachedNode c{};
        carregarNode<Key>(file, p, &c.node);
        (*blocosLidos)++;
        c.pos = p; c.dirty = false;
        lru.push_front(p);
        c.itLRU = lru.begin();
        cache.emplace(k, std::move(c));
        return &cache[k].node;
    }

    // cria página nova no final do arquivo e coloca no cache
    pair<ptr, no<Key>*> createNew(bool folha) {
        evictIfNeeded();
        no<Key> novo{}; novo.folha = folha; novo.qtdKeys = 0;
        ptr pos = appendNode<Key>(file, &novo);
        (*blocosEscritos)++; // consideramos o append como escrita de 1 bloco

        CachedNode c{}; c.node = novo; c.pos = pos; c.dirty = false;
        lru.push_front(pos); c.itLRU = lru.begin();
        cache.emplace((long long)pos, std::move(c));
        return {pos, &cache[(long long)pos].node};
    }

    void markDirty(ptr p) {
        auto it = cache.find((long long)p);
        if (it != cache.end()) it->second.dirty = true;
    }

    void flushAll() {
        for (auto& [k, c] : cache) {
            if (c.dirty) {
                gravarNode<Key>(file, c.pos, &c.node);
                (*blocosEscritos)++;
                c.dirty = false;
            }
        }
        file->flush();
    }
};

// ====================== Operações de nó =====================

template<typename key>
static inline void inserirCP(no<key>* noAtual, const key& chave, ptr ponteiro, bool folha) {
    key* chaves = noAtual->keys;
    ptr* ponteiros = noAtual->ponteiros;

    int i = noAtual->qtdKeys - 1;
    while (i >= 0 && KeyOps<key>::less(chave, chaves[i])) {
        KeyOps<key>::copy(chaves[i+1], chaves[i]);
        ponteiros[i+1] = ponteiros[i];
        i--;
    }
    i++;
    // deslocamento já feito, agora insere
    KeyOps<key>::copy(chaves[i], chave);

    if (folha) {
        // em folha, ponteiros[i] aponta para o registro correspondente à chave[i]
        // empurra o ponteiro antigo para a direita
        for (int j = noAtual->qtdKeys; j > i; --j) {
            ponteiros[j] = ponteiros[j-1];
        }
        ponteiros[i] = ponteiro;
    } else {
        // interno: ponteiros[i+1] é o filho da direita da chave recém-inserida
        for (int j = noAtual->qtdKeys + 1; j > i + 1; --j) {
            ponteiros[j] = ponteiros[j-1];
        }
        ponteiros[i+1] = ponteiro;
    }

    noAtual->qtdKeys++;
}

// Divide nó cheio em dois e promove uma chave
// Retorna {chave_promovida, posicao_novo_no}

template<typename key>
static inline pair<key, ptr> cisao(PageCache<key>& cache, ptr pAtual, no<key>* noAtual) {
    bool folha = noAtual->folha;

    // novo nó (mesmo tipo do atual)
    auto [pNovo, novoNo] = cache.createNew(folha);

    int tam = noAtual->qtdKeys;
    int meio = tam / 2;

    if (folha) {
        // copia metade superior para novoNo
        for (int i = meio; i < tam; i++) {
            KeyOps<key>::copy(novoNo->keys[i - meio], noAtual->keys[i]);
            novoNo->ponteiros[i - meio] = noAtual->ponteiros[i];
        }
        novoNo->qtdKeys = tam - meio;
        noAtual->qtdKeys = meio;

        // encadeia folhas: ponteiros[M] guarda o próximo
        novoNo->ponteiros[M] = noAtual->ponteiros[M];
        noAtual->ponteiros[M] = pNovo;

        cache.markDirty(pAtual);
        cache.markDirty(pNovo);

        key promovida; // primeira chave do novo nó sobe
        KeyOps<key>::copy(promovida, novoNo->keys[0]);
        return {promovida, pNovo};
    } else {
        // nó interno: chaves[meio] é promovida; filhos à direita vão para o novo
        key promovida;
        KeyOps<key>::copy(promovida, noAtual->keys[meio]);

        // copiar chaves > meio e filhos correspondentes
        int j = 0;
        for (int i = meio + 1; i < tam; i++, j++) {
            KeyOps<key>::copy(novoNo->keys[j], noAtual->keys[i]);
            novoNo->ponteiros[j] = noAtual->ponteiros[i];
        }
        // filho extra do lado direito
        novoNo->ponteiros[j] = noAtual->ponteiros[tam];

        novoNo->qtdKeys = tam - meio - 1;
        noAtual->qtdKeys = meio;

        cache.markDirty(pAtual);
        cache.markDirty(pNovo);
        return {promovida, pNovo};
    }
}

// Desce até a folha alvo; opcionalmente guarda o caminho (para tratar cisões)

template<typename key>
static inline pair<ptr,int> acharFolha(PageCache<key>& cache, ptr pRaiz, const key& alvo, stack<ptr>* caminho=nullptr) {
    ptr pAtual = pRaiz;
    int qtd_blocos = 0; // será atualizado via cache (blocosLidos), mas mantemos para compatibilidade

    while (true) {
        no<key>* n = cache.get(pAtual); // contagem de bloco lido feita no cache
        qtd_blocos++; // métrica local (apenas para retorno desta função)
        if (n->folha) break;
        if (caminho) caminho->push(pAtual);
        int i = upper_bound(n->keys, n->keys + n->qtdKeys, alvo, comp<key>()) - n->keys;
        pAtual = n->ponteiros[i];
    }
    return {pAtual, qtd_blocos};
}

// ====================== B+Tree =====================

template<typename key>
class BpTree {
public:
    ptr raiz{-1};
    ptr primeiraFolha{-1};
    long qtd_nos{0};

    // métricas
    long blocosLidos{0};
    long blocosEscritos{0};

    // arquivo e cache
    fstream file;
    unique_ptr<PageCache<key>> cache;

    // heurística para inserções sequenciais
    ptr lastLeaf{-1}; // última folha visitada

    bool open(const string& path) {
        file.open(path, ios::in | ios::out | ios::binary);
        if (!file.is_open()) {
            // criar novo
            file.open(path, ios::out | ios::binary);
            if (!file.is_open()) return false;
            file.close();
            file.open(path, ios::in | ios::out | ios::binary);
            if (!file.is_open()) return false;
        }
        cache = make_unique<PageCache<key>>(&file, PAGE_CACHE_MAX, &blocosLidos, &blocosEscritos);
        return true;
    }

    void iniciar() {
        auto [pRaiz, raizNode] = cache->createNew(true); // folha vazia
        raiz = pRaiz;
        primeiraFolha = pRaiz;
        qtd_nos = 1;
        lastLeaf = pRaiz;
    }

    // Busca por chave exata: retorna {ponteiro_para_registro, blocosLidosNoIndice}
    pair<ptr,int> buscar(const key& alvo) {
        auto [pFolha, blocosIdx] = acharFolha<key>(*cache, raiz, alvo);
        no<key>* folha = cache->get(pFolha);
        // busca binária na folha
        int i = lower_bound(folha->keys, folha->keys + folha->qtdKeys, alvo, comp<key>()) - folha->keys;
        if (i < folha->qtdKeys && KeyOps<key>::equal(folha->keys[i], alvo)) {
            return {folha->ponteiros[i], blocosIdx};
        }
        return {static_cast<ptr>(-1), blocosIdx};
    }

    // Inserção de uma única chave
    void inserir(key chave, ptr ponteiro) {
        stack<ptr> caminho; // nós internos do caminho até a folha

        // Heurística: tenta começar pela lastLeaf se a chave parece sequencial
        ptr pFolha;
        if (lastLeaf != -1) {
            no<key>* lf = cache->get(lastLeaf);
            if (lf->folha && lf->qtdKeys > 0 && !KeyOps<key>::less(chave, lf->keys[0])) {
                pFolha = lastLeaf; // provável mesma faixa
            } else {
                pFolha = acharFolha<key>(*cache, raiz, chave, &caminho).first;
            }
        } else {
            pFolha = acharFolha<key>(*cache, raiz, chave, &caminho).first;
        }

        no<key>* folha = cache->get(pFolha);

        // 1) há espaço na folha
        if (folha->qtdKeys < M) {
            inserirCP<key>(folha, chave, ponteiro, /*folha=*/true);
            cache->markDirty(pFolha);
            lastLeaf = pFolha;
            return;
        }

        // 2) folha cheia → cisão
        // Primeiro, insere no nó temporariamente para facilitar a divisão
        // Inserimos manualmente (com espaço extra M+1) para depois dividir
        // Aqui utilizamos uma abordagem: inserir e depois chamar cisão que redistribui
        inserirCP<key>(folha, chave, ponteiro, /*folha=*/true);
        cache->markDirty(pFolha);

        auto [prom, pNovo] = cisao<key>(*cache, pFolha, folha);
        qtd_nos++;
        lastLeaf = (KeyOps<key>::less(chave, prom) ? pFolha : pNovo);

        // sobe promovendo
        while (!caminho.empty()) {
            ptr pPai = caminho.top(); caminho.pop();
            no<key>* pai = cache->get(pPai);

            if (pai->qtdKeys < M) {
                // em nó interno, ao inserir chave promovida, o ponteiro associado é o novo nó à direita
                inserirCP<key>(pai, prom, pNovo, /*folha=*/false);
                cache->markDirty(pPai);
                return;
            }
            // precisa dividir o interno também: primeiro insere, depois divide
            inserirCP<key>(pai, prom, pNovo, /*folha=*/false);
            cache->markDirty(pPai);
            tie(prom, pNovo) = cisao<key>(*cache, pPai, pai);
            qtd_nos++;
            // se ainda há pai acima, continua o loop
        }

        // se esvaziou a pilha, promovemos acima da raiz → criar nova raiz
        auto [pNovaRaiz, novaRaiz] = cache->createNew(false);
        KeyOps<key>::copy(novaRaiz->keys[0], prom);
        novaRaiz->ponteiros[0] = raiz;
        novaRaiz->ponteiros[1] = pNovo;
        novaRaiz->qtdKeys = 1;
        cache->markDirty(pNovaRaiz);
        raiz = pNovaRaiz;
        qtd_nos++;
    }

    // Inserção em lote: aproveita cache e flush único
    void inserirLote(const vector<pair<key, ptr>>& pares) {
        // dica: se as chaves não vierem ordenadas, ordenar ajuda muito
        // (B+Tree fica quase sequencial nas folhas, reduzindo descidas)
        vector<pair<key, ptr>> v = pares;
        sort(v.begin(), v.end(), [](const auto& a, const auto& b){ return KeyOps<key>::less(a.first, b.first); });
        for (auto& kv : v) inserir(kv.first, kv.second);
        cache->flushAll();
    }

    void flush() { cache->flushAll(); }

    // utilitário para debug
    void mostrarArvore(ptr p, int nivel=0) {
        if (p == (ptr)-1) return;
        no<key>* n = cache->get(p);
        cout << string(nivel*2, ' ') << (n->folha ? "[F]" : "[I]") << " pos=" << (long long)p
             << " keys=" << n->qtdKeys << " | ";
        for (int i = 0; i < n->qtdKeys; ++i) {
            if constexpr (std::is_same<key,int>::value) cout << n->keys[i] << (i+1<n->qtdKeys?',':' ');
            else cout << "*" << (i+1<n->qtdKeys?',':' ');
        }
        cout << "\n";
        if (!n->folha) {
            for (int i = 0; i <= n->qtdKeys; ++i) mostrarArvore(n->ponteiros[i], nivel+1);
        }
    }
};

// ====================== Exemplo de uso (integração) =====================
// Mantemos a mesma API conceitual usada no TP2:
// - b+ primária: key = int (ID) → ponteiro para registro no arquivo de dados
// - b+ secundária: key = char[TAM_TITULO_FIXO]
// Compile este arquivo junto com seus programas (upload/seek1/seek2) e
// instancie BpTree<int> (ou BpTree<seu_tipo_de_chave>) por arquivo/índice.

/*
// Exemplo mínimo:
int main(){
    BpTree<int> idx;
    if (!idx.open("/data/db/idx_primario.bpt")) { cerr << "Erro ao abrir" << endl; return 1; }
    idx.iniciar();

    // Inserções
    vector<pair<int,ptr>> pares;
    for (int i = 1; i <= 10000; ++i) pares.push_back({i, (ptr)(i*128)});
    idx.inserirLote(pares);

    // Busca
    auto [pos, blocos] = idx.buscar(1234);
    cout << "encontrado em pos=" << (long long)pos << " (blocos indice=" << blocos << ")\n";

    // métricas globais
    cout << "Blocos lidos (índice): " << idx.blocosLidos << " | escritos: " << idx.blocosEscritos << "\n";
    return 0;
}
*/
