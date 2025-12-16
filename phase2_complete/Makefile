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
	@echo "Usage:"
	@echo "  ./$(SERVER) [port]              - Start server"
	@echo "  ./$(CLIENT) [ip] [port]         - Start client"

$(SERVER): $(SERVER_SRC) $(HEADERS)
	@echo "🔨 Building Server..."
	$(CC) $(ALL_CFLAGS) -o $(SERVER) $(SERVER_SRC) $(ALL_LIBS)
	@echo "✅ Server built"

$(CLIENT): $(CLIENT_SRC) $(HEADERS)
	@echo "🔨 Building Client..."
	$(CC) $(ALL_CFLAGS) -o $(CLIENT) $(CLIENT_SRC) $(ALL_LIBS)
	@echo "✅ Client built"

clean:
	@echo "🧹 Cleaning..."
	rm -f $(SERVER) $(CLIENT)
	@echo "✅ Clean complete"

rebuild: clean all

# 檢查 OpenSSL
check-openssl:
	@echo "📋 Checking OpenSSL..."
ifeq ($(UNAME_S),Darwin)
	@brew list openssl >/dev/null 2>&1 || (echo "❌ Install: brew install openssl" && exit 1)
else
	@(pkg-config --exists openssl 2>/dev/null || [ -f /usr/include/openssl/evp.h ]) || \
		(echo "❌ Install: sudo apt-get install libssl-dev" && exit 1)
endif
	@echo "✅ OpenSSL found"

# 檢查所有檔案
check-deps: check-openssl
	@echo "📋 Checking files..."
	@for f in $(HEADERS) $(SERVER_SRC) $(CLIENT_SRC); do \
		[ -f "$$f" ] || (echo "❌ Missing: $$f" && exit 1); \
	done
	@echo "✅ All files present"

# 執行
run-server: $(SERVER)
	./$(SERVER) 8080

run-client: $(CLIENT)
	./$(CLIENT) 127.0.0.1 8080

# 測試
test: $(SERVER) $(CLIENT)
	@echo "🧪 Ready for testing"
	@echo "1. Start server:  ./$(SERVER) 8080"
	@echo "2. Start client:  ./$(CLIENT) 127.0.0.1 8080"

# 幫助
help:
	@echo "=== Phase 2 Complete Makefile ==="
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build everything (default)"
	@echo "  clean        - Remove binaries"
	@echo "  rebuild      - Clean and rebuild"
	@echo "  check-deps   - Check dependencies"
	@echo "  run-server   - Run server on port 8080"
	@echo "  run-client   - Run client"
	@echo "  test         - Show test instructions"
	@echo "  help         - Show this help"
	@echo ""
	@echo "OpenSSL Installation:"
	@echo "  macOS:  brew install openssl"
	@echo "  Ubuntu: sudo apt-get install libssl-dev"

.PHONY: all clean rebuild check-openssl check-deps run-server run-client test help
