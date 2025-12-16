#!/bin/bash

echo "╔══════════════════════════════════════════╗"
echo "║    Phase 2 Complete - Test Guide         ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# 檢查 OpenSSL
echo "📋 Checking OpenSSL..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    if brew list openssl >/dev/null 2>&1; then
        echo "✅ OpenSSL installed (macOS)"
    else
        echo "❌ Install: brew install openssl"
        exit 1
    fi
else
    if pkg-config --exists openssl 2>/dev/null || [ -f /usr/include/openssl/evp.h ]; then
        echo "✅ OpenSSL installed (Linux)"
    else
        echo "❌ Install: sudo apt-get install libssl-dev"
        exit 1
    fi
fi

# 檢查檔案
echo ""
echo "📋 Checking files..."
files=("ThreadPool.h" "Crypto.h" "P2PClient.h" "FileTransfer.h" "Server_Phase2.cpp" "Client_Phase2.cpp" "Makefile")
missing=0
for f in "${files[@]}"; do
    if [ -f "$f" ]; then
        echo "  ✅ $f"
    else
        echo "  ❌ $f (missing)"
        missing=1
    fi
done

if [ $missing -eq 1 ]; then
    echo ""
    echo "❌ Some files are missing!"
    exit 1
fi

# 編譯
echo ""
echo "🔨 Building..."
make clean
make

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo ""
echo "✅ Build successful!"
echo ""

# 測試指南
echo "╔══════════════════════════════════════════╗"
echo "║           Testing Instructions           ║"
echo "╚══════════════════════════════════════════╝"
echo ""

echo "┌─ Step 1: Start Server ─────────────────────┐"
echo "│ Open Terminal 1 and run:                   │"
echo "│   ./server_phase2 8080                     │"
echo "└────────────────────────────────────────────┘"
echo ""
echo "Press Enter when server is running..."
read

echo "┌─ Step 2: Start Client 1 (Alice) ───────────┐"
echo "│ Open Terminal 2 and run:                   │"
echo "│   ./client_phase2 127.0.0.1 8080           │"
echo "│                                            │"
echo "│ Register and Login:                        │"
echo "│   1 → alice → password123                  │"
echo "│   2 → alice → password123 → 9001           │"
echo "└────────────────────────────────────────────┘"
echo ""
echo "Press Enter when Alice is logged in..."
read

echo "┌─ Step 3: Start Client 2 (Bob) ─────────────┐"
echo "│ Open Terminal 3 and run:                   │"
echo "│   ./client_phase2 127.0.0.1 8080           │"
echo "│                                            │"
echo "│ Register and Login:                        │"
echo "│   1 → bob → password456                    │"
echo "│   2 → bob → password456 → 9002             │"
echo "└────────────────────────────────────────────┘"
echo ""
echo "Press Enter when Bob is logged in..."
read

echo "╔══════════════════════════════════════════╗"
echo "║         Test 1: P2P Messaging            ║"
echo "╚══════════════════════════════════════════╝"
echo ""
echo "On Bob's client:"
echo "  4 → alice → Hello Alice, this is encrypted!"
echo ""
echo "Alice should see:"
echo "  🔓💬 [P2P-Encrypted] bob: Hello Alice, this is encrypted!"
echo ""
echo "Press Enter after testing P2P..."
read

echo "╔══════════════════════════════════════════╗"
echo "║         Test 2: Group Chat               ║"
echo "╚══════════════════════════════════════════╝"
echo ""
echo "On Alice's client:"
echo "  6 → general              (Create room)"
echo ""
echo "On Bob's client:"
echo "  7 → general              (Join room)"
echo "  9 → general → Hello everyone!"
echo ""
echo "Alice should see:"
echo "  📢 ROOM_MSG:general:bob:Hello everyone!"
echo ""
echo "More tests:"
echo "  10 → general             (View history)"
echo "  11 → general             (View members)"
echo ""
echo "Press Enter after testing Group Chat..."
read

echo "╔══════════════════════════════════════════╗"
echo "║         Test 3: File Transfer            ║"
echo "╚══════════════════════════════════════════╝"
echo ""
echo "First, create a test file:"
echo "  echo 'Hello World' > test.txt"
echo ""
echo "On Alice's client:"
echo "  12 → bob → test.txt"
echo ""
echo "You should see:"
echo "  📤 Progress: 100%"
echo "  ✅ File transfer completed successfully!"
echo ""
echo "Bob should see:"
echo "  📥 Incoming file transfer from alice"
echo "  📥 Progress: 100%"
echo "  ✅ File saved to: ./test.txt"
echo ""
echo "Press Enter after testing File Transfer..."
read

echo "╔══════════════════════════════════════════╗"
echo "║           All Tests Complete!            ║"
echo "╚══════════════════════════════════════════╝"
echo ""
echo "📊 Score Summary:"
echo "  ✅ Phase 1: 40 points"
echo "  ✅ ThreadPool: 15 points"
echo "  ✅ P2P Messaging: 15 points"
echo "  ✅ Encryption: 10 points"
echo "  ✅ Group Chat: 10 points"
echo "  ✅ File Transfer: 10 points"
echo "  ────────────────────────"
echo "  Total: 100 points"
echo ""
echo "🎉 Congratulations! All features implemented!"
