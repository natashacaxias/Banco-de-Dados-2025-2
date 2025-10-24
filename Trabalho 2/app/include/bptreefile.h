#ifndef BPTREEFILE_H
#define BPTREEFILE_H

#include <bits/stdc++.h>
#include"commom.h"
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
    PageCache(fstream* f = nullptr);
    void setFile(fstream* f);
    bool contains(ptr address);
    no<key, M>* get(ptr address);
    void put(ptr address, const no<key, M>& node, bool dirty = false);
    void markDirty(ptr address);
    void flush();
    void clearAndFlush();
};

// ===== Utilitários otimizados com suporte a cache =====
template<typename key, int M>
void carregar(fstream* file, ptr pag, no<key, M> *noo, PageCache<key,M>* cache = nullptr);

template<typename key, int M>
void reescrever(fstream* file, ptr pag, no<key, M> *novo, PageCache<key,M>* cache = nullptr);

template<typename key, int M>
ptr escrever(fstream* file, no<key, M> *novo, PageCache<key,M>* cache = nullptr);

// ===== Traits para tratar diferentes tipos de chave =====
template<typename Key>
struct KeyOps {
    static bool less(const Key& a, const Key& b);
    static bool equal(const Key& a, const Key& b);
    static void copy(Key& dest, const Key& src);
};

template<typename key>
auto comp();

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

// Insere par chave/ponteiro em um nó
template<typename key, int M>
void inserirCP(fstream *file, no<key, M> *noAtual, key& chave, ptr ponteiro, ptr pAtual, PageCache<key,M>* cache);

// Divide nó cheio em dois e promove uma chave
template<typename key, int M>
pair<key,ptr> cisao(fstream *file, ptr pAtual, no<key, M> *noAtual, no<key, M> *noNovo, key chave, ptr ponteiro, PageCache<key,M>* cache);

// Desce na árvore buscando a folha correspondente à chave alvo
template<typename key, int M>
pair<ptr, int> acharFolha(fstream *file, ptr pRaiz, key& alvo, stack<ptr> *pilha = NULL, PageCache<key,M>* cache = nullptr);

// ===== Árvore B+ =====
template<typename key, int M>
struct bp{
    ptr raiz;
    ptr primeiraFolha;
    ptr qtd_nos = 0;
    fstream* file;
    unique_ptr<PageCache<key,M>> cache;

    // Inicializa a árvore e o cache
    void iniciar(fstream* f);
    void carregarArvore(fstream* f);
    void flushCache();
    pair<bool, long> buscar(key alvo, Registro& encontrado, fstream* db);
    void inserir(key chave, ptr ponteiro);
    // void mostrarArvore(ptr noAtual, int nivel);
    ~bp();
};

#endif // BPTREEFILE_H