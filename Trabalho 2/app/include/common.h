#ifndef COMMON_H
#define COMMON_H

#include <bits/stdc++.h>
using namespace std;
using ptr = int64_t; // tipo para posições no arquivo

// gravação binária exata (sem padding)
#pragma pack(push, 1)

// --- Constantes principais ---
const int NUM_BUCKETS = 100003;   // nº de buckets do hash
const int BUCKET_SIZE = 10;       // registros por bucket
const int BATCH_SIZE = 10000;     // lote de leitura no upload
const int PROGRESS_STEP = 50000;  // passo para exibir progresso
const int M_ID = 341;             // ordem da B+Tree (índice por ID)
const int M_TITULO = 14;          // ordem da B+Tree (índice por título)

// --- Estrutura do registro no arquivo de dados ---
struct Registro {
    int id;                       // identificador
    array<char,300> titulo;       // título do artigo
    char ano[8];                  // ano de publicação
    char autores[150];            // lista de autores
    char citacoes[16];            // nº de citações
    char data_atualizacao[32];    // data/hora da última atualização
    char snippet[1024];           // resumo do artigo
    ptr prox;                     // ponteiro p/ encadeamento no hash
};

#pragma pack(pop)

// retorna o tamanho de um registro (em bytes)
static inline ptr regSize() { return static_cast<ptr>(sizeof(Registro)); }

#endif
