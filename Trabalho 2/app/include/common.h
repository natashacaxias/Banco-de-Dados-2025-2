#ifndef COMMON_H
#define COMMON_H

#include <bits/stdc++.h>
using namespace std;
using ptr = int64_t;

// Constantes
const int NUM_BUCKETS = 97;
const int BUCKET_SIZE = 4096;
const int BATCH_SIZE = 10000;
const int PROGRESS_STEP = 50000;
const int M_ID = 341;
const int M_TITULO = 14;

struct Registro {
    int id;
    array<char,300> titulo;           
    char ano[8];
    char autores[150];          
    char citacoes[16];
    char data_atualizacao[32];
    char snippet[1024];         
    ptr prox;              
};

static inline ptr regSize() { return static_cast<ptr>(sizeof(Registro)); }

#endif