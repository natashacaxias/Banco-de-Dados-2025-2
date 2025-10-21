#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include "../include/hashfile.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "=== TP2 – Busca Direta (findrec) ===" << endl;

    if (argc < 2) {
        cerr << "Uso: ./bin/findrec <id>" << endl;
        return 1;
    }

    int id = stoi(argv[1]);
    string dbPath = "/data/data.db";

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Procurando registro com ID = " << id << endl;

    // ⚠️ Deve bater com o que foi usado no upload
    const int NUM_BUCKETS = 97;
    const int BUCKET_SIZE = 4096;

    HashFile hashFile(dbPath, NUM_BUCKETS, BUCKET_SIZE);
    Registro r;

    auto inicio = chrono::high_resolution_clock::now();
    bool encontrado = hashFile.buscar(id, r);
    auto fim = chrono::high_resolution_clock::now();

    double tempoMs = chrono::duration<double, milli>(fim - inicio).count();

    if (encontrado) {
        cout << "\n✅ Registro encontrado:\n";
        cout << "ID: " << r.id << "\n";
        cout << "Título: " << r.titulo << "\n";
        cout << "Ano: " << r.ano << "\n";
        cout << "Autores: " << r.autores << "\n";
        cout << "Citações: " << r.citacoes << "\n";
        cout << "Atualização: " << r.data_atualizacao << "\n";
        cout << "Snippet: " << r.snippet << "\n";
    } else {
        cout << "\n❌ Registro não encontrado.\n";
    }

    cout << "\n📊 Estatísticas:\n";
    cout << fixed << setprecision(2);
    cout << "Tempo de execução: " << tempoMs << " ms\n";
    cout << "Blocos lidos (registros): " << hashFile.getBlocosLidos() << "\n";
    cout << "Total de blocos no arquivo: " << hashFile.getTotalBlocos() << "\n";

    return 0;
}
