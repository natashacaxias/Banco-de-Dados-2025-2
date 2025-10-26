#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <array>

#ifdef _WIN32
    #define byte unsigned char  // definir antes de windows.h
    #include <windows.h>
#else
    #include <sys/statvfs.h>
#endif

using namespace std;

int calcularM(int tamBloco, int tamTipo, int tamPoint) {
    // número máximo de chaves por nó da B+Tree
    return floor(tamBloco / static_cast<float>(tamPoint + tamTipo));
}

class DiskInfo {
public:
    static unsigned long getBlockSize(const string& path = "") {
#ifdef _WIN32
        const char* drivePath = path.empty() ? "C:\\\\" : path.c_str();
        DWORD sectorsPerCluster, bytesPerSector, freeClusters, totalClusters;
        
        if (GetDiskFreeSpaceA(drivePath, &sectorsPerCluster, &bytesPerSector, 
                              &freeClusters, &totalClusters)) {
            return sectorsPerCluster * bytesPerSector;
        }
        return 4096; // valor padrão se falhar
#else
        const char* unixPath = path.empty() ? "/" : path.c_str();
        struct statvfs vfs;
        
        if (statvfs(unixPath, &vfs) == 0) {
            return vfs.f_bsize; // tamanho do bloco de transferência
        }
        return 4096;
#endif
    }
    
    static void printDiskInfo(const string& path = "") {
#ifdef _WIN32
        cout << "Sistema: Windows" << endl;
#else
        cout << "Sistema: Linux/Unix" << endl;
#endif
        unsigned long block_size = getBlockSize(path);
        
        cout << "Tamanho do Bloco: " << block_size << " bytes ("
             << (block_size / 1024.0) << " KB)" << endl;

        cout << "\nCálculo da ordem (M) da B+Tree:" << endl;
        cout << "  - Chave ID (int): M = "
             << calcularM(block_size, sizeof(int), sizeof(int64_t)) << endl;
        cout << "  - Chave Título (char[300]): M = "
             << calcularM(block_size, 300, sizeof(int64_t)) << endl;
        cout << endl;
    }
};

int main() {
    DiskInfo::printDiskInfo();
    return 0;
}
