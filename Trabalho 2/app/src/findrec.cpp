#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include "../include/hashfile.h"

using namespace std;

int main(int argc, char* argv[]) {
    cout << "=== TP2 – Busca Direta (findrec) ===" << endl;

    if (argc < 2) {
        cerr << "Uso: ./findrec <id>" << endl;
        return 1;
    }

    int id = stoi(argv[1]);
    string dbPath = "/data/data.db";

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Procurando registro com ID = " << id << endl;

    HashFile hashFile(dbPath, 100, 4);
    Registro r;

    auto inicio = chrono::high_resolution_clock::now();
    bool encontrado = hashFile.buscar(id, r);
    auto fim = chrono::high_resolution_clock::now();

    double tempoMs = chrono::duration<double, milli>(fim - inicio).count();
    int blocos = hashFile.getBlocosLidos();

    if (encontrado) {
        cout << "\n✅ Registro encontrado:" << endl;
        cout << "ID: " << r.id << endl;
        cout << "Título: " << r.titulo << endl;
        cout << "Ano: " << r.ano << endl;
        cout << "Autores: " << r.autores << endl;
        cout << "Citações: " << r.citacoes << endl;
        cout << "Atualização: " << r.data_atualizacao << endl;
        cout << "Snippet: " << r.snippet << endl;
    } else {
        cout << "\n❌ Registro não encontrado." << endl;
    }

    cout << "\n📊 Estatísticas:" << endl;
    cout << "Blocos lidos: " << blocos << endl;
    cout << fixed << setprecision(2);
    cout << "Tempo de execução: " << tempoMs << " ms" << endl;

    return 0;
}
