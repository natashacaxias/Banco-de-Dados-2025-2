// ============================================================
// seek2.cpp
// ------------------------------------------------------------
// Programa responsável por buscar um registro no arquivo de
// dados utilizando o índice secundário (B+Tree por Título).
//
// A busca é realizada com base na chave textual (Título).
// O índice B+ mapeia o título → posição no arquivo de dados,
// permitindo localizar o registro correspondente de forma
// eficiente em disco.
// ============================================================

#include <bits/stdc++.h>
#include <chrono>
#include <fstream>
#include "../include/bptreefile.h"
#include "../include/common.h"
#include "../include/logger.h"   // <<< Adicionado para o LOG_LEVEL
using namespace std;

// ============================================================
// Função principal
// ------------------------------------------------------------
// Espera como argumento na linha de comando:
//   ./bin/seek2 "<Título>"
//
// Exemplo de execução dentro do container:
//   docker run --rm -v $(pwd)/data:/data tp2 ./bin/seek2 "Deep Learning Survey"
// ============================================================
int main(int argc, char* argv[]) {
    Logger log;  // <<< Instancia o logger

    auto inicioTotal = chrono::high_resolution_clock::now();

    log.info("=== TP2 – Busca por Título (seek2) ===");

    // --------------------------------------------------------
    // Validação dos argumentos
    // --------------------------------------------------------
    if (argc < 2) {
        log.error("Uso: ./bin/seek2 <Título>");
        return 1;
    }

    // --------------------------------------------------------
    // Inicialização dos caminhos e parâmetros
    // --------------------------------------------------------
    string chaveStr = argv[1];               // título fornecido
    string dbPath = "/data/data.db";         // arquivo de dados principal
    string idxPath = "/data/bptreeTitulo.idx"; // arquivo de índice secundário

    log.info("Arquivo de dados: " + dbPath);
    log.info("Arquivo de índices: " + idxPath);
    log.info("Chave: " + chaveStr);

    // ========================================================
    // Etapa 1: Abertura dos arquivos
    // ========================================================

    fstream bptFile(idxPath, ios::in | ios::out | ios::binary);
    if (!bptFile.is_open()) {
        log.error("Erro ao abrir arquivo de índice B+ secundário: " + idxPath);
        return 1;
    }

    fstream db(dbPath, ios::in | ios::out | ios::binary);
    if (!db.is_open()) {
        log.error("Erro ao abrir arquivo de dados: " + dbPath);
        return 1;
    }

    // ========================================================
    // Etapa 2: Definição do tipo de chave e carregamento da árvore
    // ========================================================

    using Chave = array<char, 300>;
    bp<Chave, M_TITULO> bptree;
    bptree.carregarArvore(&bptFile);

    Registro r;

    // ========================================================
    // Etapa 3: Preparação da chave para busca
    // ========================================================
    Chave chave{};
    memset(chave.data(), 0, sizeof(Chave));
    strncpy(chave.data(), chaveStr.c_str(), sizeof(Chave));

    // ========================================================
    // Etapa 4: Execução da busca no índice
    // ========================================================
    log.info("Recuperando registro do arquivo de dados...");
    auto inicioBusca = chrono::high_resolution_clock::now();

    pair<bool, long> res = bptree.buscar(chave, r, &db);

    auto fimBusca = chrono::high_resolution_clock::now();
    double tempoBusca = chrono::duration<double, milli>(fimBusca - inicioBusca).count();

    // ========================================================
    // Etapa 5: Exibição do resultado
    // ========================================================
    if (res.first) {
        log.info("Registro encontrado: ");
        cout << "ID: " << r.id << "\n";
        cout << "Título: " << string(r.titulo.data()) << "\n";
        cout << "Ano: " << r.ano << "\n";
        cout << "Autores: " << r.autores << "\n";
        cout << "Citações: " << r.citacoes << "\n";
        cout << "Atualização: " << r.data_atualizacao << "\n";
        cout << "Snippet: " << r.snippet << "\n\n";
    } else {
        log.warn("Registro não encontrado no arquivo de dados.");
    }

    // ========================================================
    // Etapa 6: Estatísticas da busca
    // ========================================================
    log.info("Estatísticas da busca:");
    cout << "  Blocos lidos no índice: " << res.second << endl;
    cout << "  Tempo de busca: " << tempoBusca << " ms" << endl;
    cout << "  Total de blocos no índice: " << bptree.contarBlocos() << "\n\n";

    // ========================================================
    // Etapa 7: Estatísticas gerais e encerramento
    // ========================================================
    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    log.info(" Estatísticas gerais:");
    cout << fixed << setprecision(2);
    cout << "  Tempo total de execução: " << tempoTotal << " ms" << endl;
    cout << "  Total de blocos no arquivo: " << bptree.contarBlocos() << "\n" << endl;

    log.debug("Execução concluída com sucesso. ");
    return 0;
}
