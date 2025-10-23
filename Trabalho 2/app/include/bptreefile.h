#ifndef BPTREE_H
#define BPTREE_H

#ifndef M
#define M 4096 // Valor padrão caso não seja passado na compilação
#endif

#include <fstream>
#include <stack>
#include <algorithm>
using namespace std;
using ptr = streampos;

template<typename key>
struct bp {
    ptr raiz;
    ptr primeiraFolha;
    ptr prox_livre;
    ptr qtd_pags = 0;
    fstream* file;

    void iniciar(fstream* f);
    ptr busca(key alvo);
    void inserir(key chave, ptr ponteiro);
    void mostrarArvore(ptr noAtual, int nivel);
};

#endif // BPTREE_H