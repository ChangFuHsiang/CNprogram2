CC = g++
CFLAGS = -std=c++14 -Wall -Wextra -g -pthread -O0

# 檢測作業系統並設定適當的編譯旗標
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS 特定設定
    CFLAGS += -D_DARWIN_C_SOURCE
else
    # Linux 特定設定  
    CFLAGS += -D_GNU_SOURCE
endif

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

# 預設目標：編譯Phase 2版本
all: phase2

# Phase 2版本
phase2: $(SERVER_P2) $(CLIENT_P2)
	@echo "✅ Phase 2 build completed"
	@echo "Server: ./$(SERVER_P2) [port]"
	@echo "Client: ./$(CLIENT_P2) [server_ip] [server_port]"

# Phase 1版本（備用）
phase1: $(SERVER_P1) $(CLIENT_P1)
	@echo "✅ Phase 1 build completed"
	@echo "Server: ./$(SERVER_P1) [port]"
	@echo "Client: ./$(CLIENT_P1) [server_ip] [server_port]"

# 編譯Phase 2 server（依賴ThreadPool.h）
$(SERVER_P2): $(SERVER_P2_SRC) $(THREADPOOL_HEADER)
	@echo "🔨 Building Phase 2 Server with ThreadPool..."
	$(CC) $(CFLAGS) -o $(SERVER_P2) $(SERVER_P2_SRC)

# 編譯Phase 2 client（依賴P2PClient.h）
$(CLIENT_P2): $(CLIENT_P2_SRC) $(P2P_HEADER)
	@echo "🔨 Building Phase 2 Client with P2P support..."
	$(CC) $(CFLAGS) -o $(CLIENT_P2) $(CLIENT_P2_SRC)

# 編譯Phase 1 server（備用）
$(SERVER_P1): $(SERVER_P1_SRC)
	@echo "🔨 Building Phase 1 Server (backup)..."
	$(CC) $(CFLAGS) -o $(SERVER_P1) $(SERVER_P1_SRC)

# 編譯Phase 1 client（備用）
$(CLIENT_P1): $(CLIENT_P1_SRC)
	@echo "🔨 Building Phase 1 Client (backup)..."
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

# 檢查依賴
check-deps:
	@echo "📋 Checking dependencies..."
	@if [ ! -f $(THREADPOOL_HEADER) ]; then echo "❌ Missing: $(THREADPOOL_HEADER)"; exit 1; fi
	@if [ ! -f $(P2P_HEADER) ]; then echo "❌ Missing: $(P2P_HEADER)"; exit 1; fi
	@if [ ! -f $(SERVER_P2_SRC) ]; then echo "❌ Missing: $(SERVER_P2_SRC)"; exit 1; fi
	@if [ ! -f $(CLIENT_P2_SRC) ]; then echo "❌ Missing: $(CLIENT_P2_SRC)"; exit 1; fi
	@echo "✅ All Phase 2 dependencies found"

# 測試編譯
test-compile: check-deps
	@echo "🧪 Test compiling Phase 2..."
	$(CC) $(CFLAGS) -fsyntax-only $(SERVER_P2_SRC)
	$(CC) $(CFLAGS) -fsyntax-only $(CLIENT_P2_SRC)
	@echo "✅ Syntax check passed"

# 顯示幫助
help:
	@echo "=== Phase 2 Socket Programming Makefile ==="
	@echo ""
	@echo "Main targets:"
	@echo "  phase2      - Build Phase 2 version (default)"
	@echo "  phase1      - Build Phase 1 version (backup)"
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
	@echo "  check-deps     - Check if all files exist"
	@echo "  test-compile   - Test syntax without building"
	@echo ""
	@echo "Features in Phase 2:"
	@echo "  ✅ Professional ThreadPool (10 workers)"
	@echo "  ✅ Enhanced concurrency handling"
	@echo "  ✅ P2P discovery support (GET_USER_INFO)"
	@echo "  ✅ Direct P2P messaging (P2P_CHAT)"
	@echo "  ✅ Real-time message receiving"
	@echo "  ✅ Detailed logging and monitoring"

.PHONY: all phase1 phase2 clean clean-p1 clean-p2 rebuild rebuild-p1 run-server-p2 run-client-p2 run-server-p1 run-client-p1 check-deps test-compile help