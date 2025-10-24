#ifndef HASHFILE_H
#define HASHFILE_H

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include "commom.h"
using namespace std;

// =====================
// Estruturas
// =====================

struct loteReturn{
    long long pos;
    array<char,300> titulo;
    int id;
};

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
// Classe principal
// =====================
class HashFile {
private:
    string filePath;   // caminho do arquivo físico
    int numBuckets;    // número total de buckets
    int bucketSize;    // número de registros por bucket (na área primária)

    // estatísticas internas (para a operação corrente)
    long blocosLidos = 0;

    // helpers
    long long fileSizeBytes() const;

public:
    HashFile(string path, int nb, int bs);

    void criarArquivoVazio();
    void inserir(const Registro& r);
    vector<loteReturn> inserirEmLote(const vector<Registro>& regs);
    bool buscar(int id, Registro& encontrado);

    // métricas
    long getBlocosLidos() const { return blocosLidos; }
    long getTotalBlocos() const; // total de "blocos" = número de registros no arquivo
    void resetarMetricas() { blocosLidos = 0; }

    // util
    int hashFunction(int key) const;
};

#endif
