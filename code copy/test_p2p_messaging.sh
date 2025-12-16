#!/bin/bash

# P2P Direct Messaging 測試腳本
echo "=== Phase 2 P2P Direct Messaging 測試 ==="
echo ""
echo "🎯 本次新增功能："
echo "✅ 客戶端P2P監聽 (每個用戶都能接收直接連接)"
echo "✅ P2P直接訊息傳送 (用戶間直接通訊，繞過伺服器)"
echo "✅ P2P通訊協定 (P2P_MSG:sender:content 格式)"
echo "✅ 實時雙向通訊 (可同時發送和接收訊息)"
echo "✅ 自動用戶發現 (通過伺服器獲取P2P連接資訊)"
echo ""

# 檢查檔案
echo "📁 檢查P2P相關檔案..."
required_files=("P2PClient.h" "ThreadPool.h" "Server_Phase2.cpp" "Client_Phase2.cpp" "Makefile_Phase2")
missing_files=()

for file in "${required_files[@]}"; do
    if [ ! -f "$file" ]; then
        missing_files+=("$file")
    fi
done

if [ ${#missing_files[@]} -ne 0 ]; then
    echo "❌ 缺少以下檔案："
    for file in "${missing_files[@]}"; do
        echo "   - $file"
    done
    echo ""
    echo "請確保所有檔案都已創建並放在正確位置。"
    exit 1
fi

echo "✅ 所有P2P相關檔案都存在"
echo ""

# 編譯檢查
echo "🔨 編譯Phase 2 with P2P..."
cp Makefile_Phase2 Makefile
make clean
make phase2

if [ $? -ne 0 ]; then
    echo "❌ 編譯失敗"
    echo "請檢查P2P相關程式碼是否有錯誤"
    exit 1
fi

echo "✅ Phase 2 P2P版本編譯成功"
echo ""

# 檢查執行檔
if [ ! -f "server_phase2" ] || [ ! -f "client_phase2" ]; then
    echo "❌ 執行檔未找到"
    exit 1
fi

echo "✅ 執行檔準備就緒"
echo ""

echo "🧪 開始P2P直接訊息傳送測試..."
echo ""

echo "=== P2P測試步驟 ==="
echo ""

echo "步驟1: 啟動Phase 2 Server"
echo "-------------------------------"
echo "在新Terminal中執行:"
echo "  ./server_phase2 8080"
echo ""
echo "你應該看到Worker Pool啟動訊息:"
echo "  === Phase 2 Socket Programming Server ==="
echo "  Creating ThreadPool with 10 workers"
echo "  Phase 2 Server started on port 8080"
echo ""
echo "Server已準備好處理P2P用戶發現請求"
echo "按Enter繼續..."
read

echo "步驟2: 啟動第一個P2P客戶端 (Alice)"
echo "-----------------------------------"
echo "在新Terminal中執行:"
echo "  ./client_phase2 127.0.0.1 8080"
echo ""
echo "進行以下操作:"
echo "  1 → alice → pass123    (註冊Alice)"
echo "  2 → alice → pass123 → 9001    (登入Alice，P2P端口9001)"
echo ""
echo "你應該看到P2P啟動訊息:"
echo "  🚀 Starting P2P listener..."
echo "  ✅ P2P Listener started on port 9001"
echo "  ✅ P2P system ready! You can now send/receive direct messages"
echo ""
echo "Alice已準備好接收P2P訊息!"
echo "按Enter繼續..."
read

echo "步驟3: 啟動第二個P2P客戶端 (Bob)"
echo "----------------------------------"
echo "在另一個新Terminal中執行:"
echo "  ./client_phase2 127.0.0.1 8080"
echo ""
echo "進行以下操作:"
echo "  1 → bob → pass456    (註冊Bob)"
echo "  2 → bob → pass456 → 9002    (登入Bob，P2P端口9002)"
echo ""
echo "Bob也應該看到P2P系統啟動!"
echo "按Enter繼續..."
read

echo "步驟4: 測試P2P用戶發現"
echo "----------------------"
echo "在Bob的Terminal中:"
echo "  3 → alice    (獲取Alice的P2P資訊)"
echo ""
echo "你應該看到:"
echo "  ✅ User alice P2P Info:"
echo "     IP: 127.0.0.1"
echo "     Port: 9001"
echo "     Status: Ready for P2P messaging"
echo ""
echo "這證明伺服器正確提供了P2P連接資訊!"
echo "按Enter繼續..."
read

echo "步驟5: 🎉 第一次P2P直接訊息！"
echo "----------------------------"
echo "在Bob的Terminal中:"
echo "  4 → alice → Hello Alice, this is Bob!    (P2P訊息)"
echo ""
echo "Bob端應該顯示:"
echo "  📤 Sending P2P message to 127.0.0.1:9001"
echo "  ✅ P2P message delivered successfully"
echo ""
echo "Alice端應該立即顯示:"
echo "  💬 [P2P] bob: Hello Alice, this is Bob!"
echo "  Press Enter to continue..."
echo ""
echo "🎉 這是真正的P2P直接通訊！訊息沒有經過伺服器！"
echo "按Enter繼續..."
read

echo "步驟6: 測試雙向P2P通訊"
echo "----------------------"
echo "在Alice的Terminal中回覆:"
echo "  4 → bob → Hi Bob! P2P works great!    (回覆P2P訊息)"
echo ""
echo "現在Bob應該收到來自Alice的直接訊息!"
echo ""
echo "🔄 試試多輪對話:"
echo "  Bob → Alice: How are you?"
echo "  Alice → Bob: I'm fine, thanks!"
echo "  Bob → Alice: P2P is awesome!"
echo ""
echo "完成雙向測試後按Enter繼續..."
read

echo "步驟7: 測試多用戶P2P網絡"
echo "------------------------"
echo "在第三個Terminal啟動Charlie:"
echo "  ./client_phase2 127.0.0.1 8080"
echo "  1 → charlie → pass789"
echo "  2 → charlie → pass789 → 9003"
echo ""
echo "現在測試三方P2P通訊:"
echo "  Charlie → Alice: Hello everyone!"
echo "  Alice → Charlie: Welcome Charlie!"
echo "  Bob → Charlie: Nice to meet you!"
echo ""
echo "每個訊息都是直接P2P連接，不經過伺服器!"
echo "完成多用戶測試後按Enter繼續..."
read

echo "步驟8: 測試P2P系統穩定性"
echo "-------------------------"
echo "測試以下場景:"
echo ""
echo "🔌 離線測試:"
echo "  1. 讓Alice登出 (5 → LOGOUT)"
echo "  2. Bob嘗試發送訊息給Alice"
echo "  3. 應該看到連接失敗訊息"
echo ""
echo "🔄 重連測試:"
echo "  1. Alice重新登入 (不同port: 9004)"
echo "  2. Bob再次嘗試發送訊息 (應該自動獲取新的連接資訊)"
echo ""
echo "🚫 錯誤處理:"
echo "  1. 嘗試發送給不存在的用戶"
echo "  2. 嘗試給自己發送訊息"
echo "  3. 測試空訊息處理"
echo ""
echo "完成穩定性測試後按Enter繼續..."
read

echo "步驟9: 檢查伺服器日誌"
echo "--------------------"
echo "觀察Server Terminal，你應該看到:"
echo ""
echo "✅ 正確的日誌格式:"
echo "  [Client X] GetUserInfo request: bob asking for alice"
echo "  [Client X] Provided user info for P2P: USER_INFO:127.0.0.1:9001"
echo ""
echo "✅ 重要觀察:"
echo "  - 伺服器只處理用戶發現請求"
echo "  - P2P訊息內容不經過伺服器"
echo "  - Worker Pool有效分配P2P發現請求"
echo "  - 無訊息內容洩露到伺服器日誌"
echo ""
echo "這證明了真正的P2P架構設計!"
echo "按Enter繼續..."
read

echo "=== P2P Direct Messaging 測試完成 ==="
echo ""
echo "🎉 恭喜！如果所有測試都通過，你已成功實作："
echo ""
echo "✅ 核心P2P功能："
echo "   💬 P2P Direct Messaging (15分) - 完成!"
echo "   📡 P2P Listener System (客戶端監聽)"
echo "   🔍 P2P User Discovery (伺服器發現服務)"
echo "   📨 P2P Message Protocol (標準化通訊協定)"
echo "   ⚡ Real-time Communication (即時雙向通訊)"
echo ""
echo "🏗️ 技術架構亮點："
echo "   🎯 真正的P2P: 訊息不經過伺服器"
echo "   🔗 Hybrid架構: 伺服器做用戶發現，P2P做訊息傳送"
echo "   🧵 Multi-threading: 同時處理發送和接收"
echo "   🛡️ 錯誤處理: 優雅處理離線和網路錯誤"
echo ""
echo "📊 Phase 2 進度更新："
echo "   ✅ Worker Pool: 100% 完成 (15分)"
echo "   ✅ P2P Messaging: 100% 完成 (15分)"
echo "   ⏳ Encryption: 0% 完成 (10分)"
echo "   ⏳ Group Chat: 0% 完成 (10分)"
echo "   ⏳ File Transfer: 0% 完成 (10分)"
echo ""
echo "🎯 Phase 2 總進度: 50% (30分/60分)"
echo ""
echo "🚀 準備實作的下一個功能："
echo "   🔒 Message Encryption with OpenSSL (10分+5分bonus)"
echo "     - AES對稱加密"
echo "     - 金鑰交換機制"
echo "     - 加密P2P通訊"
echo ""
echo "💡 Demo準備建議："
echo "1. P2P功能是Phase 2的最大亮點!"
echo "2. 強調真正的去中心化訊息傳送"
echo "3. 展示同時多用戶P2P網絡"
echo "4. 說明Hybrid架構的優勢"
echo ""
echo "🎬 當前版本已具備完整的P2P通訊能力！"
echo "準備好繼續實作加密功能了嗎？ 🔐"