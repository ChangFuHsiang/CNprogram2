# (基於原始檔案修改)
CC = g++
CFLAGS = -std=c++14 -Wall -Wextra -g -pthread -O0

# OpenSSL
OPENSSL_LIBS = -lssl -lcrypto

# 作業系統檢測
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -D_DARWIN_C_SOURCE
    OPENSSL_PREFIX = $(shell brew --prefix openssl 2>/dev/null || echo "/usr/local/opt/openssl")
    OPENSSL_CFLAGS = -I$(OPENSSL_PREFIX)/include
    OPENSSL_LIBS = -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto
else
    CFLAGS += -D_GNU_SOURCE
    OPENSSL_CFLAGS =
endif

ALL_CFLAGS = $(CFLAGS) $(OPENSSL_CFLAGS)
ALL_LIBS = $(OPENSSL_LIBS)

# 目標檔案
SERVER = server_phase2
CLIENT = client_phase2

# 源檔案
SERVER_SRC = Server_Phase2.cpp
CLIENT_SRC = Client_Phase2.cpp

# 標頭檔
HEADERS = ThreadPool.h Crypto.h P2PClient.h FileTransfer.h

# 預設目標
all: $(SERVER) $(CLIENT)
	@echo ""
	@echo "╔══════════════════════════════════════════╗"
	@echo "║     Build Complete - Phase 2 Full        ║"
	@echo "╠══════════════════════════════════════════╣"
	@echo "║ Features:                                ║"
	@echo "║  ✅ ThreadPool (10 workers)              ║"
	@echo "║  ✅ P2P Direct Messaging                 ║"
	@echo "║  ✅ OpenSSL Encryption (AES-256-CBC)     ║"
	@echo "║  ✅ Group Chat (Relay Mode)              ║"
	@echo "║  ✅ File Transfer (Encrypted)            ║"
	@echo "╚══════════════════════════════════════════╝"
	@echo ""
	@echo "🚀 Quick Start:"
	@echo "  1. make setup           - Create Alice/Bob test env"
	@echo "  2. make run-server      - Start server"
	@echo "  3. make run-alice       - Start client Alice (Sender)"
	@echo "  4. make run-bob         - Start client Bob (Receiver)"

$(SERVER): $(SERVER_SRC) $(HEADERS)
	@echo "🔨 Building Server..."
	$(CC) $(ALL_CFLAGS) -o $(SERVER) $(SERVER_SRC) $(ALL_LIBS)
	@echo "✅ Server built"

$(CLIENT): $(CLIENT_SRC) $(HEADERS)
	@echo "🔨 Building Client..."
	$(CC) $(ALL_CFLAGS) -o $(CLIENT) $(CLIENT_SRC) $(ALL_LIBS)
	@echo "✅ Client built"

clean:
	@echo "🧹 Cleaning binaries..."
	rm -f $(SERVER) $(CLIENT)
	@echo "✅ Clean complete"

# === ✨ 新增：自動化測試環境設置 ===

# 建立獨立的測試資料夾，防止檔案覆蓋
setup: all
	@echo "📦 Setting up test environment..."
	@mkdir -p alice_dir
	@mkdir -p bob_dir
	@cp $(CLIENT) alice_dir/
	@cp $(CLIENT) bob_dir/
	@echo "Hello World from Alice!" > alice_dir/test.txt
	@echo "✅ Environment ready:"
	@echo "   📂 alice_dir/ (contains test.txt)"
	@echo "   📂 bob_dir/"

# 清除測試環境
clean-env: clean
	@echo "🧹 Cleaning test environment..."
	rm -rf alice_dir bob_dir
	@echo "✅ Test environment removed"

rebuild: clean-env all

# 檢查依賴
check-deps:
	@echo "📋 Checking OpenSSL..."
ifeq ($(UNAME_S),Darwin)
	@brew list openssl >/dev/null 2>&1 || (echo "❌ Install: brew install openssl" && exit 1)
else
	@(pkg-config --exists openssl 2>/dev/null || [ -f /usr/include/openssl/evp.h ]) || \
		(echo "❌ Install: sudo apt-get install libssl-dev" && exit 1)
endif
	@echo "✅ OpenSSL found"
	@echo "📋 Checking files..."
	@for f in $(HEADERS) $(SERVER_SRC) $(CLIENT_SRC); do \
		[ -f "$$f" ] || (echo "❌ Missing: $$f" && exit 1); \
	done
	@echo "✅ All files present"

# === 🚀 執行指令 (修改版) ===

run-server: $(SERVER)
	@echo "🟢 Starting Server on port 8080..."
	./$(SERVER) 8080

# 在 alice_dir 中執行 Client
run-alice:
	@if [ ! -d "alice_dir" ]; then echo "⚠️ Please run 'make setup' first"; exit 1; fi
	@echo "👤 Starting Alice..."
	@cd alice_dir && ./$(CLIENT) 127.0.0.1 8080

# 在 bob_dir 中執行 Client
run-bob:
	@if [ ! -d "bob_dir" ]; then echo "⚠️ Please run 'make setup' first"; exit 1; fi
	@echo "👤 Starting Bob..."
	@cd bob_dir && ./$(CLIENT) 127.0.0.1 8080

.PHONY: all clean rebuild check-deps run-server run-alice run-bob setup clean-env