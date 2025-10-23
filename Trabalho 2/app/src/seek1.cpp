#include <bits/stdc++.h>
#include "../include/bptreefile.h"
using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Uso: ./seek1 <arquivo_idx.bpt> <arquivo_data.db> <id>\n";
        return 1;
    }

    string idxFile = argv[1];
    string dataFile = argv[2];
    int idBusca = stoi(argv[3]);

    BpTree<int> idx;
    if (!idx.open(idxFile)) {
        cerr << "❌ Erro ao abrir índice: " << idxFile << endl;
        return 1;
    }

    auto [pos, blocosIndice] = idx.buscar(idBusca);

    if (pos == -1) {
        cout << "⚠️ ID " << idBusca << " não encontrado no índice.\n";
        cout << "Blocos lidos no índice: " << blocosIndice << endl;
        return 0;
    }

    // Ler registro diretamente do arquivo de dados
    fstream file(dataFile, ios::in | ios::binary);
    if (!file.is_open()) {
        cerr << "❌ Erro ao abrir " << dataFile << endl;
        return 1;
    }

    file.seekg(pos);
    Registro r{};
    file.read(reinterpret_cast<char*>(&r), sizeof(Registro));

    cout << "✅ Registro encontrado via índice!\n";
    cout << "ID: " << r.id << "\nTítulo: " << r.titulo << "\nAno: " << r.ano
         << "\nAutores: " << r.autores << "\nCitações: " << r.citacoes
         << "\nData/hora: " << r.datahora << "\nSnippet: " << r.snippet << endl;

    cout << "Blocos lidos no índice: " << blocosIndice
         << " | Total blocos de índice: " << idx.qtd_nos << endl;

    file.close();
    return 0;
}
