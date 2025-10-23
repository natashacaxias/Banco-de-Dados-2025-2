#include<bits/stdc++.h>
using namespace std;
using ptr = streampos;

#ifndef M // Ordem
#define M 4096 // Valor padrão
#endif

// ===== Traits para tratar diferentes tipos de chave ============================

// Trait padrão para tipos triviais (int, double, etc.)
template<typename Key>
struct KeyOps {
    static bool less(const Key& a, const Key& b) { return a < b; }
    static bool equal(const Key& a, const Key& b) { return a == b; }
    static void copy(Key& dest, const Key& src) { dest = src; }
};

// Especialização para arrays de char (strings)
template<size_t N>
struct KeyOps<char[N]> {
    static bool less(const char a[N], const char b[N]) { return strcmp(a, b) < 0; }
    static bool equal(const char a[N], const char b[N]) { return strcmp(a, b) == 0; }
    static void copy(char dest[N], const char src[N]) { strcpy(dest, src); }
};

// Comparador dos traits para busca binária
template<typename key>
auto comp() {
    return [](const key& a, const key& b) { return KeyOps<key>::less(a,b); };
}

// ===== Utilitários =================================================

// Nó da árvore (pode ser folha ou nó interno)
template<typename key>
struct no{
    // Espaços mais para fazer inserções temporárias em caso de cisão
    ptr ponteiros[M+2];
    key keys[M+1];
    int qtdKeys;
    bool folha;
};

template<typename key>
void carregar(fstream* file, ptr pag, no<key> *noo){
    file->clear();
    file->seekg(pag);
    file->read(reinterpret_cast<char*>(noo), sizeof(no<key>));
}

template<typename key>
void reescrever(fstream* file, ptr pag, no<key> *novo){
    file->clear();
    file->seekp(pag);
    file->write(reinterpret_cast<char*>(novo), sizeof(no<key>));
    file->flush();
}

template<typename key>
ptr escrever(fstream* file, no<key> *novo){
    file->clear();
    file->seekp(0, ios::end);
    ptr pt = file->tellp();
    file->write(reinterpret_cast<char*>(novo), sizeof(no<key>));
    file->flush();
    return pt;
}

// Insere par chave/ponteiro em um nó
template<typename key>
void inserirCP(fstream *file, no<key> *noAtual, key chave, ptr ponteiro, ptr pAtual) {

    key *chaves = noAtual->keys;
    ptr *ponteiros = noAtual->ponteiros;

    int i = noAtual->qtdKeys-1;

    // Desloca as chaves e ponteiros até encontrar o local certo para a novva chave
    while(i>=0 && KeyOps<key>::less(chave, chaves[i])){
        KeyOps<key>::copy(chaves[i+1], chaves[i]);
        ponteiros[i+1] = ponteiros[i];
        i--;
    }
    i++;
    KeyOps<key>::copy(chaves[i+1], chave);

    if(noAtual->folha){
        // Se for folha, chaves e ponteiros em índices iguais andam juntos
        ponteiros[i+1] = ponteiros[i];
        ponteiros[i] = ponteiro;
    }
    else{
        // Se for nó interno, o novo ponteiro corresponde ao novo nó à direita após cisão
        ponteiros[i+1] = ponteiro;
    }
    
    noAtual->qtdKeys++;
    reescrever(file, pAtual, noAtual);
}

// Divide nó cheio em dois e promove uma chave
template<typename key>
pair<key,ptr> cisao(fstream *file, ptr pAtual, no<key> *noAtual, no<key> *noNovo, key chave, ptr ponteiro) {
    inserirCP(file, noAtual, chave, ponteiro, pAtual);

    int tam = noAtual->qtdKeys;
    int meio = tam / 2;

    noNovo->folha = noAtual->folha;

    // Copia metade superior para o novo nó
    for (int i = meio; i < tam; i++) {
        KeyOps<key>::copy(noNovo->keys[i - meio], noAtual->keys[i]);
        noNovo->ponteiros[i - meio] = noAtual->ponteiros[i];
    }
    noNovo->qtdKeys = tam - meio;
    noAtual->qtdKeys = meio; // mantém apenas metade inferior

    if (noAtual->folha) {
        // Conecta folhas
        noNovo->ponteiros[M] = noAtual->ponteiros[M];
        ptr pNovo = escrever(file, noNovo);
        noAtual->ponteiros[M] = pNovo;
        reescrever(file, pAtual, noAtual);
    } else {
        noNovo->ponteiros[noNovo->qtdKeys] = noAtual->ponteiros[tam];
        ptr pNovo = escrever(file, noNovo);
        reescrever(file, pAtual, noAtual);
    }

    key promovida;
    KeyOps<key>::copy(promovida, noNovo->keys[0]);
    return {promovida, pNovo};
}

// Desce na árvore buscando a folha correspondente à chave alvo
template<typename key>
ptr acharFolha(fstream *file, ptr pRaiz, key alvo, stack<ptr> *pilha = NULL){
    ptr pAtual = pRaiz;
    no<key> noAtual;

    while(true){ // Desce a árvore até encontrar uma folha
        carregar(file, pAtual, &noAtual);

        if(noAtual.folha) break;

        if(pilha) pilha->push(pAtual); // Insere nós internos na pilha para guardar caminho
        
        // Busca binária no nó interno (encontra o primeiro estritamente maior que o alvo)
        int i = upper_bound(noAtual.keys, noAtual.keys + noAtual.qtdKeys, alvo, comp<key>()) - noAtual.keys;

        pAtual = noAtual.ponteiros[i]; // Ponteiro esquerdo da chave encontrada
    }

    return pAtual;
}

// ======= Árvore B+ ===========================================

template<typename key>
struct bp{
    ptr raiz;
    ptr primeiraFolha;
    ptr qtd_nos = 0;
    fstream* file;

    void iniciar(fstream* f){
        this->file = f;
        no<key> raiz; raiz.folha = true; raiz.qtdKeys = 0;
        this->raiz = escrever(f, &raiz);
        this->qtd_nos++;
    }

    ptr buscar(key alvo){
        no<key> folha;
        ptr pFolha = acharFolha(file, this->raiz, alvo);
        carregar(file, pFolha, &folha);

        // Busca binária na folha (encontra igual ou primeiro maior)
        int i = lower_bound(folha.keys, folha.keys + folha.qtdKeys, alvo, comp<key>())-folha.keys;
        if (i < folha.qtdKeys && KeyOps<key>::equal(folha.keys[i], alvo))
            return folha.ponteiros[i];
        else // Se não for igual, alvo não existe
            return -1;
    }

    void inserir(key chave, ptr ponteiro){
        stack<ptr> pilha; // Guarda caminho na árvore para casos de cisão
        ptr pAtual = acharFolha(file, this->raiz, chave, &pilha);
        no<key> noAtual;

        carregar(file, pAtual, &noAtual);

        // Caso 1: há espaço na folha
        if (noAtual.qtdKeys < M) {
            inserirCP(file, &noAtual, chave, ponteiro, pAtual);
            return;
        }

        // Caso 2: não há espaço na folha

        // Divide a folha em dois e promove a chave do meio
        no<key> novoNo; // folha
        novoNo.folha = true; novoNo.qtdKeys = 0;
        pair<key, ptr> promovida = cisao(file, pAtual, &noAtual, &novoNo, chave, ponteiro);
        this->qtd_nos++;
        chave = promovida.first; ponteiro = promovida.second;

        // "Sobe" na árvore até encontrar espaço (tira da pilha)
        while(!pilha.empty()){
            pAtual = pilha.top(); pilha.pop();
            carregar(file, pAtual, &noAtual);

            // Se houver espaço, insere
            if(noAtual.qtdKeys<M){
                inserirCP(file, &noAtual, chave, ponteiro, pAtual);
                return;
            }

            // Se não, divide o nó interno em dois e promove a chave do meio
            novoNo.folha = false; novoNo.qtdKeys = 0;
            promovida = cisao(file, pAtual, &noAtual, &novoNo, chave, ponteiro);
            this->qtd_nos++;
            chave = promovida.first; ponteiro = promovida.second;

            // Se chegou na raíz, para
            if(pilha.empty()) break; 
        }

        // Cria nova raíz
        no<key> novaRaiz;
        novaRaiz.folha = false;
        KeyOps<key>::copy(novaRaiz.keys[0], chave);
        novaRaiz.ponteiros[0] = pAtual;
        novaRaiz.ponteiros[1] = ponteiro;
        novaRaiz.qtdKeys = 1;
        this->raiz = escrever(file, &novaRaiz);
        this->qtd_nos++;
    }

    void mostrarArvore(ptr noAtual, int nivel) {
        if (noAtual == -1) return;
        
        no<key> no;
        carregar(file, noAtual, &no);

        cout << "Quantidade de nós: " << this->qtd_nos << "\n";
        
        // Imprime indentação conforme o nível
        for (int i=0; i<nivel; i++) cout << "  ";
        
        // Imprime o nó
        cout << "No " << noAtual << " (nivel " << nivel << ", " 
            << (no.folha ? "folha" : "interno") << "): ";
        
        for (int i=0;i<no.qtdKeys;i++) {
            cout << no.keys[i];
            if (i < no.qtdKeys - 1) cout << ", ";
        }
        cout << endl;
        
        // Se não é folha, mostra os filhos recursivamente
        if (!no.folha) {
            for (int i=0;i<=no.qtdKeys;i++) {
                mostrarArvore(no.ponteiros[i], nivel + 1);
            }
        }
    }
};