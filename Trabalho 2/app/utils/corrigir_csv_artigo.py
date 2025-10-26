import csv
import re
import gzip
import os

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "..", "data")

ARQUIVO_ENTRADA = os.path.join(DATA_DIR, "artigo.csv.gz")
ARQUIVO_SAIDA = os.path.join(DATA_DIR, "artigo_corrigido.csv")
ARQUIVO_LOG = os.path.join(DATA_DIR, "erros_csv.log")


def abrir_arquivo(entrada):
    """Abre o arquivo normalmente ou descompacta se for .gz"""
    if entrada.endswith(".gz"):
        return gzip.open(entrada, "rt", encoding="utf-8", errors="replace")
    return open(entrada, "r", encoding="utf-8", errors="replace")


def corrigir_csv(entrada, saida, log, num_campos=7):
    with abrir_arquivo(entrada) as f_in, \
         open(saida, "w", encoding="utf-8", newline="\n") as f_out, \
         open(log, "w", encoding="utf-8") as f_log:

        total = 0
        erros = 0

        # Normaliza CRLF → LF durante a leitura
        linhas_normalizadas = (linha.replace("\r\n", "\n").replace("\r", "\n") for linha in f_in)
        leitor = csv.reader(linhas_normalizadas, delimiter=";", quotechar='"')

        for linha in leitor:
            total += 1

            # Remove aspas extras
            linha = [campo.strip('"') for campo in linha]

            # Corrige linhas incompletas
            if len(linha) != num_campos:
                texto = ";".join(linha)
                texto = re.sub(r"\s+", " ", texto)
                partes = texto.split(";")

                if len(partes) > num_campos:
                    partes = partes[:num_campos]
                elif len(partes) < num_campos:
                    partes += [""] * (num_campos - len(partes))

                if len(partes) != num_campos:
                    erros += 1
                    f_log.write(f"Linha {total} com erro: {linha}\n")
                    continue

                linha = partes

            # Grava linha manualmente em formato Unix (LF)
            f_out.write(";".join(linha) + "\n")

        print(f"✅ Corrigido! Linhas processadas: {total}")
        print(f"🧹 Linhas recuperadas/corrigidas: {total - erros}")
        print(f"⚠️ Linhas ainda problemáticas: {erros}")
        print(f"📁 Arquivo corrigido salvo em: {saida}")
        print(f"📜 Log de erros: {log}")


if __name__ == "__main__":
    corrigir_csv(ARQUIVO_ENTRADA, ARQUIVO_SAIDA, ARQUIVO_LOG)
