// ============================================================
// bptreefile.h
// ------------------------------------------------------------

#include <bits/stdc++.h>
#include "common.h"
using namespace std;
using ptr = int64_t; // tipo para armazenar endereços de disco (offsets)

// Define o tamanho fixo do cabeçalho da árvore (3 ponteiros)
constexpr ptr BPTREE_HEADER_SIZE = static_cast<ptr>(3 * sizeof(ptr));

// ============================================================
// Estrutura do nó da Árvore B+
// ============================================================
// Cada nó da árvore pode ser um nó interno ou uma folha.
// Os nós internos guardam chaves e ponteiros para outros nós,
// enquanto as folhas guardam as chaves e os ponteiros para
// os registros no arquivo de dados.
// ------------------------------------------------------------
template<typename key, int M>
struct no {
    ptr ponteiros[M+2];  // ponteiros para filhos (ou registros se for folha)
    key keys[M+1];       // vetor de chaves armazenadas no nó
    int qtdKeys;         // número de chaves usadas
    bool folha;          // indica se o nó é uma folha

    // Construtor: inicializa o nó vazio
    no() {
        qtdKeys = 0;
        folha = false;
        // todos os ponteiros começam com -1 (indica vazio)
        for (int i = 0; i < M+2; ++i) ponteiros[i] = static_cast<ptr>(-1);
        // todas as chaves são inicializadas com valor padrão
        for (int i = 0; i < M+1; ++i) keys[i] = key();
    }
};

// ============================================================
// Cache de páginas (LRU)
// ============================================================
// O cache serve para armazenar temporariamente páginas (nós)
// em memória, evitando que o programa precise acessar o disco
// toda hora. Ele usa uma política LRU (Least Recently Used):
// o nó usado há mais tempo é o primeiro a ser removido.
// ------------------------------------------------------------
template<typename key, int M, size_t CACHE_SIZE = 64>
class PageCache {
public:
    struct CacheEntry {
        ptr endereco;      // posição no arquivo
        no<key, M> pagina; // conteúdo da página
        bool dirty;        // indica se a página foi modificada
    };

private:
    // Lista duplamente encadeada que controla a ordem de uso:
    // o início (front) é o mais recentemente usado (MRU)
    // o final (back) é o menos usado (LRU)
    list<pair<ptr, CacheEntry>> lruList;

    // Tabela para acesso rápido (endereço → posição na lista)
    unordered_map<ptr, typename list<pair<ptr, CacheEntry>>::iterator> table;

    // Arquivo associado à árvore
    fstream* file;

public:
    // Construtor
    PageCache(fstream* f = nullptr) : file(f) {}

    // Permite alterar o arquivo depois que o cache já existe
    void setFile(fstream* f) { file = f; }

    // Verifica se um endereço já está no cache
    bool contains(ptr address) {
        return table.find(address) != table.end();
    }

    // Retorna ponteiro para a página em cache (ou nullptr se não estiver)
    no<key, M>* get(ptr address) {
        auto itmap = table.find(address);
        if (itmap == table.end()) return nullptr;
        auto it = itmap->second;
        // move o item para o início da lista (foi recém-usado)
        lruList.splice(lruList.begin(), lruList, it);
        return &it->second.pagina;
    }

    // Insere ou atualiza uma entrada no cache
    void put(ptr address, const no<key, M>& node, bool dirty = false) {
        auto itmap = table.find(address);
        if (itmap != table.end()) {
            // se já existe, atualiza e move pra frente
            auto it = itmap->second;
            it->second.pagina = node;
            it->second.dirty = it->second.dirty || dirty;
            lruList.splice(lruList.begin(), lruList, it);
            return;
        }

        // Se o cache estiver cheio, remove o último (menos usado)
        if (lruList.size() >= CACHE_SIZE) {
            auto last = lruList.back();
            // Se a página for "suja", escreve no disco antes de remover
            if (last.second.dirty) {
                if (file) {
                    file->clear();
                    file->seekp(last.first);
                    file->write(reinterpret_cast<const char*>(&last.second.pagina), sizeof(no<key, M>));
                }
            }
            // Remove da tabela e da lista
            table.erase(last.first);
            lruList.pop_back();
        }

        // Cria nova entrada e adiciona no início da lista
        CacheEntry e{address, node, dirty};
        lruList.push_front({address, e});
        table[address] = lruList.begin();
    }

    // Marca uma página como "suja" (precisa ser salva)
    void markDirty(ptr address) {
        auto itmap = table.find(address);
        if (itmap != table.end()) itmap->second->second.dirty = true;
    }

    // Escreve todas as páginas sujas no disco
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

    // Força gravação de tudo e limpa o cache
    void clearAndFlush() {
        flush();
        lruList.clear();
        table.clear();
    }
};

// ============================================================
// Funções auxiliares de leitura e escrita de nós
// ============================================================
// Essas funções manipulam diretamente o arquivo de índice
// em disco. Elas são usadas em várias partes da árvore.
// ------------------------------------------------------------

// Lê um nó do arquivo ou do cache
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

// Reescreve (atualiza) um nó no arquivo
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

// Escreve um novo nó no final do arquivo e retorna seu endereço
template<typename key, int M>
ptr escrever(fstream* file, no<key, M> *novo, PageCache<key,M>* cache = nullptr){
    file->clear();
    file->seekp(0, ios::end);
    ptr pt = file->tellp();
    file->write(reinterpret_cast<char*>(novo), sizeof(no<key, M>));
    // não precisa flush imediato para otimizar
    if (cache) cache->put(pt, *novo, false);
    return pt;
}

// ============================================================
// Traits para manipulação de chaves
// ============================================================
// Esses "traits" permitem que a árvore funcione com diferentes
// tipos de chaves (int, string fixa, etc.), sem precisar duplicar código.
// ------------------------------------------------------------

// Versão padrão: tipos simples como int, double, etc.
template<typename Key>
struct KeyOps {
    static bool less(const Key& a, const Key& b) { return a < b; }
    static bool equal(const Key& a, const Key& b) { return a == b; }
    static void copy(Key& dest, const Key& src) { dest = src; }

    static void print(const Key& k) {
        cout << k;
    }
};

// Especialização para array<char, N> (strings de tamanho fixo)
template<size_t N>
struct KeyOps<array<char, N>> {
    static bool less(const array<char, N>& a, const array<char, N>& b){
        return strncmp(a.data(), b.data(), N) < 0;
    }
    static bool equal(const array<char, N>& a, const array<char, N>& b){
        return strncmp(a.data(), b.data(), N) == 0;
    }
    static void copy(array<char, N>& dest, const array<char, N>& src) {
        memset(dest.data(), 0, N);
        strncpy(dest.data(), src.data(), N-1);
        //dest[N-1] = '\0';
    }

    static void print(const array<char, N>& k) {
        cout << k.data();
    }
};

// Função auxiliar para usar comparações genéricas (STL)
template<typename key>
auto comp() {
    return [](const key& a, const key& b) { return KeyOps<key>::less(a,b); };
}

// ============================================================
// Inserção simples de chave e ponteiro em um nó
// ============================================================
// Essa função é usada quando ainda há espaço no nó.
// Ela insere a chave no local certo e move as demais.
// ------------------------------------------------------------
template <typename key, int M>
void inserirCP(fstream *file, no<key, M> *noAtual, key chave, ptr ponteiro, ptr pAtual, PageCache<key, M> *cache)
{
    key *chaves = noAtual->keys;
    ptr *ponteiros = noAtual->ponteiros;

    int i = noAtual->qtdKeys - 1;

    if (noAtual->folha){
        // encontra posição onde a nova chave deve entrar
        while (i >= 0 && KeyOps<key>::less(chave, chaves[i]))
            i--;
        int pos = i + 1;

        // move as chaves e ponteiros para abrir espaço
        for (int j = noAtual->qtdKeys - 1; j >= pos; --j){
            KeyOps<key>::copy(chaves[j + 1], chaves[j]);
            ponteiros[j + 1] = ponteiros[j];
        }

        // insere nova chave e ponteiro
        KeyOps<key>::copy(chaves[pos], chave);
        ponteiros[pos] = ponteiro;
    }
    else{
        // se for nó interno, a lógica é parecida, mas ajusta ponteiros
        while (i >= 0 && KeyOps<key>::less(chave, chaves[i])) i--;
        int pos = i + 1;

        for (int j = noAtual->qtdKeys - 1; j >= pos; --j){
            KeyOps<key>::copy(chaves[j + 1], chaves[j]);
        }

        for (int j = noAtual->qtdKeys; j >= pos + 1; --j){
            ponteiros[j + 1] = ponteiros[j];
        }

        KeyOps<key>::copy(chaves[pos], chave);
        ponteiros[pos + 1] = ponteiro;
    }

    noAtual->qtdKeys++;
    // grava o nó atualizado
    reescrever(file, pAtual, noAtual, cache);
}

// ============================================================
// Cisão (divisão de nó cheio)
// ============================================================
// Quando um nó atinge o número máximo de chaves, ele precisa
// ser dividido em dois. A chave do meio sobe para o nível acima.
// ------------------------------------------------------------
template<typename key, int M>
pair<key, ptr> cisao(fstream *file, ptr pAtual, no<key, M> *noAtual, no<key, M> *noNovo, key chave, ptr ponteiro, PageCache<key,M>* cache) {

    // Cria vetores temporários para reorganizar as chaves
    key tempKeys[M+1];
    ptr tempPtrs[M+2];

    int tam = noAtual->qtdKeys;
    for (int i=0;i<tam;i++) {
        KeyOps<key>::copy(tempKeys[i], noAtual->keys[i]);
        tempPtrs[i] = noAtual->ponteiros[i];
    }
    tempPtrs[tam] = noAtual->ponteiros[tam]; // último ponteiro

    // Encontra posição onde a nova chave será inserida
    int i = tam-1;
    while (i>=0 && KeyOps<key>::less(chave, tempKeys[i])) {
        KeyOps<key>::copy(tempKeys[i+1], tempKeys[i]);
        tempPtrs[i+2] = tempPtrs[i+1];
        i--;
    }
    int pos = i+1;
    KeyOps<key>::copy(tempKeys[pos], chave);
    tempPtrs[pos+1] = ponteiro;
    tam++;

    // calcula posição central (meio)
    int meio = tam / 2;

    // inicializa o novo nó
    noNovo->folha = noAtual->folha;
    noNovo->qtdKeys = 0;
    for (int j=0;j<M+2;j++) noNovo->ponteiros[j] = static_cast<ptr>(-1);

    // =======================================================
    // Caso 1: nó folha
    // =======================================================
    if (noAtual->folha) {
        // copia metade direita para o novo nó
        for (int j=meio;j<tam;j++) {
            KeyOps<key>::copy(noNovo->keys[j-meio], tempKeys[j]);
            noNovo->ponteiros[j-meio] = tempPtrs[j];
        }
        noNovo->qtdKeys = tam - meio;
        noAtual->qtdKeys = meio;

        // liga a nova folha com a próxima
        noNovo->ponteiros[M] = noAtual->ponteiros[M];
        ptr pNovo = escrever(file, noNovo, cache);
        noAtual->ponteiros[M] = pNovo;

        // atualiza o nó antigo com a metade esquerda
        for (int j=0;j<meio;j++) {
            KeyOps<key>::copy(noAtual->keys[j], tempKeys[j]);
            noAtual->ponteiros[j] = tempPtrs[j];
        }
        reescrever(file, pAtual, noAtual, cache);

        // retorna a menor chave da nova folha (para subir)
        key promovida;
        KeyOps<key>::copy(promovida, noNovo->keys[0]);
        return {promovida, pNovo};
    } 
    // =======================================================
    // Caso 2: nó interno
    // =======================================================
    else {
        // chave central será promovida
        key promovida;
        KeyOps<key>::copy(promovida, tempKeys[meio]);

        // novo nó recebe as chaves à direita da promovida
        int idx = 0;
        for (int j=meio+1;j<tam;j++) {
            KeyOps<key>::copy(noNovo->keys[idx], tempKeys[j]);
            idx++;
        }
        // e também os ponteiros correspondentes
        for (int j=meio+1;j<=tam;j++) {
            noNovo->ponteiros[j-(meio+1)] = tempPtrs[j];
        }
        noNovo->qtdKeys = tam - (meio + 1);

        // nó original fica com a parte esquerda
        noAtual->qtdKeys = meio;
        for (int j=0;j<meio;j++) {
            KeyOps<key>::copy(noAtual->keys[j], tempKeys[j]);
            noAtual->ponteiros[j] = tempPtrs[j];
        }
        noAtual->ponteiros[meio] = tempPtrs[meio];

        // grava novo nó no disco e atualiza o antigo
        ptr pNovo = escrever(file, noNovo, cache);
        reescrever(file, pAtual, noAtual, cache);

        // retorna a chave promovida e o ponteiro do novo nó
        return {promovida, pNovo};
    }
}

// ============================================================
// Função que desce na árvore até encontrar a folha correta
// ============================================================
// Essa função percorre a árvore B+ a partir da raiz até a
// folha onde a chave deve estar. Ela também conta quantos
// blocos (nós) foram lidos no processo.
// ------------------------------------------------------------
template<typename key, int M>
pair<ptr, int> acharFolha(fstream *file, ptr pRaiz, key& alvo, stack<ptr> *pilha = NULL, PageCache<key,M>* cache = nullptr){
    ptr pAtual = pRaiz;
    no<key, M> noAtual;
    int qtd_blocos = 0;

    while(true){ // percorre até chegar em uma folha
        carregar(file, pAtual, &noAtual, cache);
        qtd_blocos++;

        if(noAtual.folha) break; // se chegou numa folha, para

        // se a pilha existir, guarda o caminho percorrido
        if(pilha) pilha->push(pAtual);
        
        // busca binária pra encontrar o ponteiro certo
        int i = upper_bound(noAtual.keys, noAtual.keys + noAtual.qtdKeys, alvo, comp<key>()) - noAtual.keys;
        
        pAtual = noAtual.ponteiros[i];
    }
    // retorna o ponteiro da folha e a quantidade de blocos lidos
    return {pAtual, qtd_blocos};
}

// ============================================================
// Estrutura principal da Árvore B+
// ============================================================
// Essa estrutura guarda os metadados da árvore e implementa
// as operações principais: iniciar, carregar, buscar e inserir.
// ------------------------------------------------------------
template<typename key, int M>
struct bp{
    ptr raiz;           // endereço da raiz no arquivo
    ptr primeiraFolha;  // endereço da primeira folha
    ptr qtd_nos = 0;    // contador de nós existentes
    fstream* file;      // arquivo de índice
    unique_ptr<PageCache<key,M>> cache; // cache de páginas

    // ========================================================
    // Inicializa uma nova árvore B+ vazia no arquivo
    // ========================================================
    void iniciar(fstream* f){
        this->file = f;
        this->cache = make_unique<PageCache<key,M>>(f);
        this->cache->setFile(f);

        // verifica se o arquivo está vazio
        f->clear();
        f->seekg(0, ios::end);
        auto fileSize = f->tellg();
        if (fileSize == 0) {
            // escreve um cabeçalho inicial vazio
            vector<char> header(BPTREE_HEADER_SIZE, 0);
            f->seekp(0, ios::beg);
            f->write(header.data(), header.size());
            f->flush();
        }

        // cria o primeiro nó (raiz)
        no<key, M> raiz; 
        raiz.folha = true; 
        raiz.qtdKeys = 0;
        for (int i = 0; i < M+2; ++i) raiz.ponteiros[i] = static_cast<ptr>(-1);

        // grava o nó raiz no final do arquivo
        this->raiz = escrever(f, &raiz, cache.get());
        this->qtd_nos += 1;
        this->primeiraFolha = this->raiz;

        // salva os metadados no início do arquivo
        salvarMetadados();
    }

    // ========================================================
    // Salva informações básicas da árvore no início do arquivo
    // ========================================================
    void salvarMetadados() {
        if (!file) return;
        file->seekp(0);
        file->write(reinterpret_cast<const char*>(&raiz), sizeof(ptr));
        file->write(reinterpret_cast<const char*>(&primeiraFolha), sizeof(ptr));
        file->write(reinterpret_cast<const char*>(&qtd_nos), sizeof(ptr));
        file->flush();
    }

    // ========================================================
    // Carrega uma árvore B+ existente do disco
    // ========================================================
    void carregarArvore(fstream* f){
        this->file = f;
        this->cache = make_unique<PageCache<key,M>>(f);
        this->cache->setFile(f);

        f->clear();
        f->seekg(0, ios::end);
        auto fileSize = f->tellg();
        // se o arquivo não tem cabeçalho, considera árvore vazia
        if (fileSize < static_cast<streamoff>(BPTREE_HEADER_SIZE)) {
            this->raiz = static_cast<ptr>(-1);
            this->primeiraFolha = static_cast<ptr>(-1);
            this->qtd_nos = 0;
            return;
        }

        // lê os metadados
        f->seekg(0, ios::beg);
        f->read(reinterpret_cast<char*>(&raiz), sizeof(ptr));
        f->read(reinterpret_cast<char*>(&primeiraFolha), sizeof(ptr));
        f->read(reinterpret_cast<char*>(&qtd_nos), sizeof(ptr));
    }

    // ========================================================
    // Escreve todas as páginas sujas no disco
    // ========================================================
    void flushCache() {
        if (cache) cache->flush();
    }

    // ========================================================
    // Retorna a quantidade de blocos (nós) armazenados
    // ========================================================
    ptr contarBlocos() { 
        if (!file) return static_cast<ptr>(0); 
        flushCache(); 
        file->clear(); 
        file->seekg(0, ios::end); 
        streamoff fileSize = file->tellg(); 
        if (fileSize <= static_cast<streamoff>(BPTREE_HEADER_SIZE))
            return static_cast<ptr>(0);
         
        streamoff dataBytes = fileSize - static_cast<streamoff>(BPTREE_HEADER_SIZE); 
        ptr nodeSize = static_cast<ptr>(sizeof(no<key, M>)); 
        ptr total = static_cast<ptr>(dataBytes / static_cast<streamoff>(nodeSize)); 
        return total; 
    }

    // ========================================================
    // Busca uma chave dentro da árvore e retorna o registro
    // ========================================================
    // Essa função usa o índice (B+) para localizar o endereço
    // do registro dentro do arquivo de dados.
    // --------------------------------------------------------
    pair<bool, long> buscar(key alvo, Registro& encontrado, fstream* db){
        int qtd_blocos = 0;
        no<key, M> folha;

        // desce na árvore até a folha onde a chave pode estar
        pair<ptr,int> res = acharFolha(file, this->raiz, alvo, nullptr, cache.get());
        
        ptr pFolha = res.first; 
        carregar(file, pFolha, &folha, cache.get());
        qtd_blocos = res.second + 1;        

        // procura a chave dentro da folha
        int i = lower_bound(folha.keys, folha.keys + folha.qtdKeys, alvo, comp<key>()) - folha.keys;
        Registro temp{};

        pair<bool, long> res2;
        res2.second = qtd_blocos;

        // se encontrou a chave, lê o registro no arquivo de dados
        if (i < folha.qtdKeys && KeyOps<key>::equal(folha.keys[i], alvo)){
            (*db).seekg(folha.ponteiros[i], ios::beg);
            (*db).read(reinterpret_cast<char*>(&temp), sizeof(Registro));
            encontrado = temp;
            res2.first = true;
        }
        else res2.first = false;

        res2.second = qtd_blocos;
        return res2;
    }

    // ========================================================
    // Insere uma nova chave e ponteiro na árvore B+
    // ========================================================
    // Essa é a parte mais importante da estrutura.
    // Ela insere a nova chave e, se necessário, divide nós.
    // --------------------------------------------------------
    void inserir(key chave, ptr ponteiro){
        stack<ptr> pilha; // guarda o caminho percorrido até a folha
        pair<ptr,int> res = acharFolha(file, this->raiz, chave, &pilha, cache.get());
        ptr pAtual = res.first;
        no<key, M> noAtual;

        carregar(file, pAtual, &noAtual, cache.get());

        // Caso 1: ainda há espaço na folha
        if (noAtual.qtdKeys < M) {
            inserirCP(file, &noAtual, chave, ponteiro, pAtual, cache.get());
            return;
        }

        // Caso 2: a folha está cheia → precisa dividir
        no<key, M> novoNo; 
        novoNo.folha = true; 
        novoNo.qtdKeys = 0;
        for (int i = 0; i < M+2; ++i) novoNo.ponteiros[i] = static_cast<ptr>(-1);

        // divide a folha e obtém a chave promovida
        pair<key, ptr> promovida = cisao(file, pAtual, &noAtual, &novoNo, chave, ponteiro, cache.get());
        this->qtd_nos += 1;
        
        // atualiza variáveis com o que subiu
        KeyOps<key>::copy(chave, promovida.first);
        ponteiro = promovida.second;

        // sobe pela pilha, tentando inserir a chave nos níveis de cima
        while(!pilha.empty()){
            pAtual = pilha.top(); 
            pilha.pop();
            carregar(file, pAtual, &noAtual, cache.get());

            // se ainda houver espaço no nó pai, insere e termina
            if(noAtual.qtdKeys < M){
                inserirCP(file, &noAtual, chave, ponteiro, pAtual, cache.get());
                return;
            }

            // se o nó pai também estiver cheio, divide novamente
            no<key, M> novoNo2;
            novoNo2.folha = false; 
            novoNo2.qtdKeys = 0;
            for (int i = 0; i < M+2; ++i) novoNo2.ponteiros[i] = static_cast<ptr>(-1);

            promovida = cisao(file, pAtual, &noAtual, &novoNo2, chave, ponteiro, cache.get());
            this->qtd_nos += 1;

            KeyOps<key>::copy(chave, promovida.first);
            ponteiro = promovida.second;

            // se subiu até a raiz, precisa criar uma nova
            if(pilha.empty()) break; 
        }

        // Cria uma nova raiz (a árvore cresceu um nível)
        no<key, M> novaRaiz;
        novaRaiz.folha = false;
        for (int i = 0; i < M+2; ++i) novaRaiz.ponteiros[i] = static_cast<ptr>(-1);
        KeyOps<key>::copy(novaRaiz.keys[0], chave);
        novaRaiz.ponteiros[0] = pAtual;
        novaRaiz.ponteiros[1] = ponteiro;
        novaRaiz.qtdKeys = 1;
        this->raiz = escrever(file, &novaRaiz, cache.get());
        this->qtd_nos += 1;
    }

    // ========================================================
    // Destrutor: garante que o cache seja gravado no disco
    // ========================================================
    ~bp() {
        if (cache) cache->flush();
    }
};

