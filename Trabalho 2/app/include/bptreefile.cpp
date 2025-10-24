#include <bits/stdc++.h>
using namespace std;
using ptr = long long;

// ===== Nó da árvore (pode ser folha ou nó interno) =====
template<typename key, int M>
struct no{
    // Espaços mais para fazer inserções temporárias em caso de cisão
    ptr ponteiros[M+2];
    key keys[M+1];
    int qtdKeys;
    bool folha;
};

// ===== Cache de páginas (LRU) para reduzir I/O =====
// Parâmetro CACHE_SIZE ajustável; default razoável (64 páginas)
template<typename key, int M, size_t CACHE_SIZE = 64>
class PageCache {
public:
    struct CacheEntry {
        ptr endereco;
        no<key, M> pagina;
        bool dirty;
    };

private:
    // Lista dupla encadeada para LRU: front = MRU, back = LRU
    list<pair<ptr, CacheEntry>> lruList;
    unordered_map<ptr, typename list<pair<ptr, CacheEntry>>::iterator> table;
    fstream* file;

public:
    PageCache(fstream* f = nullptr) : file(f) {}

    void setFile(fstream* f) { file = f; }

    bool contains(ptr address) {
        return table.find(address) != table.end();
    }

    // Retorna ponteiro para página em cache (ou nullptr se não existir)
    no<key, M>* get(ptr address) {
        auto itmap = table.find(address);
        if (itmap == table.end()) return nullptr;
        auto it = itmap->second;
        // Move para frente (mais recentemente usada)
        lruList.splice(lruList.begin(), lruList, it);
        return &it->second.pagina;
    }

    // Insere ou atualiza entrada no cache; marca dirty conforme necessário
    void put(ptr address, const no<key, M>& node, bool dirty = false) {
        auto itmap = table.find(address);
        if (itmap != table.end()) {
            auto it = itmap->second;
            it->second.pagina = node;
            it->second.dirty = it->second.dirty || dirty;
            lruList.splice(lruList.begin(), lruList, it);
            return;
        }

        // Evict LRU se necessário
        if (lruList.size() >= CACHE_SIZE) {
            auto last = lruList.back();
            if (last.second.dirty) {
                // Escreve no disco antes de remover
                if (file) {
                    file->clear();
                    file->seekp(last.first);
                    file->write(reinterpret_cast<const char*>(&last.second.pagina), sizeof(no<key, M>));
                }
            }
            table.erase(last.first);
            lruList.pop_back();
        }

        CacheEntry e{address, node, dirty};
        lruList.push_front({address, e});
        table[address] = lruList.begin();
    }

    void markDirty(ptr address) {
        auto itmap = table.find(address);
        if (itmap != table.end()) itmap->second->second.dirty = true;
    }

    // Forçar escrita de todas as páginas sujas no disco
    void flush() {
        if (!file) return;
        for (auto &entryPair : lruList) {
            ptr address = entryPair.first;
            CacheEntry &entry = entryPair.second;
            if (entry.dirty) {
                file->clear();
                file->seekp(address);
                file->write(reinterpret_cast<const char*>(&entry.pagina), sizeof(no<key, M>));
                entry.dirty = false;
            }
        }
        file->flush();
    }

    // Forçar escrita e limpar cache (opcional)
    void clearAndFlush() {
        flush();
        lruList.clear();
        table.clear();
    }
};

// ===== Utilitários otimizados com suporte a cache =====

template<typename key, int M>
void carregar(fstream* file, ptr pag, no<key, M> *noo, PageCache<key,M>* cache = nullptr){
    if (cache) {
        no<key,M>* cached = cache->get(pag);
        if (cached) {
            *noo = *cached;
            return;
        }
    }
    file->clear();
    file->seekg(pag);
    file->read(reinterpret_cast<char*>(noo), sizeof(no<key, M>));
    if (cache) cache->put(pag, *noo, false);
}

template<typename key, int M>
void reescrever(fstream* file, ptr pag, no<key, M> *novo, PageCache<key,M>* cache = nullptr){
    if (cache) {
        cache->put(pag, *novo, true);
        return;
    }
    file->clear();
    file->seekp(pag);
    file->write(reinterpret_cast<char*>(novo), sizeof(no<key, M>));
    file->flush();
}

template<typename key, int M>
ptr escrever(fstream* file, no<key, M> *novo, PageCache<key,M>* cache = nullptr){
    file->clear();
    file->seekp(0, ios::end);
    ptr pt = file->tellp();
    file->write(reinterpret_cast<char*>(novo), sizeof(no<key, M>));
    // Não forçamos flush imediato para permitir agrupamento de writes
    if (cache) cache->put(pt, *novo, false);
    return pt;
}

// ===== Traits para tratar diferentes tipos de chave =====

// Trait padrão para tipos triviais (int, double, etc.)
template<typename Key>
struct KeyOps {
    static bool less(const Key& a, const Key& b) { return a < b; }
    static bool equal(const Key& a, const Key& b) { return a == b; }
    static void copy(Key& dest, const Key& src) { dest = src; }
};

// Especialização para arrays de char (strings)
template<size_t N>
struct KeyOps<char[N]> {
    static bool less(const char a[N], const char b[N]) { return strcmp(a, b) < 0; }
    static bool equal(const char a[N], const char b[N]) { return strcmp(a, b) == 0; }
    static void copy(char dest[N], const char src[N]) { strcpy(dest, src); }
};

// Comparador dos traits para busca binária
template<typename key>
auto comp() {
    return [](const key& a, const key& b) { return KeyOps<key>::less(a,b); };
}

// ===== Nó da árvore (pode ser folha ou nó interno) =====
// template<typename key, int M>
// struct no{
//     // Espaços mais para fazer inserções temporárias em caso de cisão
//     ptr ponteiros[M+2];
//     key keys[M+1];
//     int qtdKeys;
//     bool folha;
// };


// Insere par chave/ponteiro em um nó
template<typename key, int M>
void inserirCP(fstream *file, no<key, M> *noAtual, key chave, ptr ponteiro, ptr pAtual, PageCache<key,M>* cache){
    key *chaves = noAtual->keys;
    ptr *ponteiros = noAtual->ponteiros;

    int i = noAtual->qtdKeys-1;

    // Desloca as chaves e ponteiros até encontrar o local certo para a novva chave
    while(i>=0 && KeyOps<key>::less(chave, chaves[i])){
        KeyOps<key>::copy(chaves[i+1], chaves[i]);
        ponteiros[i+1] = ponteiros[i];
        i--;
    }
    i++;
    KeyOps<key>::copy(chaves[i+1], chave);

    if(noAtual->folha){
        // Se for folha, chaves e ponteiros em índices iguais andam juntos
        ponteiros[i+1] = ponteiros[i];
        ponteiros[i] = ponteiro;
    }
    else{
        // Se for nó interno, o novo ponteiro corresponde ao novo nó à direita após cisão
        ponteiros[i+1] = ponteiro;
    }
    
    noAtual->qtdKeys++;
    reescrever(file, pAtual, noAtual, cache);
}

// Divide nó cheio em dois e promove uma chave
template<typename key, int M>
pair<key,ptr> cisao(fstream *file, ptr pAtual, no<key, M> *noAtual, no<key, M> *noNovo, key chave, ptr ponteiro, PageCache<key,M>* cache) {
    inserirCP(file, noAtual, chave, ponteiro, pAtual, cache);

    int tam = noAtual->qtdKeys;
    int meio = tam / 2;

    noNovo->folha = noAtual->folha;

    // Copia metade superior para o novo nó
    for (int i = meio; i < tam; i++) {
        KeyOps<key>::copy(noNovo->keys[i - meio], noAtual->keys[i]);
        noNovo->ponteiros[i - meio] = noAtual->ponteiros[i];
    }
    noNovo->qtdKeys = tam - meio;
    noAtual->qtdKeys = meio; // mantém apenas metade inferior

    ptr pNovo;
    if (noAtual->folha) {
        // Conecta folhas
        noNovo->ponteiros[M] = noAtual->ponteiros[M];
        pNovo = escrever(file, noNovo, cache);
        noAtual->ponteiros[M] = pNovo;
        reescrever(file, pAtual, noAtual, cache);
    } else {
        noNovo->ponteiros[noNovo->qtdKeys] = noAtual->ponteiros[tam];
        pNovo = escrever(file, noNovo, cache);
        reescrever(file, pAtual, noAtual, cache);
    }

    key promovida;
    KeyOps<key>::copy(promovida, noNovo->keys[0]);
    return {promovida, pNovo};
}

// Desce na árvore buscando a folha correspondente à chave alvo
template<typename key, int M>
pair<ptr, int> acharFolha(fstream *file, ptr pRaiz, key alvo, stack<ptr> *pilha = NULL, PageCache<key,M>* cache = nullptr){
    ptr pAtual = pRaiz;
    no<key, M> noAtual;
    int qtd_blocos = 0;

    while(true){ // Desce a árvore até encontrar uma folha
        carregar(file, pAtual, &noAtual, cache);
        qtd_blocos++;

        if(noAtual.folha) break;

        if(pilha) pilha->push(pAtual); // Insere nós internos na pilha para guardar caminho
        
        // Busca binária no nó interno (encontra o primeiro estritamente maior que o alvo)
        int i = upper_bound(noAtual.keys, noAtual.keys + noAtual.qtdKeys, alvo, comp<key>()) - noAtual.keys;

        pAtual = noAtual.ponteiros[i]; // Ponteiro esquerdo da chave encontrada
    }

    // Retorna ponteiro para folha e quantidade de blocos lidos
    return {pAtual, qtd_blocos};
}

// ===== Árvore B+ =====

template<typename key, int M>
struct bp{
    ptr raiz;
    ptr primeiraFolha;
    ptr qtd_nos = 0;
    fstream* file;
    unique_ptr<PageCache<key,M>> cache;

    // Inicializa a árvore e o cache
    void iniciar(fstream* f){
        this->file = f;
        this->cache = make_unique<PageCache<key,M>>(f);
        this->cache->setFile(f);

        no<key, M> raiz; raiz.folha = true; raiz.qtdKeys = 0;
        // Inicializa ponteiros com -1 por segurança
        for (int i = 0; i < M+2; ++i) raiz.ponteiros[i] = -1;
        this->raiz = escrever(f, &raiz, cache.get());
        this->qtd_nos += 1;
        this->primeiraFolha = this->raiz;
    }

    void carregarArvore(fstream* f){
        this->file = f;
        this->cache = make_unique<PageCache<key,M>>(f);
        this->cache->setFile(f);

        f->seekg(0, ios::beg);
        ptr st = file->tellp();
        this->raiz = st;
    }

    // Força flush do cache para disco
    void flushCache() {
        if (cache) cache->flush();
    }

    pair<ptr,int> buscar(key alvo){
        int qtd_blocos = 0;
        no<key, M> folha;
        pair<ptr,int> res = acharFolha(file, this->raiz, alvo, nullptr, cache.get());
        //cout << "hey" << endl;
        ptr pFolha = res.first; 
        carregar(file, pFolha, &folha, cache.get());
        qtd_blocos = res.second + 1;

        // Busca binária na folha (encontra igual ou primeiro maior)
        int i = lower_bound(folha.keys, folha.keys + folha.qtdKeys, alvo, comp<key>())-folha.keys;

        // Retorna ponteiro e quantidade de blocos lidos
        if (i < folha.qtdKeys && KeyOps<key>::equal(folha.keys[i], alvo))
            return {folha.ponteiros[i], qtd_blocos};
        else // Se não for igual, alvo não existe
            return {-1,qtd_blocos};
    }

    void inserir(key chave, ptr ponteiro){
        stack<ptr> pilha; // Guarda caminho na árvore para casos de cisão
        pair<ptr,int> res = acharFolha(file, this->raiz, chave, &pilha, cache.get());
        ptr pAtual = res.first;
        no<key, M> noAtual;

        carregar(file, pAtual, &noAtual, cache.get());

        // Caso 1: há espaço na folha
        if (noAtual.qtdKeys < M) {
            inserirCP(file, &noAtual, chave, ponteiro, pAtual, cache.get());
            return;
        }

        // Caso 2: não há espaço na folha

        // Divide a folha em dois e promove a chave do meio
        no<key, M> novoNo; // folha
        novoNo.folha = true; novoNo.qtdKeys = 0;
        // inicializa ponteiros do novo nó
        for (int i = 0; i < M+2; ++i) novoNo.ponteiros[i] = -1;

        pair<key, ptr> promovida = cisao(file, pAtual, &noAtual, &novoNo, chave, ponteiro, cache.get());
        this->qtd_nos += 1;
        chave = promovida.first; ponteiro = promovida.second;

        // "Sobe" na árvore até encontrar espaço (tira da pilha)
        while(!pilha.empty()){
            pAtual = pilha.top(); pilha.pop();
            carregar(file, pAtual, &noAtual, cache.get());

            // Se houver espaço, insere
            if(noAtual.qtdKeys<M){
                inserirCP(file, &noAtual, chave, ponteiro, pAtual, cache.get());
                return;
            }

            // Se não, divide o nó interno em dois e promove a chave do meio
            novoNo.folha = false; novoNo.qtdKeys = 0;
            for (int i = 0; i < M+2; ++i) novoNo.ponteiros[i] = -1;

            promovida = cisao(file, pAtual, &noAtual, &novoNo, chave, ponteiro, cache.get());
            this->qtd_nos += 1;
            chave = promovida.first; ponteiro = promovida.second;

            // Se chegou na raíz, para
            if(pilha.empty()) break; 
        }

        // Cria nova raíz
        no<key, M> novaRaiz;
        novaRaiz.folha = false;
        // inicializa ponteiros
        for (int i = 0; i < M+2; ++i) novaRaiz.ponteiros[i] = -1;
        KeyOps<key>::copy(novaRaiz.keys[0], chave);
        novaRaiz.ponteiros[0] = pAtual;
        novaRaiz.ponteiros[1] = ponteiro;
        novaRaiz.qtdKeys = 1;
        this->raiz = escrever(file, &novaRaiz, cache.get());
        this->qtd_nos += 1;
    }

    void mostrarArvore(ptr noAtual, int nivel) {
        if (noAtual == -1) return;
        
        no<key, M> no;
        carregar(file, noAtual, &no, cache.get());

        cout << "Quantidade de nós: " << this->qtd_nos << "\n";
        
        // Imprime indentação conforme o nível
        for (int i=0; i<nivel; i++) cout << "  ";
        
        // Imprime o nó
        cout << "No " << noAtual << " (nivel " << nivel << ", " 
            << (no.folha ? "folha" : "interno") << "): ";
        
        for (int i=0;i<no.qtdKeys;i++) {
            cout << no.keys[i];
            if (i < no.qtdKeys - 1) cout << ", ";
        }
        cout << endl;
        
        // Se não é folha, mostra os filhos recursivamente
        if (!no.folha) {
            for (int i=0;i<=no.qtdKeys;i++) {
                mostrarArvore(no.ponteiros[i], nivel + 1);
            }
        }
    }

    // Destrutor para garantir que o cache seja flushado ao final
    ~bp() {
        if (cache) cache->flush();
    }
};

// No final de bptreefile.cpp, substitua as instanciações por:

// Instanciações para int
template struct bp<int, 64>;
template void inserirCP<int, 64>(fstream*, no<int, 64>*, int, ptr, ptr, PageCache<int, 64>*);
template pair<int, ptr> cisao<int, 64>(fstream*, ptr, no<int, 64>*, no<int, 64>*, int, ptr, PageCache<int, 64>*);
template pair<ptr, int> acharFolha<int, 64>(fstream*, ptr, int, stack<ptr>*, PageCache<int, 64>*);

// Especializações explícitas para char[300] - precisamos usar wrappers
using Char300 = char[300];

// Wrapper para char[300] que pode ser usado em std::pair
struct Char300Wrapper {
    char data[300];
    
    Char300Wrapper() { data[0] = '\0'; }
    Char300Wrapper(const char* src) { 
        strncpy(data, src, 299);
        data[299] = '\0';
    }
    
    bool operator<(const Char300Wrapper& other) const {
        return strcmp(data, other.data) < 0;
    }
    
    bool operator==(const Char300Wrapper& other) const {
        return strcmp(data, other.data) == 0;
    }
};

ostream& operator<<(std::ostream& os, const Char300Wrapper& wrapper) {
    os << wrapper.data;
    return os;
}

// Instanciações usando o wrapper
template struct bp<Char300Wrapper, 64>;
template void inserirCP<Char300Wrapper, 64>(fstream*, no<Char300Wrapper, 64>*, Char300Wrapper, ptr, ptr, PageCache<Char300Wrapper, 64>*);
template pair<Char300Wrapper, ptr> cisao<Char300Wrapper, 64>(fstream*, ptr, no<Char300Wrapper, 64>*, no<Char300Wrapper, 64>*, Char300Wrapper, ptr, PageCache<Char300Wrapper, 64>*);
template pair<ptr, int> acharFolha<Char300Wrapper, 64>(fstream*, ptr, Char300Wrapper, stack<ptr>*, PageCache<Char300Wrapper, 64>*);