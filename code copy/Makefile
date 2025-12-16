CC = g++
CFLAGS = -std=c++14 -Wall -Wextra -g -pthread -O0

# OpenSSL 編譯和鏈接旗標
OPENSSL_CFLAGS = $(shell pkg-config --cflags openssl 2>/dev/null || echo "")
OPENSSL_LIBS = $(shell pkg-config --libs openssl 2>/dev/null || echo "-lssl -lcrypto")

# 檢測作業系統並設定適當的編譯旗標
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS 特定設定
    CFLAGS += -D_DARWIN_C_SOURCE
    # macOS Homebrew OpenSSL 路徑
    OPENSSL_PREFIX = $(shell brew --prefix openssl 2>/dev/null || echo "/usr/local/opt/openssl")
    OPENSSL_CFLAGS = -I$(OPENSSL_PREFIX)/include
    OPENSSL_LIBS = -L$(OPENSSL_PREFIX)/lib -lssl -lcrypto
else
    # Linux 特定設定  
    CFLAGS += -D_GNU_SOURCE
endif

# 合併所有旗標
ALL_CFLAGS = $(CFLAGS) $(OPENSSL_CFLAGS)
ALL_LIBS = $(OPENSSL_LIBS)

# Phase 2 目標檔案
SERVER_P2 = server_phase2
CLIENT_P2 = client_phase2

# Phase 1 目標檔案（保持向下相容）
SERVER_P1 = server
CLIENT_P1 = client

# 源檔案
SERVER_P2_SRC = Server_Phase2.cpp
CLIENT_P2_SRC = Client_Phase2.cpp
SERVER_P1_SRC = Server.cpp
CLIENT_P1_SRC = Client.cpp

# 依賴檔案
THREADPOOL_HEADER = ThreadPool.h
P2P_HEADER = P2PClient.h
CRYPTO_HEADER = Crypto.h

# 預設目標：編譯Phase 2版本（帶加密）
all: phase2

# Phase 2版本（帶加密）
phase2: check-openssl $(SERVER_P2) $(CLIENT_P2)
	@echo "✅ Phase 2 build completed (with OpenSSL encryption)"
	@echo "Server: ./$(SERVER_P2) [port]"
	@echo "Client: ./$(CLIENT_P2) [server_ip] [server_port]"

# Phase 1版本（備用，無加密）
phase1: $(SERVER_P1) $(CLIENT_P1)
	@echo "✅ Phase 1 build completed"
	@echo "Server: ./$(SERVER_P1) [port]"
	@echo "Client: ./$(CLIENT_P1) [server_ip] [server_port]"

# 編譯Phase 2 server（依賴ThreadPool.h和Crypto.h）
$(SERVER_P2): $(SERVER_P2_SRC) $(THREADPOOL_HEADER) $(CRYPTO_HEADER)
	@echo "🔨 Building Phase 2 Server with ThreadPool and OpenSSL..."
	$(CC) $(ALL_CFLAGS) -o $(SERVER_P2) $(SERVER_P2_SRC) $(ALL_LIBS)

# 編譯Phase 2 client（依賴P2PClient.h和Crypto.h）
$(CLIENT_P2): $(CLIENT_P2_SRC) $(P2P_HEADER) $(CRYPTO_HEADER)
	@echo "🔨 Building Phase 2 Client with P2P and OpenSSL..."
	$(CC) $(ALL_CFLAGS) -o $(CLIENT_P2) $(CLIENT_P2_SRC) $(ALL_LIBS)

# 編譯Phase 1 server（備用，無加密）
$(SERVER_P1): $(SERVER_P1_SRC)
	@echo "🔨 Building Phase 1 Server (backup, no encryption)..."
	$(CC) $(CFLAGS) -o $(SERVER_P1) $(SERVER_P1_SRC)

# 編譯Phase 1 client（備用，無加密）
$(CLIENT_P1): $(CLIENT_P1_SRC)
	@echo "🔨 Building Phase 1 Client (backup, no encryption)..."
	$(CC) $(CFLAGS) -o $(CLIENT_P1) $(CLIENT_P1_SRC)

# 清理所有編譯結果
clean:
	@echo "🧹 Cleaning up..."
	rm -f $(SERVER_P2) $(CLIENT_P2) $(SERVER_P1) $(CLIENT_P1)

# 只清理Phase 2
clean-p2:
	@echo "🧹 Cleaning Phase 2..."
	rm -f $(SERVER_P2) $(CLIENT_P2)

# 只清理Phase 1
clean-p1:
	@echo "🧹 Cleaning Phase 1..."
	rm -f $(SERVER_P1) $(CLIENT_P1)

# 重新編譯Phase 2
rebuild: clean phase2

# 重新編譯Phase 1
rebuild-p1: clean-p1 phase1

# 執行Phase 2 server
run-server-p2:
	@if [ ! -f $(SERVER_P2) ]; then echo "❌ Phase 2 server not built. Run 'make phase2' first."; exit 1; fi
	./$(SERVER_P2) 8080

# 執行Phase 2 client
run-client-p2:
	@if [ ! -f $(CLIENT_P2) ]; then echo "❌ Phase 2 client not built. Run 'make phase2' first."; exit 1; fi
	./$(CLIENT_P2) 127.0.0.1 8080

# 執行Phase 1 server（備用）
run-server-p1:
	@if [ ! -f $(SERVER_P1) ]; then echo "❌ Phase 1 server not built. Run 'make phase1' first."; exit 1; fi
	./$(SERVER_P1) 8080

# 執行Phase 1 client（備用）
run-client-p1:
	@if [ ! -f $(CLIENT_P1) ]; then echo "❌ Phase 1 client not built. Run 'make phase1' first."; exit 1; fi
	./$(CLIENT_P1) 127.0.0.1 8080

# 檢查OpenSSL是否安裝
check-openssl:
	@echo "📋 Checking OpenSSL installation..."
ifeq ($(UNAME_S),Darwin)
	@if ! brew list openssl >/dev/null 2>&1; then \
		echo "❌ OpenSSL not found. Install with: brew install openssl"; \
		exit 1; \
	fi
	@echo "✅ OpenSSL found at $(OPENSSL_PREFIX)"
else
	@if ! pkg-config --exists openssl 2>/dev/null; then \
		if [ ! -f /usr/include/openssl/evp.h ]; then \
			echo "❌ OpenSSL not found. Install with: sudo apt-get install libssl-dev"; \
			exit 1; \
		fi; \
	fi
	@echo "✅ OpenSSL found"
endif

# 檢查所有依賴
check-deps: check-openssl
	@echo "📋 Checking source dependencies..."
	@if [ ! -f $(THREADPOOL_HEADER) ]; then echo "❌ Missing: $(THREADPOOL_HEADER)"; exit 1; fi
	@if [ ! -f $(P2P_HEADER) ]; then echo "❌ Missing: $(P2P_HEADER)"; exit 1; fi
	@if [ ! -f $(CRYPTO_HEADER) ]; then echo "❌ Missing: $(CRYPTO_HEADER)"; exit 1; fi
	@if [ ! -f $(SERVER_P2_SRC) ]; then echo "❌ Missing: $(SERVER_P2_SRC)"; exit 1; fi
	@if [ ! -f $(CLIENT_P2_SRC) ]; then echo "❌ Missing: $(CLIENT_P2_SRC)"; exit 1; fi
	@echo "✅ All Phase 2 dependencies found"

# 測試編譯
test-compile: check-deps
	@echo "🧪 Test compiling Phase 2 with OpenSSL..."
	$(CC) $(ALL_CFLAGS) -fsyntax-only $(SERVER_P2_SRC)
	$(CC) $(ALL_CFLAGS) -fsyntax-only $(CLIENT_P2_SRC)
	@echo "✅ Syntax check passed"

# 測試加密功能
test-crypto: $(SERVER_P2) $(CLIENT_P2)
	@echo "🧪 Running encryption test..."
	@echo "Start server in another terminal: ./$(SERVER_P2) 8080"
	@echo "Then run client: ./$(CLIENT_P2) 127.0.0.1 8080"
	@echo "Encryption self-test runs automatically on startup"

# 顯示幫助
help:
	@echo "=== Phase 2 Socket Programming Makefile (with OpenSSL) ==="
	@echo ""
	@echo "Main targets:"
	@echo "  phase2      - Build Phase 2 with encryption (default)"
	@echo "  phase1      - Build Phase 1 without encryption (backup)"
	@echo "  clean       - Clean all build files"
	@echo "  rebuild     - Clean and rebuild Phase 2"
	@echo ""
	@echo "Running:"
	@echo "  run-server-p2  - Run Phase 2 server (port 8080)"
	@echo "  run-client-p2  - Run Phase 2 client"
	@echo "  run-server-p1  - Run Phase 1 server"
	@echo "  run-client-p1  - Run Phase 1 client"
	@echo ""
	@echo "Development:"
	@echo "  check-openssl  - Check OpenSSL installation"
	@echo "  check-deps     - Check if all files exist"
	@echo "  test-compile   - Test syntax without building"
	@echo "  test-crypto    - Instructions for encryption test"
	@echo ""
	@echo "Features in Phase 2:"
	@echo "  ✅ Professional ThreadPool (10 workers)"
	@echo "  ✅ P2P Direct Messaging"
	@echo "  ✅ OpenSSL Encryption (AES-256-CBC)"
	@echo "     - Client-Server encrypted communication"
	@echo "     - P2P encrypted messaging"
	@echo "  ✅ Detailed logging and monitoring"
	@echo ""
	@echo "OpenSSL Installation:"
	@echo "  macOS:  brew install openssl"
	@echo "  Ubuntu: sudo apt-get install libssl-dev"
	@echo "  CentOS: sudo yum install openssl-devel"

# 顯示編譯旗標（除錯用）
show-flags:
	@echo "CFLAGS: $(CFLAGS)"
	@echo "OPENSSL_CFLAGS: $(OPENSSL_CFLAGS)"
	@echo "OPENSSL_LIBS: $(OPENSSL_LIBS)"
	@echo "ALL_CFLAGS: $(ALL_CFLAGS)"
	@echo "ALL_LIBS: $(ALL_LIBS)"

.PHONY: all phase1 phase2 clean clean-p1 clean-p2 rebuild rebuild-p1 \
        run-server-p2 run-client-p2 run-server-p1 run-client-p1 \
        check-openssl check-deps test-compile test-crypto help show-flags