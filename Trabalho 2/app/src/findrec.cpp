#include<bits/stdc++.h>
#include <chrono>
#include <fstream>
#include "../include/hashfile.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "=== TP2 – Busca por id (findrec) ===" << endl;

    if (argc < 1) {
        cerr << "Uso: ./bin/seek1 <chave>" << endl;
        return 1;
    }

    string chaveStr = argv[1];
    string dbPath = "data/data.db";

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Chave: " << chaveStr << endl;

    // ⚠️ Deve bater com o que foi usado no upload
    const int NUM_BUCKETS = 97;
    const int BUCKET_SIZE = 4096;

    HashFile hashFile(dbPath, NUM_BUCKETS, BUCKET_SIZE);
    Registro r;

    auto inicioTotal = chrono::high_resolution_clock::now();

    int id = stoi(chaveStr);

    // Busca o registro no arquivo de dados
    cout << "\nRecuperando registro do arquivo de dados..." << endl;
    auto inicioDados = chrono::high_resolution_clock::now();
    
    bool encontrado = hashFile.buscar(id, r);
    
    auto fimDados = chrono::high_resolution_clock::now();
    double tempoBuscaDados = chrono::duration<double, milli>(fimDados - inicioDados).count();

    if (encontrado) {
        cout << "\nRegistro encontrado:\n";
        cout << "ID: " << r.id << "\n";
        cout << "Titulo: " << r.titulo << "\n";
        cout << "Ano: " << r.ano << "\n";
        cout << "Autores: " << r.autores << "\n";
        cout << "Citacoes: " << r.citacoes << "\n";
        cout << "Atualizacao: " << r.data_atualizacao << "\n";
        cout << "Snippet: " << r.snippet << "\n";
    } else {
        cout << "\nRegistro nao encontrado no arquivo de dados.\n";
    }

    cout << "\nEstatisticas da busca em dados:" << endl;
    cout << "  Blocos lidos nos dados: " << hashFile.getBlocosLidos() << endl;
    cout << "  Tempo de busca nos dados: " << tempoBuscaDados << " ms" << endl;

    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    cout << "\nEstatísticas Gerais:" << endl;
    cout << fixed << setprecision(2);
    cout << "Tempo total de execucao: " << tempoTotal << " ms" << endl;
    cout << "Total de blocos no arquivo: " << hashFile.getTotalBlocos() << endl;

    return 0;
}