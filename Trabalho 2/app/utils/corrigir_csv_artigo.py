import csv
import re
import gzip
import os

ARQUIVO_ENTRADA = "../data/artigo.csv.gz"   # agora pode ser .csv ou .csv.gz
ARQUIVO_SAIDA = "../data/artigo_corrigido.csv"
ARQUIVO_LOG = "../data/erros_csv.log"

def abrir_arquivo(entrada):
    """Abre o arquivo normalmente ou descompacta se for .gz"""
    if entrada.endswith(".gz"):
        return gzip.open(entrada, 'rt', encoding='utf-8', errors='replace')
    else:
        return open(entrada, 'r', encoding='utf-8', errors='replace')

def corrigir_csv(entrada, saida, log, num_campos=7):
    with abrir_arquivo(entrada) as f_in, \
         open(saida, 'w', newline='', encoding='utf-8') as f_out, \
         open(log, 'w', encoding='utf-8') as f_log:

        leitor = csv.reader(f_in, delimiter=';', quotechar='"')
        escritor = csv.writer(f_out, delimiter=';', quotechar='"', quoting=csv.QUOTE_ALL)

        total = 0
        erros = 0

        for linha in leitor:
            total += 1
            if len(linha) == num_campos:
                escritor.writerow(linha)
                continue

            # 🧩 Tenta reparar a linha
            texto = ";".join(linha)
            texto = re.sub(r'\s+', ' ', texto)
            partes = texto.split(';')

            if len(partes) > num_campos:
                partes = partes[:num_campos]
            elif len(partes) < num_campos:
                partes += [''] * (num_campos - len(partes))

            if len(partes) != num_campos:
                erros += 1
                f_log.write(f"Linha {total} com erro: {linha}\n")
                continue

            escritor.writerow(partes)

        print(f"✅ Corrigido! Linhas processadas: {total}")
        print(f"🧹 Linhas recuperadas/corrigidas: {total - erros}")
        print(f"⚠️ Linhas ainda problemáticas: {erros}")
        print(f"📁 Arquivo corrigido salvo em: {saida}")
        print(f"📜 Log de erros: {log}")

if __name__ == "__main__":
    corrigir_csv(ARQUIVO_ENTRADA, ARQUIVO_SAIDA, ARQUIVO_LOG)
