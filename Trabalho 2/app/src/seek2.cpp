#include <bits/stdc++.h>
#include <chrono>
#include <fstream>
#include "../include/bptreefile.h"
#include "../include/common.h"

using namespace std;

int main(int argc, char* argv[]) {
    auto inicioTotal = chrono::high_resolution_clock::now();

    cout << "\n === Busca por titulo (seek2) === \n " << endl;

    // verifica se foi passada a chave (título)
    if (argc < 2) {
        cerr << "Uso: ./bin/seek2 <chave>" << endl;
        return 1;
    }

    string chaveStr = argv[1];
    string dbPath = "/data/data.db";
    string idxPath = "/data/bptreeTitulo.idx";

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Arquivo de índices: " << idxPath << endl;

    cout << "Chave: " << chaveStr << endl;

    // abre o índice B+ (secundário, por título)
    fstream bptFile(idxPath, ios::in | ios::out | ios::binary);
    if (!bptFile.is_open()) {
        cerr << "Erro ao abrir arquivo de indice B+ para Titulo: " << idxPath << endl;
        return 1;
    }

    // abre o arquivo de dados principal
    fstream db(dbPath, ios::in | ios::out | ios::binary);
    if (!db.is_open()) {
        cerr << "Erro ao abrir arquivo de dados: " << dbPath << endl;
        return 1;
    }

    // define tipo da chave (array fixo de 300 chars)
    using Chave = array<char,300>;
    bp<Chave, M_TITULO> bptree;
    bptree.carregarArvore(&bptFile);
    Registro r;

    // prepara a chave para busca 
    Chave chave{};
    memset(chave.data(), 0, sizeof(Chave));
    strncpy(chave.data(), chaveStr.c_str(), sizeof(Chave));


    // realiza a busca no índice
    cout << "\nRecuperando registro do arquivo de dados..." << endl;
    auto inicioDados = chrono::high_resolution_clock::now();
    
    pair<bool, long> res = bptree.buscar(chave, r, &db);
    
    auto fimDados = chrono::high_resolution_clock::now();
    double tempoBuscaDados = chrono::duration<double, milli>(fimDados - inicioDados).count();

    // exibe o resultado encontrado
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

    // mostra estatísticas da busca
    cout << "\nEstatisticas da busca em dados:" << endl;
    cout << "Blocos lidos nos dados: " << res.second << endl;
    cout << "Tempo de busca nos dados: " << tempoBuscaDados << " ms" << endl;
    cout << "Total de blocos: " << bptree.contarBlocos() << "\n";

    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    cout << "\nEstatísticas Gerais:" << endl;
    cout << fixed << setprecision(2);
    cout << "Tempo total de execucao: " << tempoTotal << " ms" << endl;
    cout << "Total de blocos no arquivo: " << bptree.contarBlocos() << "\n\n";

    return 0;
}
