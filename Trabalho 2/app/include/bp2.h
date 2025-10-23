#ifndef BPTREEFILE_H
#define BPTREEFILE_H

#include <bits/stdc++.h>
#include "bp2.cpp" // ou ajuste o nome conforme teu include

// Aqui definimos apenas o tipo de chave e uma função utilitária para inicialização

// Estrutura de exemplo do registro (igual ao do hashfile)
struct Registro {
    int id;
    char titulo[300];
    int ano;
    char autores[150];
    int citacoes;
    char datahora[25];
    char snippet[1024];
};

#endif
