#include<bits/stdc++.h>
#include <chrono>
#include <fstream>
#include "../include/hashfile.h"
#include "../include/bptreefile.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "=== TP2 – Busca por id (índice primário) (seek1) ===" << endl;

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
    const int M = 64;  // Grau da árvore B+

    HashFile hashFile(dbPath, NUM_BUCKETS, BUCKET_SIZE);
    Registro r;

    auto inicioTotal = chrono::high_resolution_clock::now();

    int id = stoi(chaveStr);
    
    // Abre arquivo de índice B+ para IDs
    fstream bptFileId("./data/bptreeId.idx", ios::in | ios::out | ios::binary);
    if (!bptFileId.is_open()) {
        cerr << "Erro ao abrir arquivo de índice B+ para IDs: ./data/bptreeId.idx" << endl;
        return 1;
    }

    bp<int, M> bptreeId;
    bptreeId.carregarArvore(&bptFileId);

    // Busca no índice B+
    cout << "\nBuscando no índice B+ por ID..." << endl;
    auto inicioBusca = chrono::high_resolution_clock::now();
    
    pair<ptr, int> resultado = bptreeId.buscar(id);
    
    auto fimBusca = chrono::high_resolution_clock::now();
    double tempoBuscaIdx = chrono::duration<double, milli>(fimBusca - inicioBusca).count();

    if (resultado.first != -1) {
        cout << "Chave encontrada no índice B+" << endl;
        cout << "Estatísticas do índice:" << endl;
        cout << "  Blocos lidos no índice: " << resultado.second << endl;
        cout << "  Tempo de busca no índice: " << tempoBuscaIdx << " ms" << endl;

        // Agora busca o registro completo no arquivo de dados
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
            cout << "\n❌ Registro nao encontrado no arquivo de dados (inconsistência).\n";
        }

        cout << "\nEstatisticas da busca em dados:" << endl;
        cout << "  Blocos lidos nos dados: " << hashFile.getBlocosLidos() << endl;
        cout << "  Tempo de busca nos dados: " << tempoBuscaDados << " ms" << endl;

    } else {
        cout << "\nChave nao encontrada no indice B+." << endl;
        cout << "Estatisticas do indice:" << endl;
        cout << "  Blocos lidos no indice: " << resultado.second << endl;
        cout << "  Tempo de busca no indice: " << tempoBuscaIdx << " ms" << endl;
    }

    bptFileId.close();

    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    cout << "\nEstatísticas Gerais:" << endl;
    cout << fixed << setprecision(2);
    cout << "Tempo total de execucao: " << tempoTotal << " ms" << endl;
    cout << "Total de blocos no arquivo: " << hashFile.getTotalBlocos() << endl;

    return 0;
}