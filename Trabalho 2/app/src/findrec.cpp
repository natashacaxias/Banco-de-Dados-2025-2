// ============================================================
// findrec.cpp
// ------------------------------------------------------------
// Programa responsável por buscar diretamente no arquivo de
// dados (sem usar índices) um registro identificado por ID.
//
// Implementa a busca sequencial no arquivo organizado por
// hashing em disco. Retorna o registro completo, estatísticas
// de leitura (blocos) e o tempo de execução.
// ============================================================

#include <bits/stdc++.h>
#include <chrono>
#include <fstream>
#include "../include/hashfile.h"
#include "../include/common.h"
#include "../include/logger.h" // ✅ novo logger

using namespace std;

int main(int argc, char* argv[]) {
    Logger log; //  instancia o logger

    // Marca o início da execução total
    auto inicioTotal = chrono::high_resolution_clock::now();
    log.info(" === TP2 – Busca por ID (findrec) === ");

    // --------------------------------------------------------
    // Validação dos argumentos
    // --------------------------------------------------------
    if (argc < 2) {
        log.error("Uso: ./bin/findrec <ID>");
        return 1;
    }

    // --------------------------------------------------------
    // Inicialização de variáveis e caminhos
    // --------------------------------------------------------
    string chaveStr = argv[1];
    string dbPath = "/data/data.db";

    log.info("Arquivo de dados: " + dbPath);
    log.info("Chave: " + chaveStr);

    // --------------------------------------------------------
    // Criação do manipulador do arquivo hash
    // --------------------------------------------------------
    HashFile hashFile(dbPath, NUM_BUCKETS, BUCKET_SIZE);
    Registro r;
    int id = stoi(chaveStr);

    // ========================================================
    // Etapa 1: Busca direta no arquivo de dados
    // ========================================================
    log.info("Recuperando registro do arquivo de dados...");
    auto inicioDados = chrono::high_resolution_clock::now();

    bool encontrado = hashFile.buscar(id, r);

    auto fimDados = chrono::high_resolution_clock::now();
    double tempoBuscaDados = chrono::duration<double, milli>(fimDados - inicioDados).count();

    // --------------------------------------------------------
    // Exibição do resultado da busca
    // --------------------------------------------------------
    if (encontrado) {
        log.info("Registro encontrado: ");
        cout << "\nID: " << r.id << "\n";
        cout << "Título: " << r.titulo.data() << "\n";
        cout << "Ano: " << r.ano << "\n";
        cout << "Autores: " << r.autores << "\n";
        cout << "Citações: " << r.citacoes << "\n";
        cout << "Atualização: " << r.data_atualizacao << "\n";
        cout << "Snippet: " << r.snippet << "\n\n";
    } else {
        log.warn("Registro não encontrado no arquivo de dados.");
    }

    // ========================================================
    // Etapa 2: Estatísticas de desempenho
    // ========================================================
    log.info(" Estatísticas da busca em dados:");
    cout << "  Blocos lidos nos dados: " << hashFile.getBlocosLidos() << endl;
    cout << "  Tempo de busca nos dados: " << fixed << setprecision(3)
         << tempoBuscaDados << " ms" << endl;

    // ========================================================
    // Etapa 3: Estatísticas gerais e encerramento
    // ========================================================
    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    log.info("Estatísticas gerais:");
    cout << fixed << setprecision(2);
    cout << "  Tempo total de execução: " << tempoTotal << " ms" << endl;
    cout << "  Total de blocos no arquivo: " << hashFile.getTotalBlocos() << endl;

    log.debug("Execução concluída sem erros.");

    return 0;
}
