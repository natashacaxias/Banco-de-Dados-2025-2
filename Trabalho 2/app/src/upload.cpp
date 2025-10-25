#include <bits/stdc++.h>
#include <cstring>
#include "../include/hashfile.h"
#include "../include/bptreefile.h"
#include "../include/common.h"

using namespace std;

// converte linha CSV → Registro fixo
Registro toRegistro(const RegistroCSV& csv) {
    Registro r{};
    r.id = csv.id;
    
    // Garante terminação nula em todos os campos
    strncpy(r.titulo.data(), csv.titulo.c_str(), sizeof(r.titulo) - 1);
    r.titulo[sizeof(r.titulo) - 1] = '\0';
    
    strncpy(r.ano, csv.ano.c_str(), sizeof(r.ano) - 1);
    r.ano[sizeof(r.ano) - 1] = '\0';
    
    strncpy(r.autores, csv.autores.c_str(), sizeof(r.autores) - 1);
    r.autores[sizeof(r.autores) - 1] = '\0';
    
    strncpy(r.citacoes, csv.citacoes.c_str(), sizeof(r.citacoes) - 1);
    r.citacoes[sizeof(r.citacoes) - 1] = '\0';
    
    strncpy(r.data_atualizacao, csv.data_atualizacao.c_str(), sizeof(r.data_atualizacao) - 1);
    r.data_atualizacao[sizeof(r.data_atualizacao) - 1] = '\0';
    
    strncpy(r.snippet, csv.snippet.c_str(), sizeof(r.snippet) - 1);
    r.snippet[sizeof(r.snippet) - 1] = '\0';
    
    r.prox = -1;
    return r;
}

// faz o parse da linha CSV separada por ';'
RegistroCSV parseCSV(const string &linha) {
    RegistroCSV reg{};
    stringstream ss(linha);
    string campo;
    vector<string> campos;

    while (getline(ss, campo, ';')) {
        if (!campo.empty() && campo.front() == '"')
            campo.erase(0, 1);
        if (!campo.empty() && campo.back() == '"')
            campo.pop_back();
        campos.push_back(campo);
    }

    if (campos.size() < 7) {
        reg.id = 0;
        return reg;
    }

    try { 
        reg.id = stoi(campos[0]); 
    } catch (...) { 
        reg.id = 0; 
        return reg;
    }
    
    // Trata campos vazios
    reg.titulo = campos[1].empty() ? "Sem Titulo" : campos[1];
    reg.ano = campos[2].empty() ? "0000" : campos[2];
    reg.autores = campos[3].empty() ? "Sem Autores" : campos[3];
    reg.citacoes = campos[4].empty() ? "0" : campos[4];
    reg.data_atualizacao = campos[5].empty() ? "0000-00-00" : campos[5];
    reg.snippet = campos[6].empty() ? "Sem Snippet" : campos[6];

    return reg;
}

int main(int argc, char* argv[]) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.setf(std::ios::unitbuf);

    if (argc < 2) {
        cerr << "Uso: ./bin/upload <arquivo.csv>\n";
        return 1;
    }

    string caminhoCSV = argv[1];
    ifstream arquivo(caminhoCSV);
    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir arquivo CSV: " << caminhoCSV << endl;
        return 1;
    }

    cout << "Iniciando upload interativo do arquivo: " << caminhoCSV << "\n";
    cout << "--------------------------------------------------------\n";

    HashFile hashFile("./data/data.db", NUM_BUCKETS, BUCKET_SIZE);
    hashFile.criarArquivoVazio();

    // Criação do arquivo de índice B+
    using idKey = int;
    using tituloKey = array<char,300>; 
    
    // Garante que o diretório existe
    system("mkdir -p ./data");
    
    fstream bptFileId("./data/bptreeId.idx", ios::in | ios::out | ios::binary | ios::trunc);
    if (!bptFileId.is_open()) {
        cerr << "Erro ao criar arquivo de índice B+: ./data/bptreeId.idx\n";
        return 1;
    }
    fstream bptFileTitulo("./data/bptreeTitulo.idx", ios::in | ios::out | ios::binary | ios::trunc);
    if (!bptFileTitulo.is_open()) {
        cerr << "Erro ao criar arquivo de índice B+: ./data/bptreeTitulo.idx\n";
        return 1;
    }

    bp<idKey, M_ID> bptreeId;
    bp<tituloKey, M_TITULO> bptreeTitulo;
    bptreeId.iniciar(&bptFileId);
    bptreeTitulo.iniciar(&bptFileTitulo);

    vector<Registro> buffer;
    buffer.reserve(BATCH_SIZE);

    long totalLinhas = 0, inseridos = 0, invalidos = 0;
    {
        ifstream temp(caminhoCSV);
        string linhaTemp;
        getline(temp, linhaTemp); // Pula header
        while (getline(temp, linhaTemp)) {
            if (!linhaTemp.empty()) totalLinhas++;
        }
    }
    cout << "Total de linhas de dados: " << totalLinhas << "\n\n";

    auto start = chrono::high_resolution_clock::now();
    string linha;

    // Pula header
    getline(arquivo, linha);

    cout << "Iniciando leitura e inserção..." << endl;

    while (getline(arquivo, linha)) {
        if (linha.empty()) continue;

        RegistroCSV csv = parseCSV(linha);
        if (csv.id == 0) { 
            invalidos++; 
            continue; 
        }

        buffer.push_back(toRegistro(csv));
        inseridos++;

        // Processa em lotes para eficiência
        if (buffer.size() >= BATCH_SIZE) {
            vector<loteReturn> indices = hashFile.inserirEmLote(buffer);

            for (loteReturn lr : indices) {
                if (lr.pos != static_cast<int64_t>(-1)) {
                    bptreeId.inserir(lr.id, lr.pos);
                    array<char,300> tituloArray{};
                    memset(tituloArray.data(), 0, 300);
                    strncpy(tituloArray.data(), lr.titulo.data(), 299);
                    tituloArray[299] = '\0';
                    bptreeTitulo.inserir(tituloArray, lr.pos);
                }
            }
            buffer.clear();
        }

        if (inseridos % PROGRESS_STEP == 0) {
            auto now = chrono::high_resolution_clock::now();
            double elapsed = chrono::duration<double>(now - start).count();
            double percent = (100.0 * inseridos) / totalLinhas;
            double speed = inseridos / elapsed;
            cout << fixed << setprecision(1);
            cout << "⏱" << setw(7) << elapsed << "s | "
                 << setw(8) << inseridos << "/" << totalLinhas
                 << " (" << percent << "%) — "
                 << (int)speed << " regs/s\n" << flush;
        }
    }

    // Processa último lote
    if (!buffer.empty()) {
        vector<loteReturn> indices = hashFile.inserirEmLote(buffer);
        for (loteReturn lr : indices) {
            if (lr.pos != static_cast<int64_t>(-1)) {
                bptreeId.inserir(lr.id, lr.pos);
                array<char,300> tituloArray{};
                memset(tituloArray.data(), 0, 300);            
                strncpy(tituloArray.data(), lr.titulo.data(), 299);
                tituloArray[299] = '\0';
                bptreeTitulo.inserir(tituloArray, lr.pos);
            }
        }
    }

    // Força escrita de tudo no disco
    bptreeId.flushCache(); 
    bptreeTitulo.flushCache();
    bptFileId.flush(); 
    bptFileTitulo.flush();
    bptFileId.close(); 
    bptFileTitulo.close();

    auto end = chrono::high_resolution_clock::now();
    double totalTime = chrono::duration<double>(end - start).count();

    cout << "\nUpload concluido!\n";
    cout << "Registros validos: " << inseridos
         << " | Invalidos: " << invalidos << "\n";
    cout << "Tempo total: " << fixed << setprecision(2)
         << totalTime << " s\n";
    if (totalTime > 0)
        cout << "Velocidade media: " << (int)(inseridos / totalTime)
             << " regs/s\n";

    cout << "Total de blocos no arquivo: "
         << hashFile.getTotalBlocos() << endl;

    cout << "Indices B+ salvos em: ./data/bptreeId.idx e ./data/bptreeTitulo.idx\n";
    cout << "--------------------------------------------------------\n";
    cout << "Arquivo principal salvo em: ./data/data.db\n";
    
    return 0;
}