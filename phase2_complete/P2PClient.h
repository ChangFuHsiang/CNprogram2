#ifndef P2P_CLIENT_H
#define P2P_CLIENT_H

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "Crypto.h"
#include "FileTransfer.h"

/**
 * Phase 2: P2P Client with Encryption and File Transfer Support
 * 
 * 功能：
 * - P2P 直接訊息傳送
 * - P2P 監聽接收
 * - AES-256-CBC 加密/解密
 * - P2P 檔案傳輸
 */

class P2PClient {
private:
    int listenSocket;
    int listenPort;
    std::thread listenThread;
    std::atomic<bool> isListening{false};
    std::string myUsername;
    mutable std::mutex p2p_mutex;
    
    // Phase 2: 加密模組
    Crypto crypto;
    bool encryptionEnabled;
    
    // Phase 2: 檔案傳輸模組
    FileTransfer fileTransfer;
    std::string downloadPath;
    
    // 發送帶長度前綴的數據
    bool sendWithLength(int socket, const std::string& data) {
        uint32_t len = htonl(data.length());
        if (send(socket, &len, sizeof(len), 0) != sizeof(len)) {
            return false;
        }
        
        size_t totalSent = 0;
        while (totalSent < data.length()) {
            ssize_t sent = send(socket, data.c_str() + totalSent, 
                               data.length() - totalSent, 0);
            if (sent <= 0) return false;
            totalSent += sent;
        }
        return true;
    }
    
    // 接收帶長度前綴的數據
    bool recvWithLength(int socket, std::string& data) {
        uint32_t len;
        if (recv(socket, &len, sizeof(len), MSG_WAITALL) != sizeof(len)) {
            return false;
        }
        len = ntohl(len);
        
        if (len > 100 * 1024 * 1024) { // 限制 100MB
            return false;
        }
        
        data.resize(len);
        size_t totalRecv = 0;
        while (totalRecv < len) {
            ssize_t received = recv(socket, &data[totalRecv], len - totalRecv, 0);
            if (received <= 0) return false;
            totalRecv += received;
        }
        return true;
    }
    
public:
    P2PClient(int port, const std::string& username) 
        : listenPort(port), myUsername(username), encryptionEnabled(true),
          fileTransfer(crypto), downloadPath(".") {
        
        // 執行加密自我測試
        if (crypto.selfTest()) {
            std::cout << "🔐 P2P Encryption enabled (AES-256-CBC)" << std::endl;
        } else {
            std::cerr << "⚠️ Encryption self-test failed, disabling encryption" << std::endl;
            encryptionEnabled = false;
        }
        
        fileTransfer.setEncryption(encryptionEnabled);
    }
    
    // 設定下載路徑
    void setDownloadPath(const std::string& path) {
        downloadPath = path;
        std::cout << "📁 Download path set to: " << downloadPath << std::endl;
    }
    
    // 啟用/停用加密
    void setEncryption(bool enabled) {
        encryptionEnabled = enabled;
        fileTransfer.setEncryption(enabled);
        std::cout << "🔐 P2P Encryption " << (enabled ? "enabled" : "disabled") << std::endl;
    }
    
    bool isEncryptionEnabled() const {
        return encryptionEnabled;
    }
    
    // 啟動P2P監聽
    bool startP2PListener() {
        try {
            // 建立監聽socket
            listenSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (listenSocket < 0) {
                std::cerr << "P2P: Failed to create listen socket" << std::endl;
                return false;
            }
            
            // 設定socket選項
            int opt = 1;
            if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                std::cerr << "P2P: setsockopt failed" << std::endl;
                return false;
            }
            
            // 綁定到指定port
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(listenPort);
            
            if (::bind(listenSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                std::cerr << "P2P: Failed to bind to port " << listenPort << std::endl;
                return false;
            }
            
            // 開始監聽
            if (listen(listenSocket, 5) < 0) {
                std::cerr << "P2P: Failed to listen" << std::endl;
                return false;
            }
            
            isListening = true;
            
            // 啟動監聽thread
            listenThread = std::thread([this]() {
                this->listenForP2PConnections();
            });
            
            std::cout << "✅ P2P Listener started on port " << listenPort << std::endl;
            if (encryptionEnabled) {
                std::cout << "🔒 All P2P messages and files will be encrypted" << std::endl;
            }
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "P2P: Exception in startP2PListener: " << e.what() << std::endl;
            return false;
        }
    }
    
    // 監聽P2P連接
    void listenForP2PConnections() {
        std::cout << "P2P: Listening thread started (ID: " << std::this_thread::get_id() << ")" << std::endl;
        
        while (isListening) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            
            int clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientSocket < 0) {
                if (isListening) {
                    std::cerr << "P2P: Accept failed" << std::endl;
                }
                continue;
            }
            
            std::string clientIP = inet_ntoa(clientAddr.sin_addr);
            
            // 處理P2P連接（在新thread中）
            std::thread([this, clientSocket, clientIP]() {
                this->handleP2PConnection(clientSocket, clientIP);
            }).detach();
        }
        
        std::cout << "P2P: Listening thread finished" << std::endl;
    }
    
    // 處理incoming P2P連接
    void handleP2PConnection(int clientSocket, const std::string& clientIP) {
        try {
            // 使用長度前綴協議接收數據
            std::string message;
            if (!recvWithLength(clientSocket, message)) {
                // 嘗試舊協議（向後兼容）
                char buffer[4096];
                memset(buffer, 0, sizeof(buffer));
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesReceived <= 0) {
                    close(clientSocket);
                    return;
                }
                buffer[bytesReceived] = '\0';
                message = std::string(buffer);
            }
            
            // 檢查是否為檔案傳輸請求
            if (FileTransfer::isFileTransferRequest(message)) {
                std::cout << "📨 File transfer request from: " << clientIP << std::endl;
                fileTransfer.handleFileReceive(clientSocket, message, downloadPath);
                close(clientSocket);
                return;
            }
            
            // 處理 P2P 訊息
            if (message.find("P2P_MSG:") == 0) {
                size_t firstColon = message.find(':', 8);
                if (firstColon != std::string::npos) {
                    std::string sender = message.substr(8, firstColon - 8);
                    std::string content = message.substr(firstColon + 1);
                    
                    // 檢查是否為加密訊息
                    std::string displayContent;
                    bool wasEncrypted = false;
                    
                    if (Crypto::isEncryptedMessage(content)) {
                        // 解密訊息
                        displayContent = crypto.decryptMessage(content);
                        wasEncrypted = true;
                        if (displayContent.empty()) {
                            displayContent = "[Decryption failed]";
                        }
                    } else {
                        // 未加密的訊息
                        displayContent = content;
                    }
                    
                    std::lock_guard<std::mutex> lock(p2p_mutex);
                    std::cout << std::endl;
                    if (wasEncrypted) {
                        std::cout << "🔓💬 [P2P-Encrypted] " << sender << ": " << displayContent << std::endl;
                    } else {
                        std::cout << "💬 [P2P] " << sender << ": " << displayContent << std::endl;
                    }
                    std::cout << "Press Enter to continue...";
                    std::cout.flush();
                    
                    // 發送確認
                    std::string ack = "P2P_ACK:" + myUsername;
                    sendWithLength(clientSocket, ack);
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "P2P: Exception handling connection: " << e.what() << std::endl;
        }
        
        close(clientSocket);
    }
    
    // 發送P2P訊息 (支援加密)
    bool sendP2PMessage(const std::string& targetIP, int targetPort, const std::string& message) {
        try {
            std::cout << "📤 Sending P2P message to " << targetIP << ":" << targetPort;
            if (encryptionEnabled) {
                std::cout << " (encrypted)";
            }
            std::cout << std::endl;
            
            // 建立到目標的socket連接
            int targetSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (targetSocket < 0) {
                std::cerr << "P2P: Failed to create socket for sending" << std::endl;
                return false;
            }
            
            // 連接到目標
            struct sockaddr_in targetAddr;
            targetAddr.sin_family = AF_INET;
            targetAddr.sin_port = htons(targetPort);
            
            if (inet_pton(AF_INET, targetIP.c_str(), &targetAddr.sin_addr) <= 0) {
                std::cerr << "P2P: Invalid target IP address" << std::endl;
                close(targetSocket);
                return false;
            }
            
            if (connect(targetSocket, (struct sockaddr*)&targetAddr, sizeof(targetAddr)) < 0) {
                std::cerr << "P2P: Failed to connect to target" << std::endl;
                close(targetSocket);
                return false;
            }
            
            // 構造P2P訊息
            std::string p2pMessage;
            if (encryptionEnabled) {
                // 加密訊息內容
                std::string encryptedContent = crypto.encryptMessage(message);
                if (encryptedContent.empty()) {
                    std::cerr << "P2P: Encryption failed, sending unencrypted" << std::endl;
                    p2pMessage = "P2P_MSG:" + myUsername + ":" + message;
                } else {
                    p2pMessage = "P2P_MSG:" + myUsername + ":" + encryptedContent;
                    std::cout << "🔒 Message encrypted successfully" << std::endl;
                }
            } else {
                // 未加密訊息
                p2pMessage = "P2P_MSG:" + myUsername + ":" + message;
            }
            
            // 發送訊息（使用長度前綴）
            if (!sendWithLength(targetSocket, p2pMessage)) {
                std::cerr << "P2P: Failed to send message" << std::endl;
                close(targetSocket);
                return false;
            }
            
            // 等待確認
            std::string ack;
            if (recvWithLength(targetSocket, ack)) {
                if (ack.find("P2P_ACK:") == 0) {
                    std::cout << "✅ P2P message delivered successfully";
                    if (encryptionEnabled) {
                        std::cout << " (encrypted)";
                    }
                    std::cout << std::endl;
                }
            }
            
            close(targetSocket);
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "P2P: Exception in sendP2PMessage: " << e.what() << std::endl;
            return false;
        }
    }
    
    // 發送檔案
    bool sendFile(const std::string& targetIP, int targetPort, const std::string& filepath) {
        return fileTransfer.sendFile(targetIP, targetPort, filepath, myUsername);
    }
    
    // 停止P2P監聽
    void stopP2PListener() {
        if (isListening) {
            std::cout << "🛑 Stopping P2P listener..." << std::endl;
            isListening = false;
            
            if (listenSocket >= 0) {
                close(listenSocket);
            }
            
            if (listenThread.joinable()) {
                listenThread.join();
            }
            
            std::cout << "✅ P2P listener stopped" << std::endl;
        }
    }
    
    ~P2PClient() {
        stopP2PListener();
    }
};

#endif // P2P_CLIENT_H
