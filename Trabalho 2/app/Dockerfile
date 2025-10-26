# imagem base leve (Debian 12)
FROM debian:bookworm-slim

# instala compilador C++, ferramentas e Python com pandas
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ make cmake python3 python3-pip python3-pandas \
    && rm -rf /var/lib/apt/lists/*

# define diretório principal da aplicação
WORKDIR /app

# copia todo o conteúdo do projeto para dentro do container
COPY . /app

# compila o projeto (usa o Makefile)
RUN make build

# define volume onde ficam os dados (compartilhado com o host)
VOLUME ["/data"]

# define variáveis de ambiente úteis
ENV CSV_PATH=/data/input.csv \
    DATA_DIR=/data/db \
    LOG_LEVEL=info

# comando padrão exibindo instrução de uso
CMD ["bash", "-lc", "echo 'Use: docker run --rm -v $(pwd)/data:/data tp2 ./bin/upload /data/input.csv'"]
