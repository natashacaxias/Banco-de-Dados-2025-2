// ============================================================
// seek1.cpp
// ------------------------------------------------------------
// Programa responsável por buscar um registro no arquivo de
// dados utilizando o índice primário (B+Tree por ID).
//
// A partir do ID informado, a árvore B+ é percorrida até a
// folha correspondente, e o ponteiro encontrado leva ao
// registro real armazenado no arquivo de dados.
// ============================================================

#include <bits/stdc++.h>
#include <chrono>
#include <fstream>
#include "../include/bptreefile.h"
#include "../include/common.h"
#include "../include/logger.h" // ✅ adicionado

using namespace std;

// ============================================================
// Função principal
// ------------------------------------------------------------
// Espera como argumento na linha de comando:
//   ./bin/seek1 <ID>
//
// Exemplo de execução dentro do container:
//   docker run --rm -v $(pwd)/data:/data tp2 ./bin/seek1 532
// ============================================================
int main(int argc, char* argv[]) {
    Logger log; // logger com controle por LOG_LEVEL
    auto inicioTotal = chrono::high_resolution_clock::now();

    log.info("=== TP2 – Busca por ID (seek1) ===");

    // --------------------------------------------------------
    // Validação dos argumentos
    // --------------------------------------------------------
    if (argc < 2) {
        log.error("Uso: ./bin/seek1 <ID>");
        return 1;
    }

    // --------------------------------------------------------
    // Inicialização dos caminhos e parâmetros
    // --------------------------------------------------------
    string chaveStr = argv[1];
    string dbPath = "/data/data.db";
    string idxPath = "/data/bptreeId.idx";

    log.info("Arquivo de dados: " + dbPath);
    log.info("Arquivo de índices: " + idxPath);
    log.info("Chave: " + chaveStr);

    // ========================================================
    // Etapa 1: Abertura dos arquivos necessários
    // ========================================================
    fstream bptFile(idxPath, ios::in | ios::out | ios::binary);
    if (!bptFile.is_open()) {
        log.error("Erro ao abrir arquivo de índice B+ primário: " + idxPath);
        return 1;
    }

    fstream db(dbPath, ios::in | ios::out | ios::binary);
    if (!db.is_open()) {
        log.error("Erro ao abrir arquivo de dados: " + dbPath);
        return 1;
    }

    // ========================================================
    // Etapa 2: Carrega a B+Tree e prepara a busca
    // ========================================================
    log.debug("Carregando estrutura da B+Tree a partir do disco...");
    bp<int, M_ID> bptree;
    bptree.carregarArvore(&bptFile);

    Registro r;
    int id = stoi(chaveStr);
    log.debug("ID convertido para inteiro: " + to_string(id));

    // ========================================================
    // Etapa 3: Busca no índice e leitura do registro
    // ========================================================
    log.info("Recuperando registro do arquivo de dados...");
    auto inicioBusca = chrono::high_resolution_clock::now();

    pair<bool, long> res = bptree.buscar(id, r, &db);

    auto fimBusca = chrono::high_resolution_clock::now();
    double tempoBusca = chrono::duration<double, milli>(fimBusca - inicioBusca).count();

    // ========================================================
    // Etapa 4: Exibição do resultado da busca
    // ========================================================
    if (res.first) {
        log.info("Registro encontrado:");
        cout << "ID: " << r.id << "\n";
        cout << "Título: " << r.titulo.data() << "\n";
        cout << "Ano: " << r.ano << "\n";
        cout << "Autores: " << r.autores << "\n";
        cout << "Citações: " << r.citacoes << "\n";
        cout << "Atualização: " << r.data_atualizacao << "\n";
        cout << "Snippet: " << r.snippet << "\n";
    } else {
        log.warn("Registro não encontrado no arquivo de dados.");
    }

    // ========================================================
    // Etapa 5: Estatísticas de desempenho
    // ========================================================
    log.info("Estatísticas da busca:");
    cout << "  Blocos lidos no índice: " << res.second << endl;
    cout << "  Tempo de busca: " << fixed << setprecision(3)
         << tempoBusca << " ms" << endl;
    cout << "  Total de blocos no índice: " << bptree.contarBlocos() << endl;

    // ========================================================
    // Etapa 6: Tempo total e finalização
    // ========================================================
    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    log.info("Estatísticas gerais:");
    cout << fixed << setprecision(2);
    cout << "  Tempo total de execução: " << tempoTotal << " ms" << endl;
    cout << "  Total de blocos no arquivo: " << bptree.contarBlocos() << endl;

    log.debug("Execução concluída com sucesso.");
    return 0;
}
