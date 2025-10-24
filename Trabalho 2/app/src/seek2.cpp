#include <bits/stdc++.h>
#include <chrono>
#include <fstream>
#include "../include/bptreefile.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "=== TP2 - Busca por titulo (seek2) ===" << endl;

    if (argc < 2) {
        cerr << "Uso: ./bin/seek2 <chave>" << endl;
        return 1;
    }

    string chaveStr = argv[1];
    string dbPath = "data/data.db";
    string idxPath = "data/bptreeTitulo.idx";

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Chave: " << chaveStr << endl;

    // Abre arquivo de índice B+
    fstream bptFile(idxPath, ios::in | ios::out | ios::binary);
    if (!bptFile.is_open()) {
        cerr << "Erro ao abrir arquivo de índice B+ para Titulo: " << idxPath << endl;
        return 1;
    }

    // Abre arquivo de dados
    fstream db(dbPath, ios::in | ios::out | ios::binary);
    if (!db.is_open()) { // corrigido de bptFile para db
        cerr << "Erro ao abrir arquivo de dados: " << dbPath << endl;
        return 1;
    }

    // ⚠️ Deve bater com o que foi usado no upload
    const int M = 64;

    // Define chave como array<char,300>
    using Chave = array<char,300>;
    bp<Chave, M> bptree;
    bptree.carregarArvore(&bptFile);
    Registro r;

    // Preenche array<char,300> com a chave
    Chave chave{};
    memset(chave.data(), 0, chave.size()); // Zera todo o array
    strncpy(chave.data(), chaveStr.c_str(), chave.size() - 1);
    chave[chave.size() - 1] = '\0'; // Garante terminação

    // Busca o registro no arquivo de dados
    cout << "\nRecuperando registro do arquivo de dados..." << endl;
    auto inicioDados = chrono::high_resolution_clock::now();
    
    pair<bool, long> res = bptree.buscar(chave, r, &db);
    
    auto fimDados = chrono::high_resolution_clock::now();
    double tempoBuscaDados = chrono::duration<double, milli>(fimDados - inicioDados).count();

    if (res.first) {
        cout << "\nRegistro encontrado:\n";
        cout << "ID: " << r.id << "\n";
        cout << "Titulo: " << string(r.titulo.data()) << "\n";
        cout << "Ano: " << string(r.ano) << "\n";
        cout << "Autores: " << string(r.autores) << "\n";
        cout << "Citacoes: " << string(r.citacoes) << "\n";
        cout << "Atualizacao: " << string(r.data_atualizacao) << "\n";
        cout << "Snippet: " << string(r.snippet) << "\n";
    } else {
        cout << "\nRegistro nao encontrado no arquivo de dados.\n";
    }

    cout << "\nEstatisticas da busca em dados:" << endl;
    cout << "Blocos lidos nos dados: " << res.second << endl;
    cout << "Tempo de busca nos dados: " << tempoBuscaDados << " ms" << endl;

    cout << "\nEstatisticas Gerais:" << endl;
    cout << fixed << setprecision(2);

    return 0;
}
