#ifndef BPTREE_H
#define BPTREE_H

#include <fstream>
#include <stack>
#include <algorithm>
#include <iostream>
#include <memory>
#include <list>
#include <unordered_map>
#include <cstring>
using namespace std;
using ptr = long long;

struct Registro {
    int id;
    char titulo[300];           // alinhado ao enunciado
    char ano[8];
    char autores[150];          // alinhado ao enunciado
    char citacoes[16];
    char data_atualizacao[32];
    char snippet[1024];         // tamanho max (100–1024). Ajustei para 1024
    int32_t prox;               // offset (em bytes) do próximo registro na cadeia; -1 se fim
};

// ===== Wrapper para char[300] =====
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

// Sobrecarga do operador << para output
inline std::ostream& operator<<(std::ostream& os, const Char300Wrapper& wrapper) {
    os << wrapper.data;
    return os;
}

// ===== Nó da árvore (pode ser folha ou nó interno) =====
template<typename key, int M>
struct no{
    // Espaços mais para fazer inserções temporárias em caso de cisão
    ptr ponteiros[M+2];
    key keys[M+1];
    int qtdKeys;
    bool folha;
};

// Forward declarations
template<typename key, int M, size_t CACHE_SIZE = 64>
class PageCache;

// ===== Cache de páginas =====
template<typename key, int M, size_t CACHE_SIZE>
class PageCache {
public:
    struct CacheEntry {
        ptr endereco;
        no<key, M> pagina;
        bool dirty;
    };

private:
    list<pair<ptr, CacheEntry>> lruList;
    unordered_map<ptr, typename list<pair<ptr, CacheEntry>>::iterator> table;
    fstream* file;

public:
    PageCache(fstream* f = nullptr) : file(f) {}
    void setFile(fstream* f) { file = f; }
    bool contains(ptr address);
    no<key, M>* get(ptr address);
    void put(ptr address, const no<key, M>& node, bool dirty = false);
    void markDirty(ptr address);
    void flush();
    void clearAndFlush();
};

template<typename key, int M>
struct bp {
    ptr raiz;
    ptr primeiraFolha;
    ptr qtd_nos = 0;
    fstream* file;
    unique_ptr<PageCache<key, M, 64>> cache;

    void iniciar(fstream* f);
    void carregarArvore(fstream* f);
    void flushCache();
    //long getTotalBlocos();
    pair<bool, long> buscar(key alvo, Registro& encontrado, fstream* db);
    void inserir(key chave, ptr ponteiro);
    void mostrarArvore(ptr noAtual, int nivel);
    ~bp();
};

// Declarações das funções utilitárias
template<typename key, int M>
void carregar(fstream* file, ptr pag, no<key, M>* noo, PageCache<key, M, 64>* cache = nullptr);

// Declarações explícitas para os tipos que serão usados
extern template struct bp<Char300Wrapper, 64>;

#endif // BPTREE_H