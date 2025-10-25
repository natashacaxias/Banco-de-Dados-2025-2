#include<iostream>
#include<string>
#include<cmath>
#include<algorithm>
#include<cstdint>
#include<array>

#ifdef _WIN32
    #include <windows.h>
    // Definir byte como unsigned char antes de incluir windows.h
    #define byte unsigned char
#else
    #include <sys/statvfs.h>
#endif

using namespace std;

int calcularM(int tamBloco, int tamTipo, int tamPoint){
    return floor((tamBloco+tamTipo)/(float)(tamPoint+tamTipo));
}

class DiskInfo {
public:
    static unsigned long getBlockSize(const string& path = "") {
#ifdef _WIN32
        // Código Windows
        const char* drivePath = path.empty() ? "C:\\" : path.c_str();
        DWORD sectorsPerCluster, bytesPerSector, freeClusters, totalClusters;
        
        if (GetDiskFreeSpaceA(drivePath, &sectorsPerCluster, &bytesPerSector, 
                            &freeClusters, &totalClusters)) {
            return sectorsPerCluster * bytesPerSector;
        }
        return 4096; // Default se falhar
        
#else
        // Código Linux/Unix
        const char* unixPath = path.empty() ? "/" : path.c_str();
        struct statvfs vfs;
        
        if (statvfs(unixPath, &vfs) == 0) {
            return vfs.f_bsize; // Tamanho do bloco de transferência
        }
        return 4096; // Default se falhar
#endif
    }
    
    static void printDiskInfo(const string& path = "") {
#ifdef _WIN32
        cout << "Sistema: Windows" << endl;
        const char* drivePath = path.empty() ? "C:\\" : path.c_str();
#else
        cout << "Sistema: Linux/Unix" << endl;
        const char* drivePath = path.empty() ? "/" : path.c_str();
#endif

        unsigned long block_size = getBlockSize(path);
        
        cout << "Tamanho do Bloco (B): " << block_size << " bytes" << endl;
        cout << "Tamanho em KB: " << (block_size / 1024) << " KB" << endl;
        
        // Cálculo para as árvores B+
        // Id como chave
        cout << "Ordem com ID (int) como chave: " << calcularM(block_size, sizeof(int), sizeof(int64_t)) << "\n";
        // Título como chave
        cout << "Ordem com Titulo (char[300]) como chave: " << calcularM(block_size, sizeof(array<char,300>), sizeof(int64_t)) << "\n\n";
    }
};

int main() {
    DiskInfo::printDiskInfo();
    return 0;
}