#!/bin/bash

# Phase 2 第一步測試腳本
echo "=== Phase 2 第一步測試：Worker Pool + P2P Discovery ==="
echo ""
echo "🎯 本次實作內容："
echo "✅ Professional ThreadPool (10 worker threads)"
echo "✅ Enhanced concurrency handling"
echo "✅ P2P discovery support (GET_USER_INFO command)"
echo "✅ Improved logging and monitoring"
echo "✅ Full backward compatibility with Phase 1"
echo ""

# 檢查檔案
echo "📁 檢查必要檔案..."
required_files=("ThreadPool.h" "Server_Phase2.cpp" "Client_Phase2.cpp" "Makefile_Phase2")
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

echo "✅ 所有必要檔案都存在"
echo ""

# 備份Phase 1（如果存在）
echo "💾 備份Phase 1版本..."
if [ -f "server" ]; then
    cp server server_phase1_backup 2>/dev/null
    echo "✅ 備份server為server_phase1_backup"
fi
if [ -f "client" ]; then
    cp client client_phase1_backup 2>/dev/null
    echo "✅ 備份client為client_phase1_backup"
fi
echo ""

# 編譯Phase 2
echo "🔨 編譯Phase 2..."
cp Makefile_Phase2 Makefile
make clean
make phase2

if [ $? -ne 0 ]; then
    echo "❌ 編譯失敗"
    echo "請檢查程式碼是否有錯誤"
    exit 1
fi

echo "✅ Phase 2編譯成功"
echo ""

# 顯示編譯結果
echo "📋 編譯結果："
ls -la server_phase2 client_phase2 2>/dev/null || echo "❌ 執行檔未找到"
echo ""

echo "🧪 開始測試Phase 2功能..."
echo ""

echo "=== 測試步驟 ==="
echo ""

echo "步驟1: 啟動Phase 2 Server"
echo "-------------------------------"
echo "在新Terminal中執行:"
echo "  ./server_phase2 8080"
echo ""
echo "你應該看到:"
echo "  === Phase 2 Socket Programming Server ==="
echo "  === Phase 2 ChatServer ==="
echo "  Creating ThreadPool with 10 workers"
echo "  Worker 0 started (thread ID: ...)"
echo "  Worker 1 started (thread ID: ...)"
echo "  ..."
echo "  Phase 2 Server started on port 8080"
echo "  Worker Pool Status: 10 workers ready"
echo ""
echo "按Enter繼續..."
read

echo "步驟2: 測試基本功能（確保向下相容）"
echo "--------------------------------------"
echo "在新Terminal中執行:"
echo "  ./client_phase2 127.0.0.1 8080"
echo ""
echo "測試以下操作:"
echo "  1. 註冊用戶: 1 → test1 → 1234"
echo "  2. 登入用戶: 2 → test1 → 1234 → 9001"
echo "  3. 查看用戶: 1"
echo ""
echo "Server端應該顯示詳細的Worker Thread資訊:"
echo "  [Client 1] Started handling 127.0.0.1 (Worker Thread: ...)"
echo "  [Client 1] Processing: REGISTER for user: []"
echo ""
echo "完成基本測試後按Enter繼續..."
read

echo "步驟3: 測試新功能 - P2P User Discovery"
echo "-----------------------------------"
echo "在已登入的client中測試新功能:"
echo "  3 → test1"
echo ""
echo "你應該看到:"
echo "  ✅ User test1 P2P Info:"
echo "     IP: 127.0.0.1"
echo "     Port: 9001"
echo "     (Ready for future P2P messaging)"
echo ""
echo "這個功能為後續的P2P直接通訊做準備！"
echo ""
echo "完成P2P discovery測試後按Enter繼續..."
read

echo "步驟4: 測試多用戶併發（Worker Pool效果）"
echo "----------------------------------------"
echo "同時開啟3-4個新Terminal，每個執行:"
echo "  ./client_phase2 127.0.0.1 8080"
echo ""
echo "分別註冊登入:"
echo "  Terminal A: test2/1234 port:9002"
echo "  Terminal B: test3/1234 port:9003"
echo "  Terminal C: test4/1234 port:9004"
echo ""
echo "Server端應該顯示:"
echo "  [Client 2] New connection - assigning to ThreadPool (Queue: 0)"
echo "  [Client 3] New connection - assigning to ThreadPool (Queue: 0)"
echo "  每個client由不同的Worker Thread處理"
echo ""
echo "這證明ThreadPool正在有效分配工作！"
echo ""
echo "完成併發測試後按Enter繼續..."
read

echo "步驟5: 測試P2P Discovery功能"
echo "----------------------------"
echo "在任一已登入的client中:"
echo "  3 → test2  (查詢其他用戶的P2P資訊)"
echo "  3 → test3"
echo "  3 → test4"
echo ""
echo "每次查詢都應該返回正確的IP:Port資訊"
echo "Server端會記錄詳細的查詢日誌"
echo ""
echo "完成P2P查詢測試後按Enter繼續..."
read

echo "步驟6: 效能和穩定性觀察"
echo "-----------------------"
echo "觀察Server端日誌，注意:"
echo "  ✅ ThreadPool Queue size保持在低位"
echo "  ✅ Worker threads均勻分配任務"
echo "  ✅ 沒有競爭條件或死鎖"
echo "  ✅ 用戶登出時正確清理資源"
echo ""
echo "完成觀察後按Enter繼續..."
read

echo "=== Phase 2 第一步測試完成 ==="
echo ""
echo "🎉 恭喜！如果所有測試都通過，你已成功實作："
echo ""
echo "✅ 已完成功能："
echo "   📊 Professional ThreadPool (15分)"
echo "   🔍 P2P User Discovery (為15分P2P Messages準備)"
echo "   📈 Enhanced concurrency performance"
echo "   🛡️ Improved error handling and logging"
echo ""
echo "🚀 準備實作的下一步功能："
echo "   💬 P2P Direct Messaging (15分)"
echo "   🔒 Basic Message Encryption (10分)"
echo "   👥 Group Chat (10分)"
echo ""
echo "📊 Phase 2 進度："
echo "   ✅ Worker Pool: 100% 完成"
echo "   🔄 P2P Messages: 30% 完成 (discovery ready)"
echo "   ⏳ Encryption: 0% 完成"
echo "   ⏳ Group Chat: 0% 完成"
echo "   ⏳ File Transfer: 0% 完成"
echo ""
echo "💡 下次開發建議："
echo "1. 實作P2P直接訊息傳送"
echo "2. 客戶端監聽功能"
echo "3. 基礎OpenSSL加密"
echo ""
echo "🎯 當前版本已足夠穩定，可以繼續開發下一個功能！"
echo ""

# 清理選項
echo "🧹 清理選項："
echo "1. 保持Phase 2版本 (建議)"
echo "2. 恢復Phase 1版本"
echo "3. 同時保留兩個版本"
echo ""
echo -n "請選擇 (1-3): "
read cleanup_choice

case $cleanup_choice in
    1)
        echo "✅ 保持Phase 2版本，準備繼續開發"
        ;;
    2)
        echo "🔄 恢復Phase 1版本..."
        if [ -f "server_phase1_backup" ]; then
            mv server_phase1_backup server
            echo "✅ 恢復server"
        fi
        if [ -f "client_phase1_backup" ]; then
            mv client_phase1_backup client
            echo "✅ 恢復client"
        fi
        ;;
    3)
        echo "📁 保留兩個版本："
        echo "   Phase 1: server, client"
        echo "   Phase 2: server_phase2, client_phase2"
        ;;
esac

echo ""
echo "🎬 Phase 2 第一步測試完成！"
echo "準備好繼續開發下一個功能了嗎？ 🚀"