# ===========================================================
#  Dockerfile – TP2 Banco de Dados 1 (HashFile + Árvore B+)
# ===========================================================

FROM debian:bookworm-slim

# ------------------------------------------
# 1. Instala compiladores e Python
# ------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ make cmake python3 python3-pip gzip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

# ------------------------------------------
# 2. Compila binários principais e utilitários
# ------------------------------------------
RUN make build && make build-utils

# ------------------------------------------
# 3. Define volume compartilhado
# ------------------------------------------
VOLUME ["/data"]

# ------------------------------------------
# 4. Variáveis de ambiente padrão
# ------------------------------------------
ENV CSV_PATH=/data/artigo.csv.gz \
    CSV_CORRIGIDO=/data/artigo_corrigido.csv \
    DATA_DIR=/data \
    LOG_LEVEL=info

# ------------------------------------------
# 5. Comando padrão: corrige o CSV e faz upload
# ------------------------------------------
ENTRYPOINT ["bash", "-lc"]
CMD ["python3 utils/corrigir_csv_artigo.py && ./bin/upload /data/artigo_corrigido.csv"]

# ===========================================================
# Exemplo de uso:
# docker build -t tp2 .
# docker run --rm -v $(pwd)/data:/data -e LOG_LEVEL=debug tp2
# ===========================================================
