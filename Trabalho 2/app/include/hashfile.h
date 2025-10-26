#ifndef HASHFILE_H
#define HASHFILE_H

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include "common.h"
using namespace std;

// =====================
// Estruturas auxiliares
// =====================

// retorno da inserção em lote (posição e título do registro)
struct loteReturn {
    int64_t pos;
    array<char,300> titulo;
    int id;
};

// representação de uma linha do CSV (antes de virar Registro)
struct RegistroCSV {
    int id;
    string titulo;
    string ano;
    string autores;
    string citacoes;
    string data_atualizacao;
    string snippet;
};

// =====================
// Classe principal do arquivo hash
// =====================
class HashFile {
private:
    string filePath;    // caminho do arquivo .db
    int numBuckets;     // total de buckets
    int bucketSize;     // registros por bucket
    long blocosLidos = 0; // contador de blocos acessados

    int64_t fileSizeBytes() const; // tamanho do arquivo em bytes

public:
    // construtor
    HashFile(string path, int nb, int bs);

    // cria o arquivo base vazio
    void criarArquivoVazio();

    // insere um registro individual
    void inserir(const Registro& r);

    // insere vários registros de uma vez
    vector<loteReturn> inserirEmLote(const vector<Registro>& regs);

    // busca um registro pelo ID
    bool buscar(int id, Registro& encontrado);

    // métricas
    long getBlocosLidos() const { return blocosLidos; }
    long getTotalBlocos() const; // retorna total de registros gravados
    void resetarMetricas() { blocosLidos = 0; }

    // função hash principal (define bucket)
    int hashFunction(int key) const;
};

#endif
