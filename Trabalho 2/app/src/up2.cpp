#include <bits/stdc++.h>
#include "../include/bp2.h"
using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso: ./upload <arquivo.csv>\n";
        return 1;
    }

    string inputCsv = argv[1];
    string dataFile = "data/data.db";
    string indexFile = "data/idx_primario.bpt";


    cout << "=== TP2 – Upload com B+Tree ===\n";
    cout << "Arquivo de entrada: " << inputCsv << endl;
    cout << "Arquivo de dados: " << dataFile << endl;
    cout << "Arquivo de índice: " << indexFile << endl;

    // Abre CSV
    ifstream fin(inputCsv);
    if (!fin.is_open()) {
        cerr << "❌ Erro ao abrir CSV.\n";
        return 1;
    }

    // Abre arquivo binário de dados
    fstream fout(dataFile, ios::out | ios::binary | ios::trunc);
    if (!fout.is_open()) {
        cerr << "❌ Erro ao criar arquivo de dados.\n";
        return 1;
    }

    // Inicializa árvore B+
    BpTree<int> idx;
    if (!idx.open(indexFile)) {
        cerr << "❌ Erro ao abrir índice.\n";
        return 1;
    }
    idx.iniciar();

    string linha;
    long count = 0;
    auto start = chrono::high_resolution_clock::now();

    // Lê CSV linha a linha
    while (getline(fin, linha)) {
        if (linha.empty()) continue;
        stringstream ss(linha);
        Registro r{};
        string campo;

        getline(ss, campo, ',');
        r.id = stoi(campo);

        getline(ss, campo, ',');
        strncpy(r.titulo, campo.c_str(), sizeof(r.titulo));

        getline(ss, campo, ',');
        r.ano = campo.empty() ? 0 : stoi(campo);

        getline(ss, campo, ',');
        strncpy(r.autores, campo.c_str(), sizeof(r.autores));

        getline(ss, campo, ',');
        r.citacoes = campo.empty() ? 0 : stoi(campo);

        getline(ss, campo, ',');
        strncpy(r.datahora, campo.c_str(), sizeof(r.datahora));

        getline(ss, campo, ',');
        strncpy(r.snippet, campo.c_str(), sizeof(r.snippet));

        // Grava posição atual no arquivo de dados
        ptr pos = fout.tellp();
        fout.write(reinterpret_cast<char*>(&r), sizeof(Registro));

        // Insere no índice (ID → posição)
        idx.inserir(r.id, pos);
        count++;
    }

    idx.flush();
    fout.close();
    fin.close();

    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "✅ Upload concluído!\n";
    cout << "Registros inseridos: " << count << "\n";
    cout << "Tempo total: " << ms << " ms\n";
    cout << "Blocos lidos: " << idx.blocosLidos
         << " | escritos: " << idx.blocosEscritos << "\n";
    cout << "Arquivo de índice salvo em: " << indexFile << "\n";

    return 0;
}
