#include <bits/stdc++.h>
#include <chrono>
#include <fstream>
#include "../include/hashfile.h"
#include "../include/common.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "=== TP2 – Busca por id (findrec) ===" << endl;

    // verifica se o usuário passou o argumento (ID)
    if (argc < 2) {
        cerr << "Uso: ./bin/findrec.exe <chave>" << endl;
        return 1;
    }

    string chaveStr = argv[1];
    string dbPath = "/data/data.db"; // caminho do arquivo de dados

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Chave: " << chaveStr << endl;

    // cria o objeto responsável pelo arquivo hash
    HashFile hashFile(dbPath, NUM_BUCKETS, BUCKET_SIZE);
    Registro r;

    auto inicioTotal = chrono::high_resolution_clock::now();
    int id = stoi(chaveStr); // converte argumento para inteiro

    // inicia a busca no arquivo hash
    cout << "\nRecuperando registro do arquivo de dados..." << endl;
    auto inicioDados = chrono::high_resolution_clock::now();
    
    bool encontrado = hashFile.buscar(id, r); // realiza a busca
    
    auto fimDados = chrono::high_resolution_clock::now();
    double tempoBuscaDados = chrono::duration<double, milli>(fimDados - inicioDados).count();

    // mostra resultado da busca
    if (encontrado) {
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

    // mostra estatísticas da busca
    cout << "\nEstatisticas da busca em dados:" << endl;
    cout << "  Blocos lidos nos dados: " << hashFile.getBlocosLidos() << endl;
    cout << "  Tempo de busca nos dados: " << tempoBuscaDados << " ms" << endl;

    // calcula o tempo total da execução
    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    cout << "\nEstatísticas Gerais:" << endl;
    cout << fixed << setprecision(2);
    cout << "Tempo total de execucao: " << tempoTotal << " ms" << endl;
    cout << "Total de blocos no arquivo: " << hashFile.getTotalBlocos() << endl;

    return 0;
}
