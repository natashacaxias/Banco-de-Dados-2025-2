#include "../include/hashfile.h"
#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

// construtor: inicializa caminho e parâmetros
HashFile::HashFile(string path, int nb, int bs)
    : filePath(std::move(path)), numBuckets(nb), bucketSize(bs) {}

// retorna o tamanho atual do arquivo (em bytes)
int64_t HashFile::fileSizeBytes() const {
    ifstream f(filePath, ios::binary | ios::ate);
    if (!f.is_open()) return 0;
    return static_cast<int64_t>(f.tellg());
}

// cria o arquivo base de dados com registros vazios
void HashFile::criarArquivoVazio() {
    cout << filePath.c_str() << "\n\n";

    fstream file(filePath, ios::out | ios::binary | ios::trunc);
    if (!file.is_open()) {
        cerr << "Erro ao criar arquivo de dados: " << filePath << endl;
        return;
    }

    Registro vazio{};
    vazio.id = 0;
    vazio.prox = -1;

    const int64_t totalRegistros = 1LL * numBuckets * bucketSize;
    const double tamanhoMB = (totalRegistros * sizeof(Registro)) / (1024.0 * 1024.0);

    cout << "📁 Criando arquivo de dados vazio..." << endl;
    cout << "Total de registros: " << totalRegistros << endl;
    cout << fixed << setprecision(3)
         << "Tamanho estimado: " << tamanhoMB << " MB\n";

    // grava todos os registros vazios
    for (int64_t i = 0; i < totalRegistros; ++i) {
        file.write(reinterpret_cast<const char*>(&vazio), sizeof(Registro));
    }

    file.close();
    cout << "✅ Arquivo de dados inicializado com sucesso." << endl;
}

// função hash simples (mod numBuckets)
int HashFile::hashFunction(int key) const {
    int m = numBuckets > 0 ? numBuckets : 1;
    int h = key % m;
    return (h < 0 ? h + m : h);
}

// inserção em lote — otimiza escrita agrupando por bucket
vector<loteReturn> HashFile::inserirEmLote(const vector<Registro>& regs) {
    vector<loteReturn> indices;
    indices.reserve(regs.size());
    if (regs.empty()) return indices;

    // agrupa os registros conforme o bucket calculado
    vector<vector<pair<int, Registro>>> buckets(numBuckets);
    for (size_t i = 0; i < regs.size(); ++i) {
        int b = hashFunction(regs[i].id);
        buckets[b].push_back({static_cast<int>(i), regs[i]});
    }

    // abre o arquivo para leitura e escrita
    fstream file(filePath, ios::in | ios::out | ios::binary);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo de dados: " << filePath << endl;
        return indices;
    }

    const int64_t tamRegistro = sizeof(Registro);
    unordered_map<int, int64_t> idToPos; // id → posição real no arquivo

    // percorre cada bucket
    for (int b = 0; b < numBuckets; b++) {
        if (buckets[b].empty()) continue;

        int64_t base = 1LL * b * bucketSize * tamRegistro;
        int64_t registrosOcupados = 0;
        Registro tmp{};
        file.clear();
        file.seekg(base, ios::beg);

        // procura a primeira posição livre no bucket
        for (int i = 0; i < bucketSize; ++i) {
            file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
            if (!file) { file.clear(); break; }
            if (tmp.id == 0) { // espaço livre
                registrosOcupados = i;
                break;
            }
            registrosOcupados = i + 1;
        }

        // verifica se precisa de overflow (encadeamento)
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

        // escreve os registros do bucket
        for (auto [origIndex, rcopy] : buckets[b]) {
            int64_t pos = 0;
            if (registrosOcupados < bucketSize) {
                // escreve na área primária
                pos = base + registrosOcupados * tamRegistro;
                rcopy.prox = -1;
                file.seekp(pos, ios::beg);
                file.write(reinterpret_cast<const char*>(&rcopy), sizeof(Registro));
                registrosOcupados++;
            } else {
                // área de overflow (no final do arquivo)
                file.seekp(0, ios::end);
                pos = static_cast<int64_t>(file.tellp());
                rcopy.prox = -1;
                file.write(reinterpret_cast<const char*>(&rcopy), sizeof(Registro));

                // atualiza o ponteiro do último nó da cadeia
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

            // guarda posição do registro inserido
            idToPos[rcopy.id] = pos;
        }
    }

    file.flush();

    // monta vetor de retorno (posição e título de cada registro)
    indices.reserve(regs.size());
    for (const auto& r : regs) {
        loteReturn lr{};
        lr.id = r.id;
        strncpy(lr.titulo.data(), r.titulo.data(), sizeof(lr.titulo) - 1);
        lr.titulo[sizeof(lr.titulo) - 1] = '\0';
        lr.pos = idToPos.count(r.id) ? idToPos[r.id] : static_cast<int64_t>(-1);
        indices.push_back(lr);
    }

    cout << "Inserido lote de " << regs.size() << " registros.\n";
    return indices;
}

// busca um registro pelo ID (percorre bucket e overflow)
bool HashFile::buscar(int id, Registro& encontrado) {
    resetarMetricas();
    fstream file(filePath, ios::in | ios::binary);
    if (!file.is_open()) return false;

    const int64_t base = 1LL * hashFunction(id) * bucketSize * regSize();
    Registro temp{};

    // varre o bucket principal
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

        // percorre área de overflow (se houver)
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

// calcula quantos blocos existem no arquivo
long HashFile::getTotalBlocos() const {
    int64_t bytes = fileSizeBytes();
    if (bytes <= 0) return 0;
    return static_cast<long>(bytes / regSize());
}
