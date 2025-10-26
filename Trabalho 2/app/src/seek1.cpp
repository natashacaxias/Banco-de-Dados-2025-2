#include <bits/stdc++.h>
#include <chrono>
#include <fstream>
#include "../include/bptreefile.h"
#include "../include/common.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "=== TP2 - Busca por id (seek1) ===" << endl;

    // verifica se foi passada a chave (ID)
    if (argc < 2) {
        cerr << "Uso: ./bin/seek1 <chave>" << endl;
        return 1;
    }

    string chaveStr = argv[1];
    string dbPath = "/data/data.db";
    string idxPath = "/data/bptreeId.idx";

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Chave: " << chaveStr << endl;

    // abre o arquivo de índice B+ (índice primário por ID)
    fstream bptFile("/data/bptreeId.idx", ios::in | ios::out | ios::binary);
    if (!bptFile.is_open()) {
        cerr << "Erro ao abrir arquivo de índice B+ para IDs: /data/bptreeId.idx" << endl;
        return 1;
    }

    // abre o arquivo de dados principal
    fstream db("/data/data.db", ios::in | ios::out | ios::binary);
    if (!db.is_open()) {
        cerr << "Erro ao abrir arquivo de dados: /data/data.db" << endl;
        return 1;
    }

    // carrega a árvore B+ a partir do arquivo existente
    bp<int, M_ID> bptree;
    bptree.carregarArvore(&bptFile);
    Registro r;

    int id = stoi(chaveStr); // converte argumento para inteiro

    // executa a busca usando o índice B+ (acessa o arquivo de dados pelo ponteiro)
    cout << "\nRecuperando registro do arquivo de dados..." << endl;
    auto inicioDados = chrono::high_resolution_clock::now();
    
    pair<bool, long> res = bptree.buscar(id, r, &db);
    
    auto fimDados = chrono::high_resolution_clock::now();
    double tempoBuscaDados = chrono::duration<double, milli>(fimDados - inicioDados).count();

    // mostra o resultado da busca
    if (res.first) {
        cout << "\nRegistro encontrado:\n";
        cout << "ID: " << r.id << "\n";
        cout << "Titulo: " << r.titulo.data() << "\n";
        cout << "Ano: " << r.ano << "\n";
        cout << "Autores: " << r.autores << "\n";
        cout << "Citacoes: " << r.citacoes << "\n";
        cout << "Atualizacao: " << r.data_atualizacao << "\n";
        cout << "Snippet: " << r.snippet << "\n";
    } else {
        cout << "\nRegistro nao encontrado no arquivo de dados.\n";
    }

    // imprime estatísticas da operação
    cout << "\nEstatisticas da busca em dados:" << endl;
    cout << "Blocos lidos nos dados: " << res.second << endl;
    cout << "Tempo de busca nos dados: " << tempoBuscaDados << " ms" << endl;
    cout << "Total de blocos no arquivo: " << bptree.contarBlocos() << endl;

    return 0;
}
