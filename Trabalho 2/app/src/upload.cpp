// ============================================================
// upload.cpp
// ------------------------------------------------------------
// Programa responsável por realizar a carga inicial dos dados
// (upload) a partir de um arquivo CSV. 
//
// Ele cria:
//   • O arquivo de dados principal organizado por hashing;
//   • O índice primário (B+Tree por ID);
//   • O índice secundário (B+Tree por Título).
//
// O programa lê o CSV linha a linha, converte os registros
// para formato fixo e insere em disco, atualizando também
// os índices em memória secundária.
// ============================================================

#include <bits/stdc++.h>
#include <cstring>
#include "../include/hashfile.h"
#include "../include/bptreefile.h"
#include "../include/common.h"
#include "../include/logger.h"
using namespace std;

// ============================================================
// Função auxiliar: Converte RegistroCSV → Registro fixo
// ------------------------------------------------------------
// Transforma os campos variáveis lidos do CSV em um registro
// de tamanho fixo, adequado para gravação binária em disco.
// ============================================================
Registro toRegistro(const RegistroCSV& csv) {
    Registro r;
    memset(&r, 0, sizeof(Registro)); // zera a estrutura inteira

    r.id = csv.id;
    strncpy(r.titulo.data(), csv.titulo.c_str(), sizeof(r.titulo) - 1);
    strncpy(r.ano, csv.ano.c_str(), sizeof(r.ano) - 1);
    strncpy(r.autores, csv.autores.c_str(), sizeof(r.autores) - 1);
    strncpy(r.citacoes, csv.citacoes.c_str(), sizeof(r.citacoes) - 1);
    strncpy(r.data_atualizacao, csv.data_atualizacao.c_str(), sizeof(r.data_atualizacao) - 1);
    strncpy(r.snippet, csv.snippet.c_str(), sizeof(r.snippet) - 1);
    r.prox = -1; // inicializa o encadeamento

    return r;
}

// ============================================================
// Função auxiliar: parseCSV()
// ------------------------------------------------------------
// Lê uma linha do arquivo CSV, remove caracteres indesejados
// (aspas, BOM UTF-8, CRLF) e separa os campos pelo delimitador
// ';'. Retorna um RegistroCSV populado com os valores.
// ============================================================
RegistroCSV parseCSV(const string &linhaOriginal) {
    RegistroCSV reg{};
    if (linhaOriginal.empty()) {
        reg.id = 0;
        return reg;
    }

    string linha = linhaOriginal;

    // Remove BOM UTF-8 (arquivos exportados do Windows)
    if (linha.size() >= 3 &&
        (unsigned char)linha[0] == 0xEF &&
        (unsigned char)linha[1] == 0xBB &&
        (unsigned char)linha[2] == 0xBF) {
        linha.erase(0, 3);
    }

    // Remove \r no final (caso CRLF)
    if (!linha.empty() && linha.back() == '\r') linha.pop_back();

    // Divide a linha pelos ';'
    stringstream ss(linha);
    string campo;
    vector<string> campos;
    while (getline(ss, campo, ';')) {
        if (!campo.empty() && campo.front() == '"') campo.erase(0, 1);
        if (!campo.empty() && campo.back() == '"') campo.pop_back();
        campos.push_back(campo);
    }

    // Verifica quantidade mínima de campos
    if (campos.size() < 7) {
        reg.id = 0;
        return reg;
    }

    // Converte o ID para inteiro
    try {
        reg.id = stoi(campos[0]);
    } catch (...) {
        reg.id = 0;
        return reg;
    }

    // Preenche campos vazios com valores padrão
    reg.titulo           = campos[1].empty() ? "Sem Titulo"      : campos[1];
    reg.ano              = campos[2].empty() ? "0000"            : campos[2];
    reg.autores          = campos[3].empty() ? "Sem Autores"     : campos[3];
    reg.citacoes         = campos[4].empty() ? "0"               : campos[4];
    reg.data_atualizacao = campos[5].empty() ? "0000-00-00"      : campos[5];
    reg.snippet          = campos[6].empty() ? "Sem Snippet"     : campos[6];

    return reg;
}

// ============================================================
// Função principal
// ------------------------------------------------------------
// Espera como argumento na linha de comando:
//   ./bin/upload <arquivo.csv>
//
// Exemplo de execução dentro do container:
//   docker run --rm -v $(pwd)/data:/data tp2 ./bin/upload /data/artigo.csv
// ============================================================
int main(int argc, char* argv[]) {
    Logger log; //  instancia o logger
    auto inicioTotal = chrono::high_resolution_clock::now();

    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.setf(std::ios::unitbuf);

    // --------------------------------------------------------
    // Validação dos argumentos
    // --------------------------------------------------------
    if (argc < 2) {
        log.error("Uso: ./bin/upload <arquivo.csv>");
        return 1;
    }

    string caminhoCSV = argv[1];
    ifstream arquivo(caminhoCSV);
    if (!arquivo.is_open()) {
        log.error("Erro ao abrir arquivo CSV: " + caminhoCSV);
        return 1;
    }

    log.info("=== TP2 – Upload de Dados ===");
    log.info("Iniciando upload do arquivo: " + caminhoCSV);
    log.info("--------------------------------------------------------");

    // ========================================================
    // Etapa 1: Criação do arquivo de dados principal (hash)
    // ========================================================
    log.debug("Criando arquivo de dados /data/data.db via hashing...");
    HashFile hashFile("/data/data.db", NUM_BUCKETS, BUCKET_SIZE);
    hashFile.criarArquivoVazio();

    // ========================================================
    // Etapa 2: Criação dos arquivos de índice B+
    // ========================================================
    using idKey = int;
    using tituloKey = array<char,300>;
    system("mkdir -p /data");

    fstream bptFileId("/data/bptreeId.idx", ios::in | ios::out | ios::binary | ios::trunc);
    if (!bptFileId.is_open()) {
        log.error("Erro ao criar arquivo de índice primário: /data/bptreeId.idx");
        return 1;
    }

    fstream bptFileTitulo("/data/bptreeTitulo.idx", ios::in | ios::out | ios::binary | ios::trunc);
    if (!bptFileTitulo.is_open()) {
        log.error("Erro ao criar arquivo de índice secundário: /data/bptreeTitulo.idx");
        return 1;
    }

    log.debug("Arquivos de índice criados com sucesso.");

    bp<idKey, M_ID> bptreeId;
    bp<tituloKey, M_TITULO> bptreeTitulo;
    bptreeId.iniciar(&bptFileId);
    bptreeTitulo.iniciar(&bptFileTitulo);

    // ========================================================
    // Etapa 3: Leitura e inserção dos registros
    // ========================================================
    vector<Registro> buffer;
    buffer.reserve(BATCH_SIZE);

    long totalLinhas = 0, inseridos = 0, invalidos = 0;

    // Conta o número de linhas válidas no CSV
    {
        ifstream temp(caminhoCSV);
        string linhaTemp;
        while (getline(temp, linhaTemp)) {
            if (!linhaTemp.empty()) totalLinhas++;
        }
    }
    log.info("Total de linhas de dados: " + to_string(totalLinhas));

    auto start = chrono::high_resolution_clock::now();
    string linha;
    log.info("Iniciando leitura e inserção...");

    while (getline(arquivo, linha)) {
        if (linha.empty()) continue;

        RegistroCSV csv = parseCSV(linha);
        if (csv.id == 0) { invalidos++; continue; }

        buffer.push_back(toRegistro(csv));
        inseridos++;

        // Lote completo → grava
        if (buffer.size() >= BATCH_SIZE) {
            vector<loteReturn> indices = hashFile.inserirEmLote(buffer);

            for (const loteReturn &lr : indices) {
                if (lr.pos != static_cast<int64_t>(-1)) {
                    bptreeId.inserir(lr.id, lr.pos);
                    bptreeTitulo.inserir(lr.titulo, lr.pos);
                }
            }

            bptreeId.salvarMetadados();
            bptreeTitulo.salvarMetadados();
            buffer.clear();
        }

        // Mostra progresso periódico
        if (inseridos % PROGRESS_STEP == 0) {
            auto now = chrono::high_resolution_clock::now();
            double elapsed = chrono::duration<double>(now - start).count();
            double percent = (100.0 * inseridos) / totalLinhas;
            double speed = inseridos / elapsed;
            cout << fixed << setprecision(1);
            log.info(
                "Tempo: " + to_string(elapsed) + "s | " +
                to_string(inseridos) + "/" + to_string(totalLinhas) +
                " (" + to_string(percent) + "%) - " + to_string((int)speed) + " regs/s"
            );
        }
    }

    // ========================================================
    // Etapa 4: Último lote
    // ========================================================
    if (!buffer.empty()) {
        vector<loteReturn> indices = hashFile.inserirEmLote(buffer);
        for (const loteReturn &lr : indices) {
            if (lr.pos != static_cast<int64_t>(-1)) {
                bptreeId.inserir(lr.id, lr.pos);
                bptreeTitulo.inserir(lr.titulo, lr.pos);
            }
        }
    }

    // ========================================================
    // Etapa 5: Finalização
    // ========================================================
    log.debug("Salvando metadados e fechando arquivos...");
    bptreeId.flushCache();
    bptreeTitulo.flushCache();
    bptFileId.flush();
    bptFileTitulo.flush();
    bptFileId.close();
    bptFileTitulo.close();

    auto end = chrono::high_resolution_clock::now();
    double totalTime = chrono::duration<double>(end - start).count();

    // ========================================================
    // Etapa 6: Relatório final
    // ========================================================
    log.info("--------------------------------------------------------");
    log.info("Upload concluído!");
    log.info("Registros válidos: " + to_string(inseridos) + " | Inválidos: " + to_string(invalidos));
    log.info("Tempo total: " + to_string(totalTime) + " s");
    if (totalTime > 0)
        log.info("Velocidade média: " + to_string((int)(inseridos / totalTime)) + " regs/s");

    log.info("Total de blocos no arquivo: " + to_string(hashFile.getTotalBlocos()));
    log.info("Índices B+ salvos em: /data/bptreeId.idx e /data/bptreeTitulo.idx");
    log.info("Arquivo principal salvo em: /data/data.db");

    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();
    log.info("Tempo total de execução: " + to_string(tempoTotal) + " ms");
    log.info("--------------------------------------------------------");

    return 0;
}