#include<bits/stdc++.h>

using namespace std;

const int M = 4; // ajustar depois
using key = int; // mudar conforme tipo do índice
using ptr = streampos;
int idg;

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
    bool folha;
    int qtdKeys;
    key keys[M+2]; // a mais para fazer inserções temporárias
    ptr ponteiros[M+2]; // um a mais, devido à estrutura
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

void inserirCP(fstream *file, ptr atual, no *noAtual, key newKey, ptr newPointerE, bool ehFolha, ptr newPointerD=-1) {

    key *keys = noAtual->keys;
    ptr *pointers = noAtual->ponteiros;
    int *qtdKeys = &(noAtual->qtdKeys);

    int tam = noAtual->qtdKeys;
    int i = tam-1;
    while(i>=0 && keys[i]>newKey) { // move as chaves
        keys[i+1] = keys[i];
        pointers[i+2] = pointers[i+1];
        i--;
    } i++;
    keys[i+1] = newKey; // insere nova chave
    if (ehFolha){
        pointers[i+1] = pointers[i+1];
        pointers[i] = newPointerE;
    }
    else {
        // o esquerdo continua apontando pro no original e o direito aponta para o novo
        pointers[i] = newPointerE;
        pointers[i+1] = newPointerD;
    }
    (*qtdKeys)++;

    reescrever(file, atual, noAtual);
}

ptr cisao(fstream *file, ptr atual, no *noAtual, ptr novo, no *novoNo, key chave, ptr ponteiro, bool ehFolha, ptr ponteiroD=-1){

    inserirCP(file, atual, noAtual, chave, ponteiro, ehFolha, ponteiroD);

    int tam = noAtual->qtdKeys;
    int cis = ceil(tam/2.0);

    for(int i=cis;i<tam;i++){ // copia pro novo nó/folha
        novoNo->keys[i-cis] = noAtual->keys[i];
        novoNo->ponteiros[i-cis+1] = noAtual->ponteiros[i+1];
    }
    noAtual->ponteiros[0] = novoNo->ponteiros[cis];

    novoNo->qtdKeys = ceil(tam/2);
    noAtual->qtdKeys = tam - ceil(tam/2);
    if(!ehFolha) noAtual->qtdKeys--; // se nó interno, "remove" chave promovida
    

    if(ehFolha) noAtual->ponteiros[noAtual->qtdKeys] = novo; // atualiza último ponteiro para a nova folha;
    reescrever(file, atual, noAtual); // atualiza noAtual
    reescrever(file, novo, novoNo); // atualiza noAtual

    return novo;
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

            for(int i=0;i<noAtual.qtdKeys;i++){
                cout << noAtual.keys[i] << " ";
            } cout << "\n";

            if (noAtual.folha) break;

            int i = lower_bound(noAtual.keys, noAtual.keys + noAtual.qtdKeys, alvo) - noAtual.keys;
            i = max(i, 0); // caso seja o menor do bloco;
            atual = noAtual.ponteiros[i];
        }

        no folha;
        carregar(file, atual, &folha);

        // procurar valor na folha
        int i = lower_bound(folha.keys, folha.keys + folha.qtdKeys, alvo)-folha.keys;
        if (i < folha.qtdKeys && folha.keys[i] == alvo)
            return folha.ponteiros[i];
        else
            return -1;

        if (i < folha.qtdKeys && i >= 0 && folha.keys[i] == alvo) return folha.ponteiros[i];
        else return -1; // não encontrou
    }

    void inserir(key chave, ptr ponteiro){

        ptr atual = raiz, ponteiroE;
        stack<ptr> pilha; // guarda os nós intermadiários, caso precise de cisão
        no noAtual;

        while(true){ // desce até encontrar folha
            carregar(file, atual, &noAtual);
            if(noAtual.folha) break;

            // for(int i=0;i<noAtual.qtdKeys;i++){
            //     cout << noAtual.keys[i] << " ";
            // } cout << "\n";

            pilha.push(atual);
            int i = lower_bound(noAtual.keys, noAtual.keys + noAtual.qtdKeys, chave) - noAtual.keys;
            i = max(i, 0); // caso seja o menor do bloco;

            atual = noAtual.ponteiros[i];
        }
        
        if(noAtual.qtdKeys < M){ // há espaço na folha
            inserirCP(file, atual, &noAtual, chave, ponteiro, true);
            return;
        }
        else{ // não há espaço na folha
            // divide folha atual no meio, guarda ponteiro da nova folha e atualiza chave e ponteiro atuais
            no novoNo;
            novoNo.folha = true;
            novoNo.qtdKeys = 0;
            ptr novo = escrever(file, &novoNo); // escreve novoNo no arquivo
            ponteiro = cisao(file, atual, &noAtual, novo, &novoNo, chave, ponteiro, true);
            chave = noAtual.keys[noAtual.qtdKeys];
            ponteiroE = atual;
        }

        // adicionar chave nova à algum nó interno
        while(!pilha.empty()){
            atual = pilha.top(); pilha.pop();
            carregar(file, atual, &noAtual);
            
            if(noAtual.qtdKeys<M){ // há espaço no nó interno
                inserirCP(file, atual, &noAtual, chave, ponteiroE, false, ponteiro);
                return;
            }
            else { // não há espaço no nó interno
                // divide nó atual, guarda ponteiro do novo nó e atualiza chave e ponteiro atuais
                no novoNo;
                novoNo.folha = true;
                novoNo.qtdKeys = 0;
                ptr novo = escrever(file, &novoNo); // escreve novoNo no arquivo
                ponteiro = cisao(file, atual, &noAtual, novo, &novoNo, chave, ponteiro, false, novo);
                chave = noAtual.keys[noAtual.qtdKeys-1];
                ponteiroE = atual;
            }

            if(pilha.empty()) break; // chegou na raíz
        }

        if(pilha.empty()){ // chegou na raíz
            no novaRaiz;
            novaRaiz.folha = false;
            novaRaiz.keys[0] = chave;
            novaRaiz.ponteiros[0] = ponteiroE;
            novaRaiz.ponteiros[1] = ponteiro;
            novaRaiz.qtdKeys = 1;
            this->raiz = escrever(file, &novaRaiz);
        }
    }
};

int main(){
    bp arvore;
    fstream file("dados.bin", ios::in | ios::out | ios::binary | ios::trunc);
    
    if (!file.is_open()) {
        cerr << "Erro ao abrir/criar arquivo dados.bin" << endl;
        return 1;
    }

    arvore.iniciar(&file);
    ifstream arquivo("artigo.csv");
    string linha;
    
    int lim = 10;
    int i = 0;
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
                cout << id << " ";
                arvore.inserir(id, pt);
                
                // campos[1],    // campos[1] - título

            } catch (const exception& e) {
                cerr << "Erro ao processar linha: " << linha << endl;
                cerr << "Erro: " << e.what() << endl;
            }
        } cout << "\n";
    }

    cout << arvore.busca(10) << "\n";

    arquivo.close();
    return 0;
}