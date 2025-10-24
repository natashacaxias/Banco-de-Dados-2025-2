#ifndef COMMOM_H
#define COMMOM_H

#include <bits/stdc++.h>
using namespace std;
using ptr = long long;

struct Registro {
    int id;
    array<char,300> titulo;           
    char ano[8];
    char autores[150];          
    char citacoes[16];
    char data_atualizacao[32];
    char snippet[1024];         
    int32_t prox;               
};

static inline long long regSize() { return static_cast<long long>(sizeof(Registro)); }

#endif