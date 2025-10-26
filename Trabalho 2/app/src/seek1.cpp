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
    auto inicioTotal = chrono::high_resolution_clock::now();

    cout << "\n=== TP2 – Busca por ID (seek1) ===\n" << endl;

    // --------------------------------------------------------
    // Validação dos argumentos
    // --------------------------------------------------------
    // O programa exige um ID como argumento de entrada.
    // Caso não seja fornecido, exibe instrução de uso.
    if (argc < 2) {
        cerr << "Uso: ./bin/seek1 <ID>" << endl;
        return 1;
    }

    // --------------------------------------------------------
    // Inicialização dos caminhos e parâmetros
    // --------------------------------------------------------
    string chaveStr = argv[1];            // ID fornecido como string
    string dbPath = "/data/data.db";      // caminho do arquivo de dados
    string idxPath = "/data/bptreeId.idx"; // caminho do índice primário (B+Tree)

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Arquivo de índices: " << idxPath << endl;
    cout << "Chave: " << chaveStr << endl;

    // ========================================================
    // Etapa 1: Abertura dos arquivos necessários
    // ========================================================

    // --------------------------------------------------------
    // Abre o arquivo de índice B+ (índice primário por ID)
    // --------------------------------------------------------
    fstream bptFile(idxPath, ios::in | ios::out | ios::binary);
    if (!bptFile.is_open()) {
        cerr << "Erro ao abrir arquivo de índice B+ primário: " << idxPath << endl;
        return 1;
    }

    // --------------------------------------------------------
    // Abre o arquivo de dados principal (hashfile)
    // --------------------------------------------------------
    fstream db(dbPath, ios::in | ios::out | ios::binary);
    if (!db.is_open()) {
        cerr << "Erro ao abrir arquivo de dados: " << dbPath << endl;
        return 1;
    }

    // ========================================================
    // Etapa 2: Carrega a B+Tree e prepara a busca
    // ========================================================

    // Instancia a árvore B+ e carrega sua estrutura do disco
    bp<int, M_ID> bptree;
    bptree.carregarArvore(&bptFile);

    Registro r; // estrutura de saída
    int id = stoi(chaveStr); // converte o argumento para inteiro

    // ========================================================
    // Etapa 3: Busca no índice e leitura do registro
    // ========================================================
    cout << "\nRecuperando registro do arquivo de dados..." << endl;
    auto inicioBusca = chrono::high_resolution_clock::now();

    // Executa a busca na árvore B+ (usa o índice para acessar o arquivo de dados)
    pair<bool, long> res = bptree.buscar(id, r, &db);

    auto fimBusca = chrono::high_resolution_clock::now();
    double tempoBusca = chrono::duration<double, milli>(fimBusca - inicioBusca).count();

    // ========================================================
    // Etapa 4: Exibição do resultado da busca
    // ========================================================
    if (res.first) {
        cout << "\nRegistro encontrado:\n";
        cout << "ID: " << r.id << "\n";
        cout << "Título: " << r.titulo.data() << "\n";
        cout << "Ano: " << r.ano << "\n";
        cout << "Autores: " << r.autores << "\n";
        cout << "Citações: " << r.citacoes << "\n";
        cout << "Atualização: " << r.data_atualizacao << "\n";
        cout << "Snippet: " << r.snippet << "\n";
    } else {
        cout << "\nRegistro não encontrado no arquivo de dados.\n";
    }

    // ========================================================
    // Etapa 5: Estatísticas de desempenho
    // ========================================================
    cout << "\nEstatísticas da busca:" << endl;
    cout << "  Blocos lidos no índice: " << res.second << endl;
    cout << "  Tempo de busca: " << tempoBusca << " ms" << endl;
    cout << "  Total de blocos no índice: " << bptree.contarBlocos() << endl;

    // ========================================================
    // Etapa 6: Tempo total e finalização
    // ========================================================
    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    cout << "\nEstatísticas gerais:" << endl;
    cout << fixed << setprecision(2);
    cout << "  Tempo total de execução: " << tempoTotal << " ms" << endl;
    cout << "  Total de blocos no arquivo: " << bptree.contarBlocos() << "\n" << endl;

    return 0;
}
