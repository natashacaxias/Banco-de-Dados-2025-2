#include<bits/stdc++.h>

using namespace std;

const int M = 4; // ajustar depois
using key = int; // mudar conforme tipo do índice
using ptr = streampos;

struct no{
    bool folha;
    int qtdKeys;
    key keys[M];
    ptr ponteiros[M];
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
        if(i+1 == *qtdKeys-1)
        for(int j=tam;j>i+1;j--) pointers[j] = pointers[j-1];
        pointers[i+1] = newPointerE;
        pointers[i+2] = newPointerD;
    }
}

void cisao(no *noo, no *novoNo, key chave, ptr ponteiro, bool ehFolha, ptr ponteiroD=-1){
    int tam = noo->qtdKeys;
    int pos = lower_bound(noo->keys, noo->keys+tam, chave) - noo->keys;

    int cis = (tam + 1) /2;
    if(tam%2!=0 && pos<cis) cis--; // se a nova chave for pra esquerda, "doa" mais uma chave para a nova folha para não desequilibrar

    for(int i=cis;i<tam;i++){ // manda chaves e ponteiros para a nova folha
        novoNo->keys[i-cis] = noo->keys[i];
        novoNo->ponteiros[i-cis] = noo->ponteiros[i];
    }
    novoNo->qtdKeys = tam - cis; // ajusta quantidades de chaves
    noo->qtdKeys = cis;

    if (ehFolha){
        if (pos<cis) inserirCP(noo->keys, noo->ponteiros, &noo->qtdKeys, chave, ponteiro, true); // insere na folha esquerda
        else inserirCP(novoNo->keys, novoNo->ponteiros, &novoNo->qtdKeys, chave, ponteiro, true); // insere na folha da direita
    }

    else{ // nó interno
        novoNo->ponteiros[tam - cis - 1] = noo->ponteiros[tam]; // último ponteiro

        if (pos<cis) inserirCP(noo->keys, noo->ponteiros, &noo->qtdKeys, chave, ponteiro, false, ponteiroD); // insere na folha esquerda
        else inserirCP(novoNo->keys, novoNo->ponteiros, &novoNo->qtdKeys, chave, ponteiro, false,  ponteiroD); // insere na folha da direita
        
        noo->qtdKeys--; // chave do meio sobe e é removida da 
    }
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

        while (true){ // desce até encontrar folha
            carregar(file, atual, &noAtual);
            cout << noAtual.keys[0] << endl;
            if (noAtual.folha) break;
            pilha.push(atual);

            // busca binária para encontrar chave
            int i = upper_bound(noAtual.keys, noAtual.keys + noAtual.qtdKeys, chave) - 1 - noAtual.keys;
            atual = noAtual.ponteiros[i];
        }

        no folha; // carregar folha encontrada
        carregar(file, atual, &folha);

        if (folha.qtdKeys < M-1) { // caso 1: tem espaço na folha
            inserirCP(folha.keys, folha.ponteiros, &folha.qtdKeys, chave, ponteiro, true);
            reescrever(file, atual, &folha);

        }
        else{ // caso 2: não tem espaço no nó
            novoNo.folha = true; // cria novo nó 
            novoNo.qtdKeys = 0;
            pNovoNo = escrever(file, &novoNo);

            cisao(&folha, &novoNo, chave, ponteiro, true);
            reescrever(file, atual, &folha);
            reescrever(file, pNovoNo, &novoNo);

            // guardar chave e ponteiros da última chave do nó que será promovida
            chave = folha.keys[folha.qtdKeys-1];
            ponteiro = atual;
            ptr ponteiroD = pNovoNo; // caso seja inserida na primeira posição do nó
            
            while(!pilha.empty()){ // tira da pilha até encontrar nó interno com espaço
                atual = pilha.top(); pilha.pop();
                carregar(file, atual, &noAtual);

                if (noAtual.qtdKeys<M){ // caso 1: tem espaço
                    inserirCP(noAtual.keys, noAtual.ponteiros, &noAtual.qtdKeys, chave, ponteiro, false, ponteiroD);
                    reescrever(file, atual, &noAtual);
                    break;
                }
                else{ // caso 2: não tem espaço
                    novoNo.folha = false;
                    novoNo.qtdKeys = 0;
                    pNovoNo = escrever(file, &novoNo); // cria novo nó interno

                    cisao(&noAtual, &novoNo, chave, ponteiro, false, ponteiroD);
                    reescrever(file, atual, &noAtual);
                    reescrever(file, pNovoNo, &novoNo);

                    chave = folha.keys[folha.qtdKeys-1]; // pega chave do meio e sobe
                    ponteiro = atual;
                    ptr ponteiroE = pNovoNo;

                    if(pilha.empty()){ // chegou na raíz e não há espaço
                        novoNo.folha = false;
                        novoNo.qtdKeys = 0;
                        pNovoNo = escrever(file, &novoNo); // cria novo nó interno

                        cisao(&noAtual, &novoNo, chave, ponteiro, false, ponteiroD);
                        reescrever(file, atual, &noAtual);
                        reescrever(file, pNovoNo, &novoNo);

                        chave = folha.keys[folha.qtdKeys-1]; // pega chave do meio e sobe
                        ponteiro = atual;
                        ptr ponteiroE = pNovoNo;

                        novoNo.folha = false;
                        novoNo.qtdKeys = 0;
                        novoNo.keys[0] = chave;
                        novoNo.ponteiros[0] = ponteiro;
                        novoNo.ponteiros[1] = ponteiroD;
                        novoNo.qtdKeys = 1;
                        pNovoNo = escrever(file, &novoNo); // cria novo nó interno que será a nova raíz
                        this->raiz = pNovoNo;
                    }
                }
            }
        }
    }
};

int main(){
    bp arvore;
    fstream file("dados.bin", ios::in | ios::out | ios::binary);

    int id=0;

    arvore.iniciar(&file);
    cout << "oi" << endl;
    for(int i = 0; i<10; i++){
        arvore.inserir(i, id++);
    }
    return 0;
}