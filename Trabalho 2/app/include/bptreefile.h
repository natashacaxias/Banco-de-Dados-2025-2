#include <bits/stdc++.h>
#include "common.h"
using namespace std;
using ptr = int64_t;

// ===== Nó da árvore (pode ser folha ou nó interno) =====
template<typename key, int M>
struct no{
    // Espaços mais para fazer inserções temporárias em caso de cisão
    ptr ponteiros[M+2];
    key keys[M+1];
    int qtdKeys;
    bool folha;

    // Construtor para garantir inicialização definida
    no() {
        qtdKeys = 0;
        folha = false;
        // Inicializa ponteiros com -1 (sentinela)
        for (int i = 0; i < M+2; ++i) ponteiros[i] = static_cast<ptr>(-1);
        // Inicializa chaves com zero/valor default
        for (int i = 0; i < M+1; ++i) keys[i] = key();
    }
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

// === KeyOps para tipos padrão (int, double, etc.) ===
template<typename Key>
struct KeyOps {
    static bool less(const Key& a, const Key& b) { return a < b; }
    static bool equal(const Key& a, const Key& b) { return a == b; }
    static void copy(Key& dest, const Key& src) { dest = src; }

    // NOVO: Método para imprimir chave
    static void print(const Key& k) { 
        cout << k; 
    }
};

// === Especialização para array<char, N> ===
template<size_t N>
struct KeyOps<array<char, N>> {
    static bool less(const array<char, N>& a, const array<char, N>& b){
        return strcmp(a.data(), b.data()) < 0;
    }
    static bool equal(const array<char, N>& a, const array<char, N>& b){
        return strcmp(a.data(), b.data()) == 0;
    }
    static void copy(array<char, N>& dest, const array<char, N>& src) {
        // CORREÇÃO: Zera o destino antes de copiar
        memset(dest.data(), 0, N);
        strncpy(dest.data(), src.data(), N-1);
        dest[N-1] = '\0'; // Garante terminação
    }

    static void print(const array<char, N>& k) {
        cout << k.data(); // Mais simples
    }
};

template<typename Key>
void printKey(const Key& k) {
    KeyOps<Key>::print(k);
}

// Comparador dos traits para busca binária
template<typename key>
auto comp() {
    return [](const key& a, const key& b) { return KeyOps<key>::less(a,b); };
}

// Insere par chave/ponteiro em um nó
template<typename key, int M>
void inserirCP(fstream *file, no<key, M> *noAtual, key chave, ptr ponteiro, ptr pAtual, PageCache<key,M>* cache){
    key *chaves = noAtual->keys;
    ptr *ponteiros = noAtual->ponteiros;

    int i = noAtual->qtdKeys - 1;

    if(noAtual->folha){
        // Encontra posição de inserção
        while(i >= 0 && KeyOps<key>::less(chave, chaves[i])) i--;

        int pos = i + 1;
        // Move todos os elementos à direita de pos uma casa para a direita
        if (noAtual->qtdKeys - pos > 0) {
            memmove(&chaves[pos+1], &chaves[pos], sizeof(key)*(noAtual->qtdKeys - pos));
            memmove(&ponteiros[pos+1], &ponteiros[pos], sizeof(ptr)*(noAtual->qtdKeys - pos));
        }
        // Insere
        KeyOps<key>::copy(chaves[pos], chave);
        ponteiros[pos] = ponteiro;
    }
    else {
        // Nó interno: encontra posição para nova chave
        while(i >= 0 && KeyOps<key>::less(chave, chaves[i])) i--;
        int pos = i + 1;

        // Move chaves à direita de pos
        if (noAtual->qtdKeys - pos > 0) {
            memmove(&chaves[pos+1], &chaves[pos], sizeof(key)*(noAtual->qtdKeys - pos));
        }
        // Move ponteiros: há qtdKeys+1 ponteiros; precisamos mover ponteiros[pos+1 .. qtdKeys] para pos+2 .. 
        if (noAtual->qtdKeys - pos + 1 > 0) { // number of pointers to move
            memmove(&ponteiros[pos+2], &ponteiros[pos+1], sizeof(ptr)*(noAtual->qtdKeys - pos + 1));
        }

        // Insere chave e novo ponteiro à direita dela
        KeyOps<key>::copy(chaves[pos], chave);
        ponteiros[pos+1] = ponteiro;
    }

    noAtual->qtdKeys++;
    reescrever(file, pAtual, noAtual, cache);
}

// Divide nó cheio em dois e promove uma chave
template<typename key, int M>
pair<key, ptr> cisao(fstream *file, ptr pAtual, no<key, M> *noAtual, no<key, M> *noNovo, key chave, ptr ponteiro, PageCache<key,M>* cache) {
    // -----------------------------
    // 1️⃣ Inserir temporariamente chave no nó atual (somente para decidir cisão)
    // -----------------------------
    key tempKeys[M+1];
    ptr tempPtrs[M+2];

    int tam = noAtual->qtdKeys;
    for (int i=0;i<tam;i++) {
        KeyOps<key>::copy(tempKeys[i], noAtual->keys[i]);
        tempPtrs[i] = noAtual->ponteiros[i];
    }
    tempPtrs[tam] = noAtual->ponteiros[tam]; // último ponteiro

    // Encontra posição de inserção
    int i = tam-1;
    while (i>=0 && KeyOps<key>::less(chave, tempKeys[i])) {
        tempKeys[i+1] = tempKeys[i];
        tempPtrs[i+2] = tempPtrs[i+1];
        i--;
    }
    int pos = i+1;
    KeyOps<key>::copy(tempKeys[pos], chave);
    tempPtrs[pos+1] = ponteiro;
    tam++;

    // -----------------------------
    // 2️⃣ Determinar meio
    // -----------------------------
    int meio = tam / 2;

    // -----------------------------
    // 3️⃣ Separar nós folha vs interno
    // -----------------------------
    noNovo->folha = noAtual->folha;
    noNovo->qtdKeys = 0;
    for (int j=0;j<M+2;j++) noNovo->ponteiros[j] = static_cast<ptr>(-1);

    if (noAtual->folha) {
        // Folha B+: copia metade direita para noNovo
        for (int j=meio;j<tam;j++) {
            KeyOps<key>::copy(noNovo->keys[j-meio], tempKeys[j]);
            noNovo->ponteiros[j-meio] = tempPtrs[j];
        }
        noNovo->qtdKeys = tam - meio;
        noAtual->qtdKeys = meio;

        // ligações de folha
        noNovo->ponteiros[M] = noAtual->ponteiros[M]; // antigo próximo
        ptr pNovo = escrever(file, noNovo, cache);
        noAtual->ponteiros[M] = pNovo;

        // Atualiza noAtual com metade esquerda
        for (int j=0;j<meio;j++) {
            KeyOps<key>::copy(noAtual->keys[j], tempKeys[j]);
            noAtual->ponteiros[j] = tempPtrs[j];
        }
        reescrever(file, pAtual, noAtual, cache);

        // Promove a menor chave do novo nó
        key promovida;
        KeyOps<key>::copy(promovida, noNovo->keys[0]);
        return {promovida, pNovo};
    } else {
        // Nó interno: promove chave central
        key promovida;
        KeyOps<key>::copy(promovida, tempKeys[meio]);

        // No direito (novo nó) recebe chaves meio+1..tam-1
        int idx = 0;
        for (int j=meio+1;j<tam;j++) {
            KeyOps<key>::copy(noNovo->keys[idx], tempKeys[j]);
            idx++;
        }
        // Ponteiros do novo nó: meio+1 .. tam
        for (int j=meio+1;j<=tam;j++) {
            noNovo->ponteiros[j-(meio+1)] = tempPtrs[j];
        }
        noNovo->qtdKeys = tam - (meio + 1);

        // No esquerdo (noAtual) fica com 0..meio-1
        noAtual->qtdKeys = meio;
        for (int j=0;j<meio;j++) {
            KeyOps<key>::copy(noAtual->keys[j], tempKeys[j]);
            noAtual->ponteiros[j] = tempPtrs[j];
        }
        noAtual->ponteiros[meio] = tempPtrs[meio];

        ptr pNovo = escrever(file, noNovo, cache);
        reescrever(file, pAtual, noAtual, cache);

        return {promovida, pNovo};
    }
}

// Desce na árvore buscando a folha correspondente à chave alvo
template<typename key, int M>
pair<ptr, int> acharFolha(fstream *file, ptr pRaiz, key& alvo, stack<ptr> *pilha = NULL, PageCache<key,M>* cache = nullptr){
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

        no<key, M> raiz; 
        // já default-inicializado via construtor
        raiz.folha = true; raiz.qtdKeys = 0;
        // Inicializa ponteiros com -1 por segurança (construtor já fez isso, repetimos por clareza)
        for (int i = 0; i < M+2; ++i) raiz.ponteiros[i] = static_cast<ptr>(-1);
        this->raiz = escrever(f, &raiz, cache.get());
        this->qtd_nos += 1;
        this->primeiraFolha = this->raiz;
    }

    void carregarArvore(fstream* f){
        this->file = f;
        this->cache = make_unique<PageCache<key,M>>(f);
        this->cache->setFile(f);
        this->raiz = static_cast<ptr>(0);
    }

    // Força flush do cache para disco
    void flushCache() {
        if (cache) cache->flush();
    }

    pair<bool, long> buscar(key alvo, Registro& encontrado, fstream* db){
        int qtd_blocos = 0;
        no<key, M> folha;
        pair<ptr,int> res = acharFolha(file, this->raiz, alvo, nullptr, cache.get());
        
        ptr pFolha = res.first; 
        carregar(file, pFolha, &folha, cache.get());
        qtd_blocos = res.second + 1;        

        // Busca binária na folha (encontra igual ou primeiro maior)
        int i = lower_bound(folha.keys, folha.keys + folha.qtdKeys, alvo, comp<key>())-folha.keys;
        Registro temp{};

        pair<bool, long> res2;
        res2.second = qtd_blocos;
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
        no<key, M> novoNo; // já inicializado pelo construtor
        novoNo.folha = true; novoNo.qtdKeys = 0;
        // inicializa ponteiros do novo nó (construtor já fez, repetimos por clareza)
        for (int i = 0; i < M+2; ++i) novoNo.ponteiros[i] = static_cast<ptr>(-1);

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
            no<key, M> novoNo2; // default-inicializado
            novoNo2.folha = false; novoNo2.qtdKeys = 0;
            for (int i = 0; i < M+2; ++i) novoNo2.ponteiros[i] = static_cast<ptr>(-1);

            promovida = cisao(file, pAtual, &noAtual, &novoNo2, chave, ponteiro, cache.get());
            this->qtd_nos += 1;
            chave = promovida.first; ponteiro = promovida.second;

            // Se chegou na raíz, para
            if(pilha.empty()) break; 
        }

        // Cria nova raíz
        no<key, M> novaRaiz; // default-inicializado
        novaRaiz.folha = false;
        // inicializa ponteiros (construtor já fez)
        for (int i = 0; i < M+2; ++i) novaRaiz.ponteiros[i] = static_cast<ptr>(-1);
        KeyOps<key>::copy(novaRaiz.keys[0], chave);
        novaRaiz.ponteiros[0] = pAtual;
        novaRaiz.ponteiros[1] = ponteiro;
        novaRaiz.qtdKeys = 1;
        this->raiz = escrever(file, &novaRaiz, cache.get());
        this->qtd_nos += 1;
    }

    // Destrutor para garantir que o cache seja flushado ao final
    ~bp() {
        if (cache) cache->flush();
    }

    void mostrarArvore(ptr noAtual, int nivel) {
        if (noAtual == static_cast<ptr>(-1)) return;

        no<key, M> no;
        carregar(file, noAtual, &no, cache.get());

        if (nivel == 0) cout << "Quantidade de nós: " << this->qtd_nos << "\n";
        
        // Imprime indentação conforme o nível
        for (int i=0; i<nivel; i++) cout << "  ";
        
        // Imprime o nó
        cout << "No " << static_cast<long long>(noAtual) << " (nivel " << nivel << ", " 
            << (no.folha ? "folha" : "interno") << "): ";
        
        for (int i=0;i<no.qtdKeys;i++) {
            // Usar KeyOps::print para tipos especiais
            printKey(no.keys[i]);
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
};

// template struct bp<array<char,300>, 14>;
// template struct bp<int, 341>;

