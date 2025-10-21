#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <vector>
#include "../include/hashfile.h"

using namespace std;

// ============================================================
// Função auxiliar: divide uma linha CSV respeitando aspas
// ============================================================
vector<string> splitCSV(const string& line, char delimiter) {
    vector<string> campos;
    string campo;
    bool dentroAspas = false;

    for (char c : line) {
        if (c == '"') {
            dentroAspas = !dentroAspas;
        } else if (c == delimiter && !dentroAspas) {
            campos.push_back(campo);
            campo.clear();
        } else {
            campo += c;
        }
    }
    campos.push_back(campo);

    // Limpa aspas e \r
    for (auto& f : campos) {
        if (!f.empty() && f.front() == '"') f.erase(0, 1);
        if (!f.empty() && f.back() == '"')  f.pop_back();
        if (!f.empty() && f.back() == '\r') f.pop_back();
    }

    return campos;
}

// ============================================================
// Programa principal: lê o CSV e popula o arquivo hash
// ============================================================
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso: ./upload <arquivo.csv> ou <arquivo.csv.gz>" << endl;
        return 1;
    }

    string inputPath = argv[1];
    string dbPath = "/data/data.db";

    cout << "=== TP2 – Upload de Dados com Hashing ===" << endl;
    cout << "Arquivo de entrada: " << inputPath << endl;
    cout << "Arquivo de dados: " << dbPath << endl;

    HashFile hashFile(dbPath, 100, 4);
    if (!hashFile.criarArquivoVazio()) {
        cerr << "Erro ao criar arquivo de dados." << endl;
        return 1;
    }

    FILE* fp = nullptr;
    if (inputPath.find(".gz") != string::npos)
        fp = popen(("gzip -dc " + inputPath).c_str(), "r");
    else
        fp = fopen(inputPath.c_str(), "r");

    if (!fp) {
        cerr << "Erro ao abrir arquivo: " << inputPath << endl;
        return 1;
    }

    char buffer[16384];
    string linha;
    int count = 0;

    // intervalo de debug configurável via variável de ambiente
    const char* stepEnv = getenv("DEBUG_EVERY");
    int STEP = stepEnv ? max(1, atoi(stepEnv)) : 10000;

    auto inicio = chrono::high_resolution_clock::now();

    while (fgets(buffer, sizeof(buffer), fp)) {
        linha = buffer;
        if (linha.empty()) continue;

        char delim = (linha.find(';') != string::npos) ? ';' : ',';
        auto campos = splitCSV(linha, delim);
        if (campos.size() < 7) continue;

        Registro r;
        memset(&r, 0, sizeof(Registro));

        try { r.id = stoi(campos[0]); } catch (...) { r.id = -1; }
        try { r.ano = stoi(campos[2]); } catch (...) { r.ano = 0; }
        try { r.citacoes = stoi(campos[4]); } catch (...) { r.citacoes = 0; }

        strncpy(r.titulo, campos[1].c_str(), sizeof(r.titulo) - 1);
        strncpy(r.autores, campos[3].c_str(), sizeof(r.autores) - 1);
        strncpy(r.data_atualizacao, campos[5].c_str(), sizeof(r.data_atualizacao) - 1);
        strncpy(r.snippet, campos[6].c_str(), sizeof(r.snippet) - 1);

        r.prox = -1;
        hashFile.inserir(r);
        count++;

        // Log progressivo
        if (count % STEP == 0) {
            cout << "Inseridos: " << count << " registros..." << endl;
        }
    }

    pclose(fp);

    auto fim = chrono::high_resolution_clock::now();
    double tempoMs = chrono::duration<double, milli>(fim - inicio).count();

    cout << "\nUpload concluído!" << endl;
    cout << "Registros inseridos: " << count << endl;
    cout << fixed << setprecision(2);
    cout << "Tempo total: " << tempoMs << " ms" << endl;
    cout << "Arquivo salvo em: " << dbPath << endl;

    return 0;
}
