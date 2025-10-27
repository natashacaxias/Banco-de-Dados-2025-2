# ----------------------------------------------------------
# Imagem base recomendada (leve e compatível)
# ----------------------------------------------------------
FROM debian:bookworm-slim

# ----------------------------------------------------------
# Instala compilador C++ e Python 3 (para scripts utilitários)
# ----------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
      g++ make cmake python3 python3-minimal && \
    rm -rf /var/lib/apt/lists/*

# ----------------------------------------------------------
# Diretório de trabalho dentro do container
# ----------------------------------------------------------
WORKDIR /app

# Copia todos os arquivos do projeto
COPY . /app

# ----------------------------------------------------------
# Compila os programas C++ com Makefile
# ----------------------------------------------------------
RUN make build
RUN make build-utils
# ----------------------------------------------------------
# Diretório de dados persistentes (montado como volume)
# ----------------------------------------------------------
VOLUME ["/data"]

# ----------------------------------------------------------
# Variáveis de ambiente úteis
# ----------------------------------------------------------
# CSV_GZ        = arquivo original compactado (.gz)
# CSV_CORRIGIDO = arquivo limpo gerado pelo script Python
# DATA_DIR      = onde ficam data.db e índices .idx
# LOG_LEVEL     = nível de log dos binários (error|warn|info|debug)
ENV CSV_GZ=/data/artigo.csv.gz \
    CSV_CORRIGIDO=/data/artigo_corrigido.csv \
    DATA_DIR=/data \
    LOG_LEVEL=info

# ----------------------------------------------------------
# Comando padrão (orientativo para o avaliador)
# ----------------------------------------------------------
CMD ["bash", "-lc", "\
echo '=== TP2 – Banco de Dados I (UFAM) ==='; \
echo ''; \
echo 'Passo 1: python3 utils/corrigir_csv_artigo.py /data/artigo.csv.gz'; \
echo '         -> gera /data/artigo_corrigido.csv'; \
echo ''; \
echo 'Passo 2: ./bin/upload /data/artigo_corrigido.csv'; \
echo '         -> cria /data/data.db e índices B+'; \
echo ''; \
echo 'Outros comandos:'; \
echo '  ./bin/findrec <ID>                (busca direta no hash)'; \
echo '  ./bin/seek1 <ID>                  (busca via índice primário)'; \
echo '  ./bin/seek2 \"<Título>\"           (busca via índice secundário)'; \
echo ''; \
echo 'Binários disponíveis:'; \
ls -l bin/"]
