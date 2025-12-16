# Phase 2 Socket Programming - Complete Implementation

## 📊 功能總覽與分數

| 功能 | 分數 | 狀態 |
|------|------|------|
| **Phase 1** | | |
| Basic Server-Client Communication | 20 | ✅ |
| Authentication Features | 20 | ✅ |
| **Phase 2** | | |
| Multithread Server (ThreadPool) | 15 | ✅ |
| P2P Direct Messaging | 15 | ✅ |
| Message Encryption (OpenSSL) | 10 | ✅ |
| Group Chatting (Relay Mode) | 10 | ✅ |
| File Transfer (Encrypted) | 10 | ✅ |
| **總計** | **100** | ✅ |

---

## 🚀 快速開始

### 安裝 OpenSSL

```bash
# macOS
brew install openssl

# Ubuntu/Debian
sudo apt-get install libssl-dev

# CentOS/RHEL
sudo yum install openssl-devel
```

### 編譯

```bash
make
```

### 執行

```bash
# Terminal 1: 啟動 Server
./server_phase2 8080

# Terminal 2: 啟動 Client 1
./client_phase2 127.0.0.1 8080

# Terminal 3: 啟動 Client 2
./client_phase2 127.0.0.1 8080
```

---

## 📁 檔案說明

| 檔案 | 說明 |
|------|------|
| `Server_Phase2.cpp` | 完整版 Server（含群組聊天） |
| `Client_Phase2.cpp` | 完整版 Client（含所有功能） |
| `ThreadPool.h` | 專業執行緒池模組 |
| `Crypto.h` | AES-256-CBC 加密模組 |
| `P2PClient.h` | P2P 通訊模組（含檔案傳輸） |
| `FileTransfer.h` | 加密檔案傳輸模組 |
| `Makefile` | 編譯設定 |

---

## 🔧 功能詳細說明

### 1️⃣ ThreadPool (15分)

- 10 個 Worker Threads
- 任務佇列管理
- 支援 10 個同時連線

### 2️⃣ P2P Direct Messaging (15分)

```
Client 操作：
3. Get user info      - 獲取目標用戶的 IP 和 Port
4. Send P2P message   - 直接發送加密訊息（不經過 Server）
```

**特點：**
- 訊息直接在 Client 之間傳輸
- Server 僅用於用戶發現
- 支援 AES-256-CBC 加密

### 3️⃣ Message Encryption (10分)

**加密規格：**
- 演算法：AES-256-CBC
- 金鑰長度：256 bits
- IV：每次加密隨機生成
- 編碼：Base64

**加密範圍：**
- ✅ Client-Server 通訊
- ✅ P2P 訊息
- ✅ 群組訊息
- ✅ 檔案傳輸

### 4️⃣ Group Chatting (10分)

```
Client 操作：
5.  List rooms         - 列出所有聊天室
6.  Create room        - 建立新聊天室
7.  Join room          - 加入聊天室
8.  Leave room         - 離開聊天室
9.  Send room message  - 發送群組訊息
10. View room history  - 查看訊息歷史
11. View room members  - 查看成員列表
```

**架構：**
- Relay Mode（訊息經過 Server 轉發）
- 訊息按順序顯示
- 支援訊息歷史
- 加入/離開通知

### 5️⃣ File Transfer (10分)

```
Client 操作：
12. Send file          - 發送加密檔案
13. Set download path  - 設定下載路徑
```

**特點：**
- 分塊傳輸（2MB/chunk）
- AES-256-CBC 加密
- 進度顯示
- 基於 P2P 架構

---

## 📖 使用範例

### 範例 1：P2P 加密訊息

```
[Client A]
2 → alice / password123 / 9001    # 登入

[Client B]
2 → bob / password456 / 9002      # 登入
4 → alice → Hello!                # 發送 P2P 訊息

[Client A]
🔓💬 [P2P-Encrypted] bob: Hello!  # 收到加密訊息
```

### 範例 2：群組聊天

```
[Client A]
6 → general                       # 建立聊天室

[Client B]
7 → general                       # 加入聊天室
9 → general → Hi everyone!        # 發送群組訊息

[Client A]
📢 ROOM_MSG:general:bob:Hi everyone!  # 收到群組訊息
```

### 範例 3：檔案傳輸

```
[Client A]
12 → bob → /path/to/file.pdf      # 發送檔案
📁 Preparing to send file: file.pdf
   Size: 1048576 bytes
📤 Progress: 100% (1048576/1048576 bytes)
✅ File transfer completed successfully!
🔒 File was encrypted during transfer

[Client B]
📥 Incoming file transfer from alice
   Filename: file.pdf
   Size: 1048576 bytes
   Encrypted: Yes
📥 Progress: 100%
✅ File saved to: ./file.pdf
🔓 File was decrypted successfully
```

---

## 🔐 安全性

### 加密實作

1. **金鑰管理**
   - 使用預設對稱金鑰
   - 所有通訊使用相同金鑰

2. **IV 處理**
   - 每次加密生成隨機 IV
   - IV 與密文一起傳輸

3. **訊息格式**
   ```
   ENC:BASE64(IV):BASE64(CIPHERTEXT)
   ```

---

## 🧪 測試指南

### 測試 1：基本功能

1. 啟動 Server 和 2 個 Client
2. 註冊並登入兩個用戶
3. 測試 LIST 功能

### 測試 2：P2P 訊息

1. Alice 獲取 Bob 的資訊
2. Alice 發送 P2P 訊息給 Bob
3. 確認 Bob 收到加密訊息

### 測試 3：群組聊天

1. Alice 建立聊天室 "test"
2. Bob 加入聊天室
3. 雙方互發訊息
4. 查看訊息歷史

### 測試 4：檔案傳輸

1. 準備測試檔案
2. Alice 發送檔案給 Bob
3. 確認 Bob 正確接收並解密

---

## ⚠️ 注意事項

1. **Port 範圍**：1025-65535
2. **檔案大小**：理論上無限制，但建議 < 100MB
3. **同時連線**：最多 10 個
4. **加密開銷**：大檔案傳輸會有些許效能影響

---

## 📝 Demo 影片建議

1. **開場**（30秒）
   - 說明實作的功能
   - 展示編譯過程

2. **基本功能**（1分鐘）
   - 註冊、登入、登出
   - 列出線上用戶

3. **P2P 訊息**（1.5分鐘）
   - 展示加密傳輸
   - 說明 P2P 架構

4. **群組聊天**（2分鐘）
   - 建立/加入聊天室
   - 多人訊息交流
   - 訊息歷史

5. **檔案傳輸**（2分鐘）
   - 發送檔案
   - 展示進度條
   - 說明加密傳輸

6. **程式碼說明**（3分鐘）
   - ThreadPool 架構
   - 加密實作
   - 檔案分塊傳輸

---

## 👨‍💻 作者

Computer Network 2025 Socket Programming Project
