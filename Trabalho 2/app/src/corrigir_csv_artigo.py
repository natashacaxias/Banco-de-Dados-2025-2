import csv
import re

ARQUIVO_ENTRADA = "data/artigo_teste.csv"
ARQUIVO_SAIDA = "data/artigo_corrigido.csv"
ARQUIVO_LOG = "data/erros_csv.log"

def corrigir_csv(entrada, saida, log, num_campos=7):
    with open(entrada, 'r', encoding='utf-8', errors='replace') as f_in, \
         open(saida, 'w', newline='', encoding='utf-8') as f_out, \
         open(log, 'w', encoding='utf-8') as f_log:

        leitor = csv.reader(f_in, delimiter=';', quotechar='"')
        escritor = csv.writer(f_out, delimiter=';', quotechar='"', quoting=csv.QUOTE_ALL)

        buffer = []
        total = 0
        erros = 0

        for linha in leitor:
            total += 1
            if len(linha) == num_campos:
                escritor.writerow(linha)
                continue

            # 🧩 Se a linha tem campos demais/poucos, tenta reparar
            texto = ";".join(linha)
            texto = re.sub(r'\s+', ' ', texto)  # remove quebras de linha dentro do campo
            partes = texto.split(';')
            mx = max(mx, len(partes[1]))

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
        print(mx)

if __name__ == "__main__":
    corrigir_csv(ARQUIVO_ENTRADA, ARQUIVO_SAIDA, ARQUIVO_LOG)
