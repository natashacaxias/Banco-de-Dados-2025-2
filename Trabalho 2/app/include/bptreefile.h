#ifndef BPTREE_H
#define BPTREE_H

#include <fstream>
#include <stack>
#include <algorithm>

using namespace std;

const int M = 4; // FAZER FUNÇÃO PRA OBTER BLOCO E CALCULAR M CORRETO!!
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