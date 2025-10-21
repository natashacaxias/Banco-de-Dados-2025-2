#ifndef HASHFILE_H
#define HASHFILE_H

#include <string>
#include <vector>
#include <ctime>

using namespace std;

// Cada registro armazenado no arquivo
struct Registro {
    int id;
    char titulo[300];
    int ano;
    char autores[150];
    int citacoes;
    char data_atualizacao[20];
    char snippet[1024];
    int prox; // ponteiro para próximo registro no caso de colisão (-1 = nulo)
};

// Estrutura para gerenciar o arquivo hash
class HashFile {
private:
    string path;
    int numBuckets;
    int bucketSize;
    int blocosLidos;

    int hashFunction(int id);

public:
    HashFile(string path, int numBuckets = 100, int bucketSize = 4);
    bool criarArquivoVazio();
    bool inserir(const Registro& r);
    bool buscar(int id, Registro& resultado);
    void exibir(const Registro& r);
    int getBlocosLidos() const;
};

#endif
