#include <iostream>
#include <windows.h>
#include <cmath>
#include <fstream>
using namespace std;
using ptr = streampos;

/*
set /p M=< <(calcM.exe)
g++ -DM=%M% bptreefile.cpp -o bptree.exe
*/


template<typename key>
int calcular_M(size_t tamanho_bloco) {
    size_t base = sizeof(int) + sizeof(bool);
    size_t tamKey = sizeof(key);
    size_t tamPtr = sizeof(ptr);
    return (tamanho_bloco - base - 2*tamPtr - tamKey) / (tamKey + tamPtr);
}

size_t tamanho_bloco(const string& caminhoArquivo) {
#ifdef _WIN32
    DWORD setoresPorCluster, bytesPorSetor, numClustersLivres, numClustersTotais;
    if (GetDiskFreeSpaceA(caminhoArquivo.substr(0, 3).c_str(),
        &setoresPorCluster, &bytesPorSetor, &numClustersLivres, &numClustersTotais)) {
        return setoresPorCluster * bytesPorSetor;
    }
#endif
    return 4096; // fallback
}

int main() {
    size_t bloco = tamanho_bloco("C:\\");
    int M = calcular_M<int>(bloco);
    cout << M << endl;
}