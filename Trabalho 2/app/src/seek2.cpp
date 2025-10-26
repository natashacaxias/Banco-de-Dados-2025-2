// ============================================================
// seek2.cpp
// ------------------------------------------------------------
// Programa responsável por buscar um registro no arquivo de
// dados utilizando o índice secundário (B+Tree por Título).
//
// A busca é realizada com base na chave textual (Título).
// O índice B+ mapeia o título → posição no arquivo de dados,
// permitindo localizar o registro correspondente de forma
// eficiente em disco.
// ============================================================

#include <bits/stdc++.h>
#include <chrono>
#include <fstream>
#include "../include/bptreefile.h"
#include "../include/common.h"
using namespace std;

// ============================================================
// Função principal
// ------------------------------------------------------------
// Espera como argumento na linha de comando:
//   ./bin/seek2 "<Título>"
//
// Exemplo de execução dentro do container:
//   docker run --rm -v $(pwd)/data:/data tp2 ./bin/seek2 "Deep Learning Survey"
// ============================================================
int main(int argc, char* argv[]) {
    auto inicioTotal = chrono::high_resolution_clock::now();

    cout << "\n=== TP2 – Busca por Título (seek2) ===\n" << endl;

    // --------------------------------------------------------
    // Validação dos argumentos
    // --------------------------------------------------------
    // O programa exige o título como argumento.
    // Caso não seja fornecido, exibe mensagem de uso.
    if (argc < 2) {
        cerr << "Uso: ./bin/seek2 <Título>" << endl;
        return 1;
    }

    // --------------------------------------------------------
    // Inicialização dos caminhos e parâmetros
    // --------------------------------------------------------
    string chaveStr = argv[1];               // título fornecido
    string dbPath = "/data/data.db";         // arquivo de dados principal
    string idxPath = "/data/bptreeTitulo.idx"; // arquivo de índice secundário

    cout << "Arquivo de dados: " << dbPath << endl;
    cout << "Arquivo de índices: " << idxPath << endl;
    cout << "Chave: " << chaveStr << endl;

    // ========================================================
    // Etapa 1: Abertura dos arquivos
    // ========================================================

    // --------------------------------------------------------
    // Abre o arquivo de índice B+ (secundário, por título)
    // --------------------------------------------------------
    fstream bptFile(idxPath, ios::in | ios::out | ios::binary);
    if (!bptFile.is_open()) {
        cerr << "Erro ao abrir arquivo de índice B+ secundário: " << idxPath << endl;
        return 1;
    }

    // --------------------------------------------------------
    // Abre o arquivo de dados principal
    // --------------------------------------------------------
    fstream db(dbPath, ios::in | ios::out | ios::binary);
    if (!db.is_open()) {
        cerr << "Erro ao abrir arquivo de dados: " << dbPath << endl;
        return 1;
    }

    // ========================================================
    // Etapa 2: Definição do tipo de chave e carregamento da árvore
    // ========================================================

    // Define o tipo da chave usada no índice (string fixa de 300 chars)
    using Chave = array<char, 300>;
    bp<Chave, M_TITULO> bptree; // estrutura da árvore B+
    bptree.carregarArvore(&bptFile); // carrega a B+ existente do disco

    Registro r; // estrutura para armazenar o registro encontrado

    // ========================================================
    // Etapa 3: Preparação da chave para busca
    // ========================================================
    // Converte o título para o formato fixo de array<char,300>,
    // garantindo que a string seja corretamente truncada ou
    // preenchida com zeros.
    Chave chave{};
    memset(chave.data(), 0, sizeof(Chave));
    strncpy(chave.data(), chaveStr.c_str(), sizeof(Chave));

    // ========================================================
    // Etapa 4: Execução da busca no índice
    // ========================================================
    cout << "\nRecuperando registro do arquivo de dados..." << endl;
    auto inicioBusca = chrono::high_resolution_clock::now();

    // Busca o título na árvore B+ (usa ponteiro para acessar dados)
    pair<bool, long> res = bptree.buscar(chave, r, &db);

    auto fimBusca = chrono::high_resolution_clock::now();
    double tempoBusca = chrono::duration<double, milli>(fimBusca - inicioBusca).count();

    // ========================================================
    // Etapa 5: Exibição do resultado
    // ========================================================
    if (res.first) {
        cout << "\nRegistro encontrado:\n";
        cout << "ID: " << r.id << "\n";
        cout << "Título: " << string(r.titulo.data()) << "\n";
        cout << "Ano: " << r.ano << "\n";
        cout << "Autores: " << r.autores << "\n";
        cout << "Citações: " << r.citacoes << "\n";
        cout << "Atualização: " << r.data_atualizacao << "\n";
        cout << "Snippet: " << r.snippet << "\n";
    } else {
        cout << "\nRegistro não encontrado no arquivo de dados.\n";
    }

    // ========================================================
    // Etapa 6: Estatísticas da busca
    // ========================================================
    cout << "\nEstatísticas da busca:" << endl;
    cout << "  Blocos lidos no índice: " << res.second << endl;
    cout << "  Tempo de busca: " << tempoBusca << " ms" << endl;
    cout << "  Total de blocos no índice: " << bptree.contarBlocos() << "\n";

    // ========================================================
    // Etapa 7: Estatísticas gerais e encerramento
    // ========================================================
    auto fimTotal = chrono::high_resolution_clock::now();
    double tempoTotal = chrono::duration<double, milli>(fimTotal - inicioTotal).count();

    cout << "\nEstatísticas gerais:" << endl;
    cout << fixed << setprecision(2);
    cout << "  Tempo total de execução: " << tempoTotal << " ms" << endl;
    cout << "  Total de blocos no arquivo: " << bptree.contarBlocos() << "\n" << endl;

    return 0;
}
