#include "hashfile.h"
#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

static inline long long regSize() { return static_cast<long long>(sizeof(Registro)); }

HashFile::HashFile(string path, int nb, int bs)
    : filePath(std::move(path)), numBuckets(nb), bucketSize(bs) {}

long long HashFile::fileSizeBytes() const {
    ifstream f(filePath, ios::binary | ios::ate);
    if (!f.is_open()) return 0;
    return static_cast<long long>(f.tellg());
}

// Criação instantânea via ftruncate
void HashFile::criarArquivoVazio() {
    int fd = open(filePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("❌ Erro ao criar arquivo de dados");
        return;
    }
    off_t tamanho = (off_t)numBuckets * bucketSize * sizeof(Registro);
    if (ftruncate(fd, tamanho) != 0) {
        perror("❌ Erro no ftruncate");
    } else {
        cout << "📁 Arquivo de dados alocado com "
             << (tamanho / (1024.0 * 1024.0)) << " MB." << endl;
    }
    close(fd);
}

int HashFile::hashFunction(int key) const {
    int m = numBuckets > 0 ? numBuckets : 1;
    int h = key % m;
    return (h < 0 ? h + m : h);
}

// Inserção em lote ultra-rápida: escrita sequencial bucket a bucket
void HashFile::inserirEmLote(const vector<Registro>& regs) {
    if (regs.empty()) return;

    // Agrupar registros por bucket
    vector<vector<Registro>> buckets(numBuckets);
    for (const auto& r : regs) {
        int b = hashFunction(r.id);
        buckets[b].push_back(r);
    }

    fstream file(filePath, ios::in | ios::out | ios::binary);
    if (!file.is_open()) {
        cerr << "❌ Erro ao abrir arquivo de dados: " << filePath << endl;
        return;
    }

    const long long tamRegistro = sizeof(Registro);

    for (int b = 0; b < numBuckets; b++) {
        if (buckets[b].empty()) continue;

        long long base = 1LL * b * bucketSize * tamRegistro;
        file.seekp(base, ios::beg);

        long registrosEscritos = 0;
        for (auto& r : buckets[b]) {
            if (registrosEscritos >= bucketSize) {
                // bucket cheio: encadeia no final do arquivo
                file.seekp(0, ios::end);
                long long newOffset = static_cast<long long>(file.tellp());
                r.prox = -1;
                file.write(reinterpret_cast<const char*>(&r), sizeof(Registro));

                // atualizar encadeamento do anterior
                if (registrosEscritos > 0) {
                    long long prevOffset = newOffset - sizeof(Registro);
                    Registro anterior{};
                    file.seekg(prevOffset);
                    file.read(reinterpret_cast<char*>(&anterior), sizeof(Registro));
                    anterior.prox = (int32_t)newOffset;
                    file.seekp(prevOffset);
                    file.write(reinterpret_cast<const char*>(&anterior), sizeof(Registro));
                }
            } else {
                file.write(reinterpret_cast<const char*>(&r), sizeof(Registro));
                registrosEscritos++;
            }
        }
    }

    file.close();
    cout << "🧩 Inserido lote de " << regs.size() << " registros (escrita bucketizada).\n" << flush;
}

// Busca igual à versão anterior
bool HashFile::buscar(int id, Registro& encontrado) {
    resetarMetricas();
    fstream file(filePath, ios::in | ios::binary);
    if (!file.is_open()) return false;

    const long long base = 1LL * hashFunction(id) * bucketSize * regSize();
    Registro temp{};

    for (int i = 0; i < bucketSize; i++) {
        file.seekg(base + i * regSize(), ios::beg);
        file.read(reinterpret_cast<char*>(&temp), sizeof(Registro));
        if (!file) break;
        blocosLidos++;

        if (temp.id == id) {
            encontrado = temp;
            file.close();
            return true;
        }

        int32_t prox = temp.prox;
        while (prox != -1) {
            file.seekg(prox, ios::beg);
            file.read(reinterpret_cast<char*>(&temp), sizeof(Registro));
            if (!file) break;
            blocosLidos++;
            if (temp.id == id) {
                encontrado = temp;
                file.close();
                return true;
            }
            prox = temp.prox;
        }
    }

    file.close();
    return false;
}

long HashFile::getTotalBlocos() const {
    long long bytes = fileSizeBytes();
    if (bytes <= 0) return 0;
    return static_cast<long>(bytes / regSize());
}
