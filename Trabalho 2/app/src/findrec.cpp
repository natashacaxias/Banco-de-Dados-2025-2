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
using namespace std;

// ============================================================
// Função principal
// ------------------------------------------------------------
// Espera um argumento na linha de comando:
//   ./bin/findrec <ID>
// ============================================================
int main(int argc, char* argv[]) {

    // Marca o início da execução total
    auto inicioTotal = chrono::high_resolution_clock::now();

    cout << "\n=== TP2 – Busca por ID (findrec) ===\n" << endl;

    // --------------------------------------------------------
    // Validação dos argumentos
    // --------------------------------------------------------
    // Verifica se o usuário forneceu o ID na linha de comando.
    // Caso contrário, mostra a forma correta de uso.
    if (argc < 2) {
        cerr << "Uso: ./bin/findrec <ID>" << endl;
        return 1;
    }

    // --------------------------------------------------------
    // Inicialização de variáveis e caminhos
    // --------------------------------------------------------
    string chaveStr = argv[1];               // ID passado como string
    string dbPath = "/data/data.db";         // caminho do arquivo de dados no contêiner

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Chave: " << chaveStr << endl;

    // --------------------------------------------------------
    // Criação do manipulador do arquivo hash
    // --------------------------------------------------------
    // O objeto HashFile encapsula o acesso ao arquivo de dados
    // organizado por hashing, oferecendo operações de busca
    // e métricas de desempenho.
    HashFile hashFile(dbPath, NUM_BUCKETS, BUCKET_SIZE);
    Registro r; // estrutura onde o registro encontrado será armazenado

    // Converte a chave de string para inteiro (ID numérico)
    int id = stoi(chaveStr);

    // ========================================================
    // Etapa 1: Busca direta no arquivo de dados
    // ========================================================
    cout << "\nRecuperando registro do arquivo de dados..." << endl;
    auto inicioDados = chrono::high_resolution_clock::now();

    // Executa a busca no arquivo de dados usando hashing
    bool encontrado = hashFile.buscar(id, r);

    auto fimDados = chrono::high_resolution_clock::now();
    double tempoBuscaDados = chrono::duration<double, milli>(fimDados - inicioDados).count();

    // --------------------------------------------------------
    // Exibição do resultado da busca
    // --------------------------------------------------------
    if (encontrado) {
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
    // Etapa 2: Exibição das estatísticas de desempenho
    // ========================================================
    cout << "\nEstatísticas da busca em dados:" << endl;
    cout << "  Blocos lidos nos dados: " << hashFile.getBlocosLidos() << endl;
    cout << "  Tempo de busca nos dados: " << tempoBuscaDados << " ms" << endl;

    // ========================================================
    // Etapa 3: Estatísticas gerais e encerramento
    // ========================================================
    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    cout << "\nEstatísticas gerais:" << endl;
    cout << fixed << setprecision(2);
    cout << "  Tempo total de execução: " << tempoTotal << " ms" << endl;
    cout << "  Total de blocos no arquivo: " << hashFile.getTotalBlocos() << "\n" << endl;

    return 0;
}
