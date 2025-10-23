#include <iostream>
#include <sys/statvfs.h>
#include <unistd.h>
#include <fstream>
using namespace std;

// M=$(./calcM); g++ -DM=$M bptreefile.cpp -o bptree

// tipo usado no seu código principal
using ptr = streampos;

// Função genérica para calcular M com base no tamanho do bloco e tipo de chave
template<typename key>
int calcular_M(size_t tamanho_bloco) {
    size_t base = sizeof(int) + sizeof(bool);
    size_t tamKey = sizeof(key);
    size_t tamPtr = sizeof(ptr);

    // Fórmula: base + (M+1)*tamKey + (M+2)*tamPtr <= tamanho_bloco
    int M = (tamanho_bloco - base - 2*tamPtr - tamKey) / (tamKey + tamPtr);
    return M > 2 ? M : 2; // mínimo de 2
}

// Obtém o tamanho do bloco (em bytes) do sistema de arquivos atual
size_t tamanho_bloco(const string& caminho = ".") {
    struct statvfs info;
    if (statvfs(caminho.c_str(), &info) == 0) {
        return info.f_bsize; // tamanho de bloco lógico do sistema de arquivos
    }
    return 4096; // fallback seguro
}

int main() {
    size_t bloco = tamanho_bloco(".");
    int M = calcular_M<int>(bloco); // você pode trocar <int> por outro tipo de chave
    cout << M << endl;
    return 0;
}
