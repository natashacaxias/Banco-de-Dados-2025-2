#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <array>

#ifdef _WIN32
    #define byte unsigned char   // necessário antes de windows.h
    #include <windows.h>
#else
    #include <sys/statvfs.h>     // usado em Linux/Unix
#endif

using namespace std;

// calcula a ordem M da B+Tree (nº máx. de chaves por nó)
int calcularM(int tamBloco, int tamTipo, int tamPoint) {
    return floor(tamBloco / static_cast<float>(tamPoint + tamTipo));
}

// classe utilitária para obter informações do disco
class DiskInfo {
public:
    // retorna o tamanho de bloco do sistema de arquivos
    static unsigned long getBlockSize(const string& path = "") {
#ifdef _WIN32
        const char* drivePath = path.empty() ? "C:\\\\" : path.c_str();
        DWORD sectorsPerCluster, bytesPerSector, freeClusters, totalClusters;
        
        if (GetDiskFreeSpaceA(drivePath, &sectorsPerCluster, &bytesPerSector, 
                              &freeClusters, &totalClusters)) {
            return sectorsPerCluster * bytesPerSector;
        }
        return 4096; // padrão se não conseguir obter
#else
        const char* unixPath = path.empty() ? "/" : path.c_str();
        struct statvfs vfs;
        
        if (statvfs(unixPath, &vfs) == 0) {
            return vfs.f_bsize; // retorna tamanho de bloco
        }
        return 4096;
#endif
    }
    
    // imprime informações sobre o sistema e cálculos de M
    static void printDiskInfo(const string& path = "") {
#ifdef _WIN32
        cout << "Sistema: Windows" << endl;
#else
        cout << "Sistema: Linux/Unix" << endl;
#endif
        unsigned long block_size = getBlockSize(path);
        
        cout << "Tamanho do Bloco: " << block_size << " bytes ("
             << (block_size / 1024.0) << " KB)" << endl;

        // mostra cálculo das ordens da B+Tree para os dois índices
        cout << "\nCálculo da ordem (M) da B+Tree:" << endl;
        cout << "  - Chave ID (int): M = "
             << calcularM(block_size, sizeof(int), sizeof(int64_t)) << endl;
        cout << "  - Chave Título (char[300]): M = "
             << calcularM(block_size, 300, sizeof(int64_t)) << endl;
        cout << endl;
    }
};

int main() {
    // executa e exibe as informações
    DiskInfo::printDiskInfo();
    return 0;
}
