#ifndef HASHFILE_H
#define HASHFILE_H

#include <string>
#include <vector>
#include <cstdint>
using namespace std;

// =====================
// Estruturas
// =====================
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
    void inserirEmLote(const vector<Registro>& regs);
    bool buscar(int id, Registro& encontrado);

    // métricas
    long getBlocosLidos() const { return blocosLidos; }
    long getTotalBlocos() const; // total de "blocos" = número de registros no arquivo
    void resetarMetricas() { blocosLidos = 0; }

    // util
    int hashFunction(int key) const;
};

#endif
