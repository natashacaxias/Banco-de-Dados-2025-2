#include "hashfile.h"
#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"
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
vector<loteReturn> HashFile::inserirEmLote(const vector<Registro>& regs) {
    vector<loteReturn> indices;
    if (regs.empty()) return indices;

    // Agrupar registros por bucket
    vector<vector<Registro>> buckets(numBuckets);
    for (const auto& r : regs) {
        int b = hashFunction(r.id);
        buckets[b].push_back(r);
    }

    fstream file(filePath, ios::in | ios::out | ios::binary);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo de dados: " << filePath << endl;
        return indices;
    }

    const int64_t tamRegistro = sizeof(Registro);

    for (int b = 0; b < numBuckets; b++) {
        if (buckets[b].empty()) continue;

        int64_t base = 1LL * b * bucketSize * tamRegistro;

        // 1) Determinar quantos registros já ocupam a área primária deste bucket
        int64_t registrosEscritos = 0;
        Registro tmp{};
        file.clear();
        file.seekg(base, ios::beg);

        // Percorre a área primária procurando primeira posição livre (id == 0)
        for (int i = 0; i < bucketSize; ++i) {
            file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
            if (!file) { file.clear(); break; }
            if (tmp.id == 0) { // assumimos id==0 => slot livre (criararquivo preenche com zeros)
                registrosEscritos = i;
                break;
            }
            registrosEscritos = i + 1;
        }

        // 2) Se já houver overflow, achar o último nó da cadeia para linkar novos appends
        int64_t lastChainPos = -1;
        if (registrosEscritos >= bucketSize) {
            // Pega o último slot primário e percorre o campo prox até -1
            int64_t lastPrimaryPos = base + (bucketSize - 1) * tamRegistro;
            file.seekg(lastPrimaryPos, ios::beg);
            file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
            int64_t prox = tmp.prox;
            if (prox == -1) {
                lastChainPos = lastPrimaryPos;
            } else {
                // percorre os nós de overflow até o fim
                int64_t cur = prox;
                while (cur != -1) {
                    file.seekg(cur, ios::beg);
                    file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
                    if (tmp.prox == -1) {
                        lastChainPos = cur;
                        break;
                    }
                    cur = tmp.prox;
                }
                // se nunca leu com sucesso, lastChainPos permanece -1 (tratamento abaixo)
            }
        }

        // 3) Inserir registros do lote para este bucket
        for (auto rcopy : buckets[b]) {
            int64_t pos = 0;
            if (registrosEscritos < bucketSize) {
                // grava na área primária na primeira posição livre
                pos = base + registrosEscritos * tamRegistro;
                rcopy.prox = -1;
                file.seekp(pos, ios::beg);
                file.write(reinterpret_cast<const char*>(&rcopy), sizeof(Registro));
                registrosEscritos++;
            } else {
                // overflow: append ao final do arquivo e linkar o último nó da cadeia
                file.seekp(0, ios::end);
                pos = static_cast<int64_t>(file.tellp());
                rcopy.prox = -1;
                file.write(reinterpret_cast<const char*>(&rcopy), sizeof(Registro));

                // Atualiza ponteiro do último nó da cadeia para apontar para pos
                if (lastChainPos == -1) {
                    // tenta linkar ao último slot primário
                    int64_t prevOffset = base + (bucketSize - 1) * tamRegistro;
                    file.seekg(prevOffset, ios::beg);
                    file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
                    tmp.prox = static_cast<int64_t>(pos);
                    file.seekp(prevOffset, ios::beg);
                    file.write(reinterpret_cast<const char*>(&tmp), sizeof(Registro));
                } else {
                    file.seekg(lastChainPos, ios::beg);
                    file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
                    tmp.prox = static_cast<int64_t>(pos);
                    file.seekp(lastChainPos, ios::beg);
                    file.write(reinterpret_cast<const char*>(&tmp), sizeof(Registro));
                }
                lastChainPos = pos;
            }

            // registra (id, posição) para o índice B+
            loteReturn novo;
            novo.id = rcopy.id;
            strncpy(novo.titulo.data(), rcopy.titulo.data(), sizeof(novo.titulo) - 1);
            novo.titulo.data()[sizeof(novo.titulo) - 1] = '\0';
            novo.pos = pos;
            indices.push_back(novo);
        }
    }

    file.close();
    cout << "🧩 Inserido lote de " << regs.size()
         << " registros (escrita bucketizada, sem sobrescrita).\n" << flush;

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
