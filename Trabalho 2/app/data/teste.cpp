#include<bits/stdc++.h>

using namespace std;

const int M = 4; // ajustar depois
using key = int; // mudar conforme tipo do índice
using ptr = streampos;
int bsc;

// Função para remover aspas de uma string
string removerAspas(const string& str) {
    string result = str;
    result.erase(remove(result.begin(), result.end(), '\"'), result.end());
    return result;
}

// Função para trim (remover espaços no início e fim)
string trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");
    return (start == string::npos) ? "" : str.substr(start, end - start + 1);
}

struct no{
    ptr ponteiros[M+2];
    key keys[M+1]; // a mais para fazer inserções temporárias
    int qtdKeys;
    bool folha;
};

void carregar(fstream* file, ptr pag, no *noo){
    file->clear();
    file->seekg(pag);
    file->read(reinterpret_cast<char*>(noo), sizeof(no));
}

void reescrever(fstream* file, ptr pag, no *novo){
    file->clear();
    file->seekp(pag);
    file->write(reinterpret_cast<char*>(novo), sizeof(no));
    file->flush();
}

ptr escrever(fstream* file, no *novo){
    file->clear();
    file->seekp(0, ios::end);
    ptr pt = file->tellp();
    file->write(reinterpret_cast<char*>(novo), sizeof(no));
    file->flush();
    return pt;
}

void inserirCP(fstream *file, no *noAtual, key chave, ptr ponteiro, ptr pAtual) {

    key *chaves = noAtual->keys;
    ptr *ponteiros = noAtual->ponteiros;

    int i = noAtual->qtdKeys-1;

    // Desloca as chaves e ponteiros até encontrar o local certo para a novva chave
    while(i>=0 && chaves[i] > chave){
        chaves[i+1] = chaves[i];
        ponteiros[i+1] = ponteiros[i];
        i--;
    }
    i++;
    chaves[i] = chave; // Posiciona nova chave

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

pair<key,ptr> cisao(fstream *file, ptr pAtual, no *noAtual, no *noNovo, key chave, ptr ponteiro) {
    inserirCP(file, noAtual, chave, ponteiro, pAtual);

    int tam = noAtual->qtdKeys;
    int meio = tam / 2;

    noNovo->folha = noAtual->folha;

    // Copia metade superior para o novo nó
    for (int i = meio; i < tam; i++) {
        noNovo->keys[i - meio] = noAtual->keys[i];
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

        // Chave promovida = primeira do novo nó
        return {noNovo->keys[0], pNovo};
    } else {
        // Nó interno: promover chave central (primeira do novo)
        noNovo->ponteiros[noNovo->qtdKeys] = noAtual->ponteiros[tam];
        ptr pNovo = escrever(file, noNovo);
        reescrever(file, pAtual, noAtual);
        return {noNovo->keys[0], pNovo};
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
        no raiz; raiz.folha = true; raiz.qtdKeys = 0;
        this->raiz = escrever(f, &raiz);
    }

    ptr acharFolha(key alvo, stack<ptr> *pilha = NULL){
        ptr pAtual = this->raiz;
        no noAtual;

        while(true){ // Desce a árvore até encontrar uma folha
            carregar(file, pAtual, &noAtual);

            if(bsc){
                for(int i=0;i<noAtual.qtdKeys;i++){
                    cout << noAtual.keys[i] << " ";
                } cout << "\n";
            }

            if(noAtual.folha) break;

            if(pilha) pilha->push(pAtual); // Insere nós internos na pilha para guardar caminho
            
            // Busca binária no nó interno (encontra o primeiro estritamente maior que o alvo)
            int i = upper_bound(noAtual.keys, noAtual.keys + noAtual.qtdKeys, alvo) - noAtual.keys;
 
            pAtual = noAtual.ponteiros[i]; // Ponteiro esquerdo da chave encontrada
        }

        return pAtual;
    }

    ptr busca(key alvo){
        no folha;
        ptr pFolha = acharFolha(alvo);
        carregar(file, pFolha, &folha);

        // Busca binária na folha (encontra igual ou primeiro maior)
        int i = lower_bound(folha.keys, folha.keys + folha.qtdKeys, alvo)-folha.keys;
        if (i < folha.qtdKeys && folha.keys[i] == alvo)
            return folha.ponteiros[i];
        else // Se não for igual, alvo não existe
            return -1;
    }

    void inserir(key chave, ptr ponteiro){
        stack<ptr> pilha; // Guarda caminho na árvore para casos de cisão
        ptr pAtual = acharFolha(chave, &pilha);
        no noAtual;

        carregar(file, pAtual, &noAtual);

        // Caso 1: há espaço na folha
        if (noAtual.qtdKeys < M) {
            inserirCP(file, &noAtual, chave, ponteiro, pAtual);
            return;
        }

        // Caso 2: não há espaço na folha

        // Divide a folha em dois e promove a chave do meio
        no novoNo; // folha
        novoNo.folha = true; novoNo.qtdKeys = 0;
        pair<key, ptr> promovida = cisao(file, pAtual, &noAtual, &novoNo, chave, ponteiro);
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
            chave = promovida.first; ponteiro = promovida.second;

            // Se chegou na raíz, para
            if(pilha.empty()) break; 
        }

        no novaRaiz;
        novaRaiz.folha = false;
        novaRaiz.keys[0] = chave;
        novaRaiz.ponteiros[0] = pAtual;
        novaRaiz.ponteiros[1] = ponteiro;
        novaRaiz.qtdKeys = 1;
        this->raiz = escrever(file, &novaRaiz);
    }

    void mostrarNoRecursivo(ptr noAtual, int nivel) {
        if (noAtual == -1) return;
        
        no no;
        carregar(file, noAtual, &no);
        
        // Imprime indentação conforme o nível
        for (int i = 0; i < nivel; i++) cout << "  ";
        
        // Imprime o nó
        cout << "No " << noAtual << " (nivel " << nivel << ", " 
            << (no.folha ? "folha" : "interno") << "): ";
        
        for (int i = 0; i < no.qtdKeys; i++) {
            cout << no.keys[i];
            if (i < no.qtdKeys - 1) cout << ", ";
        }
        cout << endl;
        
        // Se não é folha, mostra os filhos recursivamente
        if (!no.folha) {
            for (int i = 0; i <= no.qtdKeys; i++) {
                mostrarNoRecursivo(no.ponteiros[i], nivel + 1);
            }
        }
    }

};

int main(){
    bp arvore;
    fstream file("arvore.bin", ios::in | ios::out | ios::binary | ios::trunc);
    
    if (!file.is_open()) {
        cerr << "Erro ao abrir/criar arquivo dados.bin" << endl;
        return 1;
    }

    arvore.iniciar(&file);
    ifstream arquivo("artigo_teste.csv");
    string linha;
    
    int lim = 1000;
    int i = 0;
    bsc = 0;
    while (getline(arquivo, linha) && i++ < lim) {
        vector<string> campos;
        stringstream ss(linha);
        string campo;
        
        // Ler campos separados por ;
        while (getline(ss, campo, ';')) {
            // Remover aspas e espaços em branco
            campo = trim(removerAspas(campo));
            campos.push_back(campo);
        }
        
        // Verificar se temos pelo menos 7 campos
        if (campos.size() >= 7) {
            try {
                int id = stoi(campos[0]);
                file.seekp(0, ios::end);
                ptr pt = file.tellp();
                cout << id << endl;
                arvore.inserir(id, i);
                
                // campos[1],    // campos[1] - título

            } catch (const exception& e) {
                cerr << "Erro ao processar linha: " << linha << endl;
                cerr << "Erro: " << e.what() << endl;
            }
        }
    }
    cout << "\n\n";
    bsc = 1;
    
    arvore.mostrarNoRecursivo(arvore.raiz, 0);
    
    cout << "\n\n" << arvore.busca(50) << "\n";




    arquivo.close();
    return 0;
}