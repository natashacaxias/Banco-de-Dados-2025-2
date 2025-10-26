
# imagem base recomendada
FROM debian:bookworm-slim

# instala apenas o necessário para compilação
RUN apt-get update && apt-get install -y --no-install-recommends \
      g++ make cmake && \
    rm -rf /var/lib/apt/lists/*

# diretório de trabalho dentro do container
WORKDIR /app

# copia todos os arquivos do projeto
COPY . /app

# compila usando o Makefile padrão
RUN make build

# diretório para dados persistentes (montado em runtime)
VOLUME ["/data"]

# variáveis de ambiente para facilitar testes
# CSV_GZ          = arquivo original compactado
# CSV_CORRIGIDO   = arquivo limpo gerado pelo script corrigir_csv_artigo.py
# DATA_DIR        = onde ficam data.db e os índices .idx
# LOG_LEVEL       = nível de log desejado (error|warn|info|debug)
ENV CSV_GZ=/data/artigo.csv.gz \
    CSV_CORRIGIDO=/data/artigo_corrigido.csv \
    DATA_DIR=/data \
    LOG_LEVEL=info

# comando padrão (orientativo)
# conforme a especificação mínima, apenas exibe instruções
CMD ["bash", "-lc", "echo 'Passo 1: python3 utils/corrigir_csv_artigo.py   # gera /data/artigo_corrigido.csv a partir de /data/artigo.csv.gz'; echo 'Passo 2: ./bin/upload /data/artigo_corrigido.csv   # cria data.db e índices B+'; echo ''; echo 'Busca direta no hash:      ./bin/findrec <ID>'; echo 'Busca no índice primário:  ./bin/seek1 <ID>'; echo 'Busca no índice secundário ./bin/seek2 \"<TITULO>\"'; echo ''; echo 'Binários disponíveis:'; ls -l bin/"]
