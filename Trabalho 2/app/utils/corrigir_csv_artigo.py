import csv
import re
import gzip
import os
import sys

# ----------------------------------------------------------
# Caminhos base
# ----------------------------------------------------------
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "..", "data")

# Arquivos padrão (se não for passado argumento)
ARQUIVO_PADRAO = os.path.join(DATA_DIR, "artigo.csv.gz")
ARQUIVO_SAIDA = os.path.join(DATA_DIR, "artigo_corrigido.csv")
ARQUIVO_LOG = os.path.join(DATA_DIR, "erros_csv.log")


def abrir_arquivo(entrada):
    """Abre o arquivo normalmente ou descompacta se for .gz"""
    if entrada.endswith(".gz"):
        return gzip.open(entrada, "rt", encoding="utf-8", errors="replace")
    return open(entrada, "r", encoding="utf-8", errors="replace")


def corrigir_csv(entrada, saida, log, num_campos=7):
    """Corrige inconsistências em um CSV de artigos"""
    with abrir_arquivo(entrada) as f_in, \
         open(saida, "w", encoding="utf-8", newline="\n") as f_out, \
         open(log, "w", encoding="utf-8") as f_log:

        total = 0
        erros = 0

        # normaliza finais de linha (CRLF → LF)
        linhas_normalizadas = (
            linha.replace("\r\n", "\n").replace("\r", "\n") for linha in f_in
        )
        leitor = csv.reader(linhas_normalizadas, delimiter=";", quotechar='"')

        for linha in leitor:
            total += 1

            # remove aspas desnecessárias
            linha = [campo.strip('"') for campo in linha]

            # corrige linhas quebradas ou com número errado de campos
            if len(linha) != num_campos:
                texto = ";".join(linha)
                texto = re.sub(r"\s+", " ", texto)
                partes = texto.split(";")

                # ajusta quantidade de campos
                if len(partes) > num_campos:
                    partes = partes[:num_campos]
                elif len(partes) < num_campos:
                    partes += [""] * (num_campos - len(partes))

                # se ainda estiver incorreto, registra no log
                if len(partes) != num_campos:
                    erros += 1
                    f_log.write(f"Linha {total} com erro: {linha}\n")
                    continue

                linha = partes

            # grava linha corrigida
            f_out.write(";".join(linha) + "\n")

        # resumo no terminal
        print("\n=== Correção de CSV concluída ===")
        print(f"Arquivo de entrada: {entrada}")
        print(f"Linhas processadas: {total}")
        print(f"Linhas corrigidas: {total - erros}")
        print(f"Linhas ainda problemáticas: {erros}")
        print(f"Arquivo corrigido salvo em: {saida}")
        print(f"Log de erros salvo em: {log}")
        print("=================================\n")


if __name__ == "__main__":
    # ------------------------------------------------------
    # Aceita argumento opcional: nome do arquivo de entrada
    # ------------------------------------------------------
    if len(sys.argv) > 1:
        nome_arquivo = sys.argv[1]
        if not os.path.isabs(nome_arquivo):
            entrada = os.path.join(DATA_DIR, nome_arquivo)
        else:
            entrada = nome_arquivo
    else:
        entrada = ARQUIVO_PADRAO

    corrigir_csv(entrada, ARQUIVO_SAIDA, ARQUIVO_LOG)
