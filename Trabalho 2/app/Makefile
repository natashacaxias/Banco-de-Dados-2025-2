

# Compilador e flags
CXX = g++
CXXFLAGS = -O2 -std=c++17 -I./include

# Diretórios
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
UTILS_DIR = utils
DATA_DIR = data

# Programas principais
PROGS = upload findrec seek1 seek2

# ==========================================================
# 🧩 1. BUILD LOCAL (no host)
# ==========================================================

# Compila todos os programas principais
build: $(PROGS)
	@echo "✅ Compilação local concluída."

# Regras individuais
upload: $(SRC_DIR)/upload.cpp $(SRC_DIR)/hashfile.cpp $(INC_DIR)/bptreefile.h $(INC_DIR)/common.h
	$(CXX) $(CXXFLAGS) $^ -o $(BIN_DIR)/$@

findrec: $(SRC_DIR)/findrec.cpp $(SRC_DIR)/hashfile.cpp
	$(CXX) $(CXXFLAGS) $^ -o $(BIN_DIR)/$@

seek1: $(SRC_DIR)/seek1.cpp $(SRC_DIR)/hashfile.cpp
	$(CXX) $(CXXFLAGS) $^ -o $(BIN_DIR)/$@

seek2: $(SRC_DIR)/seek2.cpp $(SRC_DIR)/hashfile.cpp
	$(CXX) $(CXXFLAGS) $^ -o $(BIN_DIR)/$@

# Utilitários auxiliares
build-utils:
	$(CXX) $(CXXFLAGS) $(UTILS_DIR)/calcularM.cpp -o $(UTILS_DIR)/calcularM
	@echo "✅ Utilitário calcularM compilado."

# ==========================================================
# 🧩 2. EXECUÇÃO LOCAL (sem Docker)
# ==========================================================

run-calcularM:
	@echo "🔹 Calculando ordem M da B+Tree..."
	@$(UTILS_DIR)/calcularM

run-corrigir:
	@echo "🔹 Corrigindo CSV de entrada..."
	python3 $(UTILS_DIR)/corrigir_csv_artigo.py

run-upload:
	@echo "🚀 Executando upload (HashFile + B+Tree prim/seg)..."
	./$(BIN_DIR)/upload $(DATA_DIR)/artigo_corrigido.csv

run-findrec:
	@read -p "Digite o ID: " ID; ./$(BIN_DIR)/findrec $$ID

run-seek1:
	@read -p "Digite o ID: " ID; ./$(BIN_DIR)/seek1 $$ID

run-seek2:
	@read -p "Digite o Título: " TIT; ./$(BIN_DIR)/seek2 "$$TIT"

# ==========================================================
# 🐳 3. DOCKER BUILD E EXECUÇÃO
# ==========================================================

docker-build:
	@echo "🐳 Construindo imagem Docker 'tp2'..."
	docker build -t tp2 .

# Executa cada programa dentro do container (usa /data montado)
docker-run-calcularM:
	docker run --rm -v "$(PWD)/data:/data" tp2 ./utils/calcularM

docker-run-corrigir:
	docker run --rm -v "$(PWD)/data:/data" tp2 python3 ./utils/corrigir_csv_artigo.py

docker-run-upload:
	docker run --rm -v "$(PWD)/data:/data" tp2 ./bin/upload /data/artigo_corrigido.csv

docker-run-findrec:
	@read -p "Digite o ID: " ID; docker run --rm -v "$(PWD)/data:/data" tp2 ./bin/findrec $$ID

docker-run-seek1:
	@read -p "Digite o ID: " ID; docker run --rm -v "$(PWD)/data:/data" tp2 ./bin/seek1 $$ID

docker-run-seek2:
	@read -p "Digite o Título: " TIT; docker run --rm -v "$(PWD)/data:/data" tp2 ./bin/seek2 "$$TIT"

# ==========================================================
# 🧹 4. LIMPEZA
# ==========================================================

clean:
	@echo "🧹 Limpando binários e arquivos temporários..."
	rm -f $(BIN_DIR)/* $(UTILS_DIR)/*.exe
	@echo "Limpo."

# ==========================================================
# 🔎 AJUDA
# ==========================================================

help:
	@echo "Comandos disponíveis:"
	@echo "  make build              -> Compila os binários locais"
	@echo "  make build-utils        -> Compila calcularM"
	@echo "  make run-calcularM      -> Mostra M_ID e M_TITULO"
	@echo "  make run-corrigir       -> Corrige CSV"
	@echo "  make run-upload         -> Executa upload local"
	@echo "  make run-findrec        -> Busca por ID nos dados"
	@echo "  make run-seek1          -> Busca por ID no índice primário"
	@echo "  make run-seek2          -> Busca por título no índice secundário"
	@echo ""
	@echo "  make docker-build       -> Constrói imagem Docker"
	@echo "  make docker-run-upload  -> Executa upload dentro do container"
	@echo "  make docker-run-findrec -> Executa findrec no container"
	@echo ""
	@echo "  make clean              -> Limpa binários"
