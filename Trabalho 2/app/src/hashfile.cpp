#include "../include/hashfile.h"
#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

HashFile::HashFile(string path, int nb, int bs)
    : filePath(std::move(path)), numBuckets(nb), bucketSize(bs) {}

int64_t HashFile::fileSizeBytes() const {
    ifstream f(filePath, ios::binary | ios::ate);
    if (!f.is_open()) return 0;
    return static_cast<int64_t>(f.tellg());
}

// Criação instantânea via ftruncate
void HashFile::criarArquivoVazio() {
    cout << filePath.c_str() << "\n\n";
    int fd = open(filePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("Erro ao criar arquivo de dados");
        return;
    }
    off_t tamanho = (off_t)numBuckets * bucketSize * sizeof(Registro);
    if (ftruncate(fd, tamanho) != 0) {
        perror("Erro no ftruncate");
    } else {
        cout << "Arquivo de dados alocado com "
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
vector<loteReturn> HashFile::inserirEmLote(const vector<Registro>& regs) {
    vector<loteReturn> indices;
    indices.reserve(regs.size());
    if (regs.empty()) return indices;

    // 1️⃣ Agrupar registros por bucket (sem alterar a ordem global)
    vector<vector<pair<int, Registro>>> buckets(numBuckets);
    for (size_t i = 0; i < regs.size(); ++i) {
        int b = hashFunction(regs[i].id);
        buckets[b].push_back({static_cast<int>(i), regs[i]}); // guarda o índice original
    }

    // 2️⃣ Abrir arquivo de dados
    fstream file(filePath, ios::in | ios::out | ios::binary);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo de dados: " << filePath << endl;
        return indices;
    }

    const int64_t tamRegistro = sizeof(Registro);
    unordered_map<int, int64_t> idToPos; // id → posição real

    // 3️⃣ Inserir bucket por bucket
    for (int b = 0; b < numBuckets; b++) {
        if (buckets[b].empty()) continue;

        int64_t base = 1LL * b * bucketSize * tamRegistro;

        // Localiza primeira posição livre
        int64_t registrosOcupados = 0;
        Registro tmp{};
        file.clear();
        file.seekg(base, ios::beg);

        for (int i = 0; i < bucketSize; ++i) {
            file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
            if (!file) { file.clear(); break; }
            if (tmp.id == 0) { // livre
                registrosOcupados = i;
                break;
            }
            registrosOcupados = i + 1;
        }

        // Localiza último nó da cadeia (para overflow)
        int64_t lastChainPos = -1;
        if (registrosOcupados >= bucketSize) {
            int64_t lastPrimaryPos = base + (bucketSize - 1) * tamRegistro;
            file.seekg(lastPrimaryPos, ios::beg);
            file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
            int64_t prox = tmp.prox;
            if (prox == -1) {
                lastChainPos = lastPrimaryPos;
            } else {
                while (prox != -1) {
                    file.seekg(prox, ios::beg);
                    file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
                    if (tmp.prox == -1) {
                        lastChainPos = prox;
                        break;
                    }
                    prox = tmp.prox;
                }
            }
        }

        // 4️⃣ Escrever registros deste bucket
        for (auto [origIndex, rcopy] : buckets[b]) {
            int64_t pos = 0;
            if (registrosOcupados < bucketSize) {
                pos = base + registrosOcupados * tamRegistro;
                rcopy.prox = -1;
                file.seekp(pos, ios::beg);
                file.write(reinterpret_cast<const char*>(&rcopy), sizeof(Registro));
                registrosOcupados++;
            } else {
                // overflow
                file.seekp(0, ios::end);
                pos = static_cast<int64_t>(file.tellp());
                rcopy.prox = -1;
                file.write(reinterpret_cast<const char*>(&rcopy), sizeof(Registro));

                // Atualiza o nó anterior para apontar para o novo
                if (lastChainPos == -1) {
                    int64_t prevOffset = base + (bucketSize - 1) * tamRegistro;
                    file.seekg(prevOffset, ios::beg);
                    file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
                    tmp.prox = pos;
                    file.seekp(prevOffset, ios::beg);
                    file.write(reinterpret_cast<const char*>(&tmp), sizeof(Registro));
                } else {
                    file.seekg(lastChainPos, ios::beg);
                    file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
                    tmp.prox = pos;
                    file.seekp(lastChainPos, ios::beg);
                    file.write(reinterpret_cast<const char*>(&tmp), sizeof(Registro));
                }
                lastChainPos = pos;
            }

            // Armazena posição real (id → pos)
            idToPos[rcopy.id] = pos;
        }
    }

    file.flush();

    // 5️⃣ Monta vetor de retorno na mesma ordem do vetor original
    indices.reserve(regs.size());
    for (const auto& r : regs) {
        loteReturn lr{};
        lr.id = r.id;
        strncpy(lr.titulo.data(), r.titulo.data(), sizeof(lr.titulo) - 1);
        lr.titulo[sizeof(lr.titulo) - 1] = '\0';
        lr.pos = idToPos.count(r.id) ? idToPos[r.id] : static_cast<int64_t>(-1);
        indices.push_back(lr);
    }

    cout << "Inserido lote de " << regs.size()
         << " registros.\n";

    return indices;
}

// Busca igual à versão anterior
bool HashFile::buscar(int id, Registro& encontrado) {
    resetarMetricas();
    fstream file(filePath, ios::in | ios::binary);
    if (!file.is_open()) return false;

    const int64_t base = 1LL * hashFunction(id) * bucketSize * regSize();
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

        int64_t prox = temp.prox;
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
    int64_t bytes = fileSizeBytes();
    if (bytes <= 0) return 0;
    return static_cast<long>(bytes / regSize());
}