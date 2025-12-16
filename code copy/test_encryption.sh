#!/bin/bash

# Phase 2 OpenSSL Encryption 測試腳本
echo "=========================================="
echo "Phase 2 OpenSSL Encryption 測試"
echo "=========================================="
echo ""
echo "🔐 本次實作功能："
echo "  ✅ AES-256-CBC 對稱加密"
echo "  ✅ Client-Server 加密通訊"
echo "  ✅ P2P 加密訊息傳送"
echo "  ✅ Base64 編碼傳輸"
echo "  ✅ 自動加密/解密處理"
echo ""

# 檢測作業系統
OS=$(uname -s)
echo "📋 作業系統: $OS"

# 檢查OpenSSL
echo ""
echo "📋 檢查 OpenSSL 安裝..."
if [ "$OS" = "Darwin" ]; then
    # macOS
    if brew list openssl >/dev/null 2>&1; then
        OPENSSL_PATH=$(brew --prefix openssl)
        echo "✅ OpenSSL 已安裝: $OPENSSL_PATH"
    else
        echo "❌ OpenSSL 未安裝"
        echo "請執行: brew install openssl"
        exit 1
    fi
else
    # Linux
    if pkg-config --exists openssl 2>/dev/null || [ -f /usr/include/openssl/evp.h ]; then
        echo "✅ OpenSSL 已安裝"
    else
        echo "❌ OpenSSL 未安裝"
        echo "請執行: sudo apt-get install libssl-dev"
        exit 1
    fi
fi

# 檢查檔案
echo ""
echo "📁 檢查必要檔案..."
required_files=("Crypto.h" "P2PClient.h" "ThreadPool.h" "Server_Phase2.cpp" "Client_Phase2.cpp" "Makefile")
missing_files=()

for file in "${required_files[@]}"; do
    if [ ! -f "$file" ]; then
        missing_files+=("$file")
        echo "  ❌ $file"
    else
        echo "  ✅ $file"
    fi
done

if [ ${#missing_files[@]} -ne 0 ]; then
    echo ""
    echo "❌ 缺少以上檔案，請確保所有檔案都存在"
    exit 1
fi

echo ""
echo "✅ 所有必要檔案已就緒"

# 編譯
echo ""
echo "🔨 開始編譯..."
make clean
make phase2

if [ $? -ne 0 ]; then
    echo ""
    echo "❌ 編譯失敗"
    echo "請檢查錯誤訊息並修正"
    exit 1
fi

echo ""
echo "✅ 編譯成功！"

# 檢查執行檔
if [ ! -f "server_phase2" ] || [ ! -f "client_phase2" ]; then
    echo "❌ 執行檔未生成"
    exit 1
fi

echo ""
echo "=========================================="
echo "🧪 加密功能測試指南"
echo "=========================================="
echo ""
echo "步驟 1: 啟動 Server"
echo "-------------------------------------------"
echo "在 Terminal 1 執行:"
echo "  ./server_phase2 8080"
echo ""
echo "你應該看到:"
echo "  🧪 Running Crypto self-test..."
echo "  ✅ Crypto self-test passed!"
echo "  🔐 Server encryption enabled (AES-256-CBC)"
echo ""
echo "按 Enter 繼續..."
read

echo "步驟 2: 啟動 Client"
echo "-------------------------------------------"
echo "在 Terminal 2 執行:"
echo "  ./client_phase2 127.0.0.1 8080"
echo ""
echo "你應該看到:"
echo "  🧪 Running Crypto self-test..."
echo "  ✅ Crypto self-test passed!"
echo "  🔐 Client encryption enabled (AES-256-CBC)"
echo "  🔒 Server supports encryption - secure communication enabled"
echo ""
echo "按 Enter 繼續..."
read

echo "步驟 3: 測試加密的 Client-Server 通訊"
echo "-------------------------------------------"
echo "在 Client 中:"
echo "  1 → alice → password123  (註冊)"
echo ""
echo "觀察 Server 日誌，你應該看到:"
echo "  [Client X] Received: [REGISTER alice password123] (decrypted)"
echo "  [Client X] Sending: [REGISTER_SUCCESS] (encrypted)"
echo ""
echo "這表示:"
echo "  ✅ Client 發送的命令被加密"
echo "  ✅ Server 成功解密命令"
echo "  ✅ Server 回應也被加密"
echo "  ✅ Client 成功解密回應"
echo ""
echo "按 Enter 繼續..."
read

echo "步驟 4: 登入並啟動 P2P"
echo "-------------------------------------------"
echo "在 Client 中:"
echo "  2 → alice → password123 → 9001  (登入)"
echo ""
echo "你應該看到:"
echo "  🔐 P2P Encryption enabled (AES-256-CBC)"
echo "  ✅ P2P Listener started on port 9001"
echo "  🔒 All P2P messages will be encrypted"
echo ""
echo "按 Enter 繼續..."
read

echo "步驟 5: 啟動第二個 Client"
echo "-------------------------------------------"
echo "在 Terminal 3 執行:"
echo "  ./client_phase2 127.0.0.1 8080"
echo ""
echo "然後:"
echo "  1 → bob → password456  (註冊)"
echo "  2 → bob → password456 → 9002  (登入)"
echo ""
echo "按 Enter 繼續..."
read

echo "步驟 6: 測試加密 P2P 訊息"
echo "-------------------------------------------"
echo "在 Bob 的 Client 中:"
echo "  4 → alice → Hello Alice, this is encrypted!"
echo ""
echo "Bob 應該看到:"
echo "  📤 Sending P2P message to 127.0.0.1:9001 (encrypted)"
echo "  🔒 Message encrypted successfully"
echo "  ✅ P2P message delivered successfully (encrypted)"
echo ""
echo "Alice 應該看到:"
echo "  📨 P2P connection from: 127.0.0.1"
echo "  🔓💬 [P2P-Encrypted] bob: Hello Alice, this is encrypted!"
echo ""
echo "🎉 這證明 P2P 訊息被加密傳輸！"
echo ""
echo "按 Enter 繼續..."
read

echo "步驟 7: 測試雙向加密通訊"
echo "-------------------------------------------"
echo "Alice 回覆 Bob:"
echo "  4 → bob → Hi Bob, encryption works great!"
echo ""
echo "多發送幾條訊息，觀察:"
echo "  - 每條訊息都顯示 (encrypted) 標記"
echo "  - 接收方顯示 [P2P-Encrypted] 前綴"
echo "  - 訊息內容正確解密顯示"
echo ""
echo "按 Enter 繼續..."
read

echo "步驟 8: 測試加密開關"
echo "-------------------------------------------"
echo "在 Client 中可以切換加密:"
echo ""
echo "登入前:"
echo "  3 → Toggle Client-Server Encryption"
echo ""
echo "登入後:"
echo "  5 → Toggle P2P Encryption"
echo ""
echo "關閉加密後發送訊息，觀察:"
echo "  - 訊息不再顯示 (encrypted) 標記"
echo "  - 接收方顯示普通 [P2P] 前綴"
echo ""
echo "按 Enter 繼續..."
read

echo "=========================================="
echo "🎉 測試完成！"
echo "=========================================="
echo ""
echo "📊 加密功能總結："
echo ""
echo "✅ Client-Server 加密通訊"
echo "   - 所有命令自動加密發送"
echo "   - 所有回應自動解密接收"
echo "   - AES-256-CBC 對稱加密"
echo ""
echo "✅ P2P 加密訊息"
echo "   - P2P 訊息端到端加密"
echo "   - 訊息不經過 Server"
echo "   - 接收方自動解密"
echo ""
echo "✅ 安全特性"
echo "   - 每次加密使用隨機 IV"
echo "   - Base64 編碼傳輸"
echo "   - 加密失敗時有明確提示"
echo ""
echo "📊 Phase 2 分數更新："
echo "   ✅ Worker Pool: 15分"
echo "   ✅ P2P Messaging: 15分"
echo "   ✅ Message Encryption: 10分"
echo "   ⏳ Group Chat: 0分"
echo "   ⏳ File Transfer: 0分"
echo ""
echo "🎯 當前 Phase 2 進度: 40分/60分"
echo "🎯 總分估計: 80分/100分"
echo ""
echo "💡 Demo 準備建議："
echo "1. 展示加密自我測試通過"
echo "2. 展示 Client-Server 加密通訊"
echo "3. 展示 P2P 加密訊息傳送"
echo "4. 強調使用 AES-256-CBC (industry standard)"
echo "5. 說明 IV 隨機生成確保安全性"
echo ""
echo "🔐 加密實作完成！"