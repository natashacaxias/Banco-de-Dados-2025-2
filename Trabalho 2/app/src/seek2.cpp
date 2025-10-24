#include <bits/stdc++.h>
#include <iomanip>
#include <chrono>
#include <fstream>
#include "../include/hashfile.h"
#include "../include/bptreefile.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "=== TP2 – Busca por título (índice secundário) (seek2) ===" << endl;

    if (argc < 2) {
        cerr << "Uso: ./bin/seek2 <chave>" << endl;
        return 1;
    }

    string chaveStr = argv[1];
    string dbPath = "data/data.db";

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Chave: " << chaveStr << endl;

    const int NUM_BUCKETS = 97;
    const int BUCKET_SIZE = 4096;
    const int M = 64;  // Grau da árvore B+

    HashFile hashFile(dbPath, NUM_BUCKETS, BUCKET_SIZE);
    Registro r;

    auto inicioTotal = chrono::high_resolution_clock::now();

    // === Inicializa Char300Wrapper corretamente ===
    Char300Wrapper tituloWrapper(chaveStr.c_str());

    // === Abre arquivo de índice B+ para títulos ===
    fstream bptFileTitulo("./data/bptreeTitulo.idx", ios::in | ios::out | ios::binary);
    if (!bptFileTitulo.is_open()) {
        cerr << "Erro ao abrir arquivo de índice B+ para títulos: ./data/bptreeTitulo.idx" << endl;
        return 1;
    }

    // === Inicializa árvore B+ para títulos ===
    bp<Char300Wrapper, M> bptreeTitulo;
    bptreeTitulo.carregarArvore(&bptFileTitulo);

    // === Busca no índice B+ ===
    cout << "\nBuscando no índice B+ por Título..." << endl;
    auto inicioBusca = chrono::high_resolution_clock::now();
    pair<long long, int> resultado = bptreeTitulo.buscar(tituloWrapper);
    auto fimBusca = chrono::high_resolution_clock::now();
    double tempoBuscaIdx = chrono::duration<double, milli>(fimBusca - inicioBusca).count();

    if (resultado.first != -1) {
        cout << "Chave encontrada no índice B+" << endl;
        cout << "Estatísticas do índice:" << endl;
        cout << "  Blocos lidos no índice: " << resultado.second << endl;
        cout << "  Tempo de busca no índice: " << tempoBuscaIdx << " ms" << endl;

        // === Recupera registro do arquivo de dados usando a posição retornada pelo índice ===
        cout << "\nRecuperando registro do arquivo de dados..." << endl;

        fstream dataFile(dbPath, ios::in | ios::binary);
        if (!dataFile.is_open()) {
            cerr << "Erro ao abrir arquivo de dados." << endl;
            return 1;
        }

        auto inicioDados = chrono::high_resolution_clock::now();
        dataFile.seekg(resultado.first);
        dataFile.read(reinterpret_cast<char*>(&r), sizeof(Registro));
        auto fimDados = chrono::high_resolution_clock::now();
        double tempoBuscaDados = chrono::duration<double, milli>(fimDados - inicioDados).count();

        if (dataFile) {
            cout << "\nRegistro encontrado:\n";
            cout << "ID: " << r.id << "\n";
            cout << "Título: " << r.titulo << "\n";
            cout << "Ano: " << r.ano << "\n";
            cout << "Autores: " << r.autores << "\n";
            cout << "Citações: " << r.citacoes << "\n";
            cout << "Atualização: " << r.data_atualizacao << "\n";
            cout << "Snippet: " << r.snippet << "\n";
        } else {
            cout << "\nErro ao ler registro do arquivo de dados.\n";
        }

        dataFile.close();

        cout << "\nEstatísticas da busca em dados:" << endl;
        cout << "  Tempo de busca nos dados: " << tempoBuscaDados << " ms" << endl;

    } else {
        cout << "\nTítulo não encontrado no índice B+." << endl;
        cout << "Estatísticas do índice:" << endl;
        cout << "  Blocos lidos no índice: " << resultado.second << endl;
        cout << "  Tempo de busca no índice: " << tempoBuscaIdx << " ms" << endl;
    }

    bptFileTitulo.close();

    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    cout << "\nEstatísticas Gerais:" << endl;
    cout << fixed << setprecision(2);
    cout << "Tempo total de execução: " << tempoTotal << " ms" << endl;
    cout << "Total de blocos no arquivo: " << hashFile.getTotalBlocos() << endl;

    return 0;
}