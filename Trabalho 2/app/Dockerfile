FROM debian:bookworm-slim

# Instala compiladores + Python + pandas
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ make cmake python3 python3-pip python3-pandas \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app
RUN make build

VOLUME ["/data"]

ENV CSV_PATH=/data/input.csv \
    DATA_DIR=/data/db \
    LOG_LEVEL=info

CMD ["bash", "-lc", "echo 'Use: docker run --rm -v $(pwd)/data:/data tp2 ./bin/upload /data/input.csv'"]
