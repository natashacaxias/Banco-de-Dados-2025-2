// ============================================================
// hashfile.cpp
// ------------------------------------------------------------
// Implementação das funções da classe HashFile, responsável
// por gerenciar o arquivo de dados em disco organizado por
// hashing. Esta estrutura oferece acesso direto aos registros
// via função hash, com suporte a área de overflow encadeada.
// ============================================================

#include "../include/hashfile.h"
#include <bits/stdc++.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

// ============================================================
// Construtor
// ------------------------------------------------------------
// Inicializa os parâmetros principais do arquivo hash:
//  - path: caminho do arquivo de dados em disco
//  - nb: número total de buckets
//  - bs: tamanho (número de registros) de cada bucket
// ============================================================
HashFile::HashFile(string path, int nb, int bs)
    : filePath(std::move(path)), numBuckets(nb), bucketSize(bs) {}

// ============================================================
// fileSizeBytes()
// ------------------------------------------------------------
// Retorna o tamanho atual do arquivo de dados em bytes.
// Caso o arquivo não exista ou não possa ser aberto,
// retorna 0.
// ============================================================
int64_t HashFile::fileSizeBytes() const {
    ifstream f(filePath, ios::binary | ios::ate);
    if (!f.is_open()) return 0;
    return static_cast<int64_t>(f.tellg());
}

// ============================================================
// criarArquivoVazio()
// ------------------------------------------------------------
// Cria um novo arquivo de dados vazio, inicializando todos
// os buckets e registros com valores nulos. Cada registro
// começa com id=0 e prox=-1 (sem encadeamento).
// ============================================================
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

    // Grava todos os registros vazios no arquivo
    for (int64_t i = 0; i < totalRegistros; ++i) {
        file.write(reinterpret_cast<const char*>(&vazio), sizeof(Registro));
    }

    file.close();
    cout << "✅ Arquivo de dados inicializado com sucesso." << endl;
}

// ============================================================
// hashFunction()
// ------------------------------------------------------------
// Calcula o índice (bucket) de um determinado ID. A função
// de hashing usada é a divisão simples (módulo do número de
// buckets). Retorna um valor entre [0, numBuckets-1].
// ============================================================
int HashFile::hashFunction(int key) const {
    int m = numBuckets > 0 ? numBuckets : 1;
    int h = key % m;
    return (h < 0 ? h + m : h);
}

// ============================================================
// inserirEmLote()
// ------------------------------------------------------------
// Realiza inserção em lote dos registros, agrupando-os por
// bucket para reduzir operações de disco. Trata colisões
// via encadeamento (overflow) no final do arquivo.
// ------------------------------------------------------------
// Retorna um vetor de estruturas `loteReturn` contendo:
//  - id inserido
//  - título do registro
//  - posição (offset) no arquivo de dados
// ============================================================
vector<loteReturn> HashFile::inserirEmLote(const vector<Registro>& regs) {
    vector<loteReturn> indices;
    indices.reserve(regs.size());
    if (regs.empty()) return indices;

    // --------------------------------------------------------
    // Agrupamento por bucket
    // --------------------------------------------------------
    vector<vector<pair<int, Registro>>> buckets(numBuckets);
    for (size_t i = 0; i < regs.size(); ++i) {
        int b = hashFunction(regs[i].id);
        buckets[b].push_back({static_cast<int>(i), regs[i]});
    }

    // --------------------------------------------------------
    // Abertura do arquivo para leitura/escrita binária
    // --------------------------------------------------------
    fstream file(filePath, ios::in | ios::out | ios::binary);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo de dados: " << filePath << endl;
        return indices;
    }

    const int64_t tamRegistro = sizeof(Registro);
    unordered_map<int, int64_t> idToPos; // mapeia id → offset real

    // ========================================================
    // Percorre cada bucket e insere registros
    // ========================================================
    for (int b = 0; b < numBuckets; b++) {
        if (buckets[b].empty()) continue;

        int64_t base = 1LL * b * bucketSize * tamRegistro;
        int64_t registrosOcupados = 0;
        Registro tmp{};
        file.clear();
        file.seekg(base, ios::beg);

        // ----------------------------------------------------
        // 1. Procura a primeira posição livre no bucket
        // ----------------------------------------------------
        for (int i = 0; i < bucketSize; ++i) {
            file.read(reinterpret_cast<char*>(&tmp), sizeof(Registro));
            if (!file) { file.clear(); break; }
            if (tmp.id == 0) { // espaço livre
                registrosOcupados = i;
                break;
            }
            registrosOcupados = i + 1;
        }

        // ----------------------------------------------------
        // 2. Verifica necessidade de overflow (encadeamento)
        // ----------------------------------------------------
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

        // ----------------------------------------------------
        // 3. Insere cada registro no bucket ou área de overflow
        // ----------------------------------------------------
        for (auto [origIndex, rcopy] : buckets[b]) {
            int64_t pos = 0;
            if (registrosOcupados < bucketSize) {
                // Insere na área primária do bucket
                pos = base + registrosOcupados * tamRegistro;
                rcopy.prox = -1;
                file.seekp(pos, ios::beg);
                file.write(reinterpret_cast<const char*>(&rcopy), sizeof(Registro));
                registrosOcupados++;
            } else {
                // Caso de overflow: escreve no final do arquivo
                file.seekp(0, ios::end);
                pos = static_cast<int64_t>(file.tellp());
                rcopy.prox = -1;
                file.write(reinterpret_cast<const char*>(&rcopy), sizeof(Registro));

                // Atualiza o ponteiro de encadeamento
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

            // Registra a posição real do registro
            idToPos[rcopy.id] = pos;
        }
    }

    // Força gravação no disco
    file.flush();

    // --------------------------------------------------------
    // Monta vetor de retorno com as posições dos registros
    // --------------------------------------------------------
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

// ============================================================
// buscar()
// ------------------------------------------------------------
// Localiza um registro com o ID fornecido, percorrendo o
// bucket correspondente e, se necessário, as áreas de overflow
// encadeadas. Retorna true se o registro for encontrado.
// ============================================================
bool HashFile::buscar(int id, Registro& encontrado) {
    resetarMetricas();
    fstream file(filePath, ios::in | ios::binary);
    if (!file.is_open()) return false;

    const int64_t base = 1LL * hashFunction(id) * bucketSize * regSize();
    Registro temp{};

    // --------------------------------------------------------
    // Varre o bucket principal
    // --------------------------------------------------------
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

        // ----------------------------------------------------
        // Se não encontrado, segue a cadeia de overflow
        // ----------------------------------------------------
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

// ============================================================
// getTotalBlocos()
// ------------------------------------------------------------
// Retorna o número total de blocos (registros) armazenados
// no arquivo de dados, com base em seu tamanho em bytes.
// ============================================================
long HashFile::getTotalBlocos() const {
    int64_t bytes = fileSizeBytes();
    if (bytes <= 0) return 0;
    return static_cast<long>(bytes / regSize());
}
