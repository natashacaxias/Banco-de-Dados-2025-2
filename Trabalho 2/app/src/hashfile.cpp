#include "../include/hashfile.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
using namespace std;

static long insertCount = 0;  // contador global de inserções

HashFile::HashFile(string path, int nb, int bs)
    : path(path), numBuckets(nb), bucketSize(bs), blocosLidos(0) {}

int HashFile::hashFunction(int key) {
    return key % numBuckets;
}

bool HashFile::criarArquivoVazio() {
    fstream file(path, ios::out | ios::binary | ios::trunc);
    if (!file) return false;

    Registro vazio;
    memset(&vazio, 0, sizeof(Registro));
    vazio.id = -1;
    vazio.prox = -1;

    for (int i = 0; i < numBuckets * bucketSize; i++)
        file.write(reinterpret_cast<char*>(&vazio), sizeof(Registro));

    file.close();
    return true;
}

// ============================================================
// INSERIR COM ENCADEAMENTO COMPLETO
// ============================================================
bool HashFile::inserir(const Registro& r) {
    fstream file(path, ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "[ERRO] Falha ao abrir arquivo de dados para inserção." << endl;
        return false;
    }

    insertCount++;
    int bucket = hashFunction(r.id);
    long offset = bucket * bucketSize * sizeof(Registro);
    file.seekg(offset);

    Registro temp;

    // 1️⃣ tenta inserir dentro do bucket
    for (int i = 0; i < bucketSize; i++) {
        file.read(reinterpret_cast<char*>(&temp), sizeof(Registro));
        if (temp.id == -1) {
            file.seekp(offset + i * sizeof(Registro));
            file.write(reinterpret_cast<const char*>(&r), sizeof(Registro));
            file.flush();

            if (insertCount % 100000 == 0)
                cout << "[DEBUG] Inseridos: " << insertCount << " registros..." << endl;

            file.close();
            return true;
        }
    }

    // 2️⃣ bucket cheio: encontrar o último da cadeia
    long posUltimo = -1;
    Registro atual;
    for (int i = 0; i < bucketSize; i++) {
        file.seekg(offset + i * sizeof(Registro));
        file.read(reinterpret_cast<char*>(&atual), sizeof(Registro));

        long prox = atual.prox;
        long anterior = (offset + i * sizeof(Registro)) / sizeof(Registro);

        while (prox != -1) {
            file.seekg(prox * sizeof(Registro));
            file.read(reinterpret_cast<char*>(&atual), sizeof(Registro));
            anterior = prox;
            prox = atual.prox;
        }

        if (atual.prox == -1) {
            posUltimo = anterior;
            break;
        }
    }

    // 3️⃣ grava o novo registro no final do arquivo
    file.seekp(0, ios::end);
    long posNovo = file.tellp() / sizeof(Registro);
    file.write(reinterpret_cast<const char*>(&r), sizeof(Registro));
    file.flush();

    // 4️⃣ atualiza ponteiro prox do último registro da cadeia
    if (posUltimo != -1) {
        file.seekg(posUltimo * sizeof(Registro));
        file.read(reinterpret_cast<char*>(&temp), sizeof(Registro));
        temp.prox = posNovo;
        file.seekp(posUltimo * sizeof(Registro));
        file.write(reinterpret_cast<const char*>(&temp), sizeof(Registro));
        file.flush();
    }

    // if (insertCount % 10000 == 0)
    //     cout << "[DEBUG] Bucket cheio! ID=" << r.id
    //          << " gravado na posição encadeada=" << posNovo
    //          << " (ligado após registro " << posUltimo << ")" << endl;

    file.close();
    return true;
}

// ============================================================
// BUSCAR – percorre encadeamento completo
// ============================================================
bool HashFile::buscar(int id, Registro& resultado) {
    fstream file(path, ios::in | ios::binary);
    if (!file) return false;
    blocosLidos = 0;

    int bucket = hashFunction(id);
    long offset = bucket * bucketSize * sizeof(Registro);

    cout << "[DEBUG] Buscando ID=" << id
         << " hash=" << bucket
         << " offset=" << offset << endl;

    Registro temp;

    for (int i = 0; i < bucketSize; i++) {
        file.seekg(offset + i * sizeof(Registro));
        file.read(reinterpret_cast<char*>(&temp), sizeof(Registro));
        blocosLidos++;

        if (temp.id == id) {
            resultado = temp;
            file.close();
            return true;
        }

        long proxAtual = temp.prox;
        while (proxAtual != -1) {
            file.seekg(proxAtual * sizeof(Registro));
            file.read(reinterpret_cast<char*>(&temp), sizeof(Registro));
            blocosLidos++;

            if (temp.id == id) {
                resultado = temp;
                file.close();
                return true;
            }

            proxAtual = temp.prox;
        }
    }

    file.close();
    return false;
}

int HashFile::getBlocosLidos() const {
    return blocosLidos;
}
