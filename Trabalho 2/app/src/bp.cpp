#include<bits/stdc++.h>

using namespace std;

const int M = 4; // ajustar depois
using key = int; // mudar conforme tipo do índice
using ptr = streampos;

struct no{
    bool folha;
    int qtdKeys;
    key keys[M];
    ptr ponteiros[M+1];
    ptr prox;
};

void carregar(fstream* file, ptr pag, no *noo){
    (*file).seekg(pag);
    (*file).read(reinterpret_cast<char*>(noo), sizeof(no));
}

void reescrever(fstream* file, ptr pag, no *novo){
    (*file).seekp(pag);
    (*file).write(reinterpret_cast<char*>(novo), sizeof(no));
}

ptr escrever(fstream* file, no *novo){
    (*file).seekp(0, ios::end);
    ptr pt = (*file).tellp();
    (*file).write(reinterpret_cast<char*>(novo), sizeof(no));
    return pt;
}

void inserirCP(key keys[], ptr pointers[], int *qtdKeys, key newKey, ptr newPointerE, bool ehFolha, ptr newPointerD=-1) { 
    int tam = *qtdKeys;
    int i = tam-1;
    while(i>=0 && keys[i]>newKey) { // move as chaves
        keys[i+1] = keys[i];
        i--;
    }
    keys[i+1] = newKey; // insere nova chave
    (*qtdKeys)++;

    if(ehFolha) { // move os ponteiros da folha
        for(int j=tam-1;j>=i+1;j--) pointers[j+1] = pointers[j];
        pointers[i+1] = newPointerE; // insere novo ponteiro
    }
    else { // move os ponteiros do nó interno
        for(int j=tam;j>i+1;j--) pointers[j] = pointers[j-1];
        pointers[i+1] = newPointerE;
        pointers[i+2] = newPointerD;
    }
}

void cisao(no *noo, no *novoNo, key chave, ptr ponteiro, bool ehFolha, ptr ponteiroD=-1){
    
    
}

struct bp{
    ptr raiz;
    ptr primeiraFolha;
    ptr prox_livre;
    ptr qtd_pags = 0;
    fstream* file;

    void iniciar(fstream* f){
        this->file = f;
        no raiz;
        raiz.qtdKeys = 0;
        raiz.folha = true;
        this->raiz = escrever(f, &raiz);
    }

    ptr busca(key alvo){
        ptr atual = raiz;

        while (true){
            no noAtual;
            
            carregar(file, atual, &noAtual);

            if (noAtual.folha) break;

            int i = upper_bound(noAtual.keys, noAtual.keys + noAtual.qtdKeys, alvo) - 1 - noAtual.keys;
            atual = noAtual.ponteiros[i];
        }

        no folha;
        carregar(file, atual, &folha);

        int i = upper_bound(folha.keys, folha.keys + folha.qtdKeys, alvo)-1-folha.keys;

        if (i < folha.qtdKeys && folha.keys[i] == alvo) return folha.ponteiros[i];
        else return -1;
    }

    void inserir(key chave, ptr ponteiro){
        ptr atual = raiz, pNovoNo;
        stack<ptr> pilha; // guarda os pais, caso precise de cisão
        no noAtual, novoNo;

        // em construção
    }
};

int main(){
    bp arvore;
    fstream file("dados.bin", ios::in | ios::out | ios::binary | ios::trunc);
    int id=0;

    arvore.iniciar(&file);
    cout << "oi" << endl;
    for(int i = 0; i<10; i++){
        arvore.inserir(i, id++);
    }
    return 0;
}