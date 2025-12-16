#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include "Crypto.h"

/**
 * Phase 2: File Transfer Module
 * 
 * 功能：
 * - 分塊檔案傳輸
 * - AES-256-CBC 加密
 * - 進度顯示
 * - 支援任意大小檔案
 */

class FileTransfer {
private:
    // 常數定義為內聯函數以避免 ODR 問題
    static size_t getChunkSize() { return 2 * 1024 * 1024; }  // 2MB
    static size_t getBufferSize() { return 65536; }
    
    Crypto& crypto;
    bool encryptionEnabled;
    
    // 獲取檔案大小
    static size_t getFileSize(const std::string& filename) {
        struct stat st;
        if (stat(filename.c_str(), &st) != 0) {
            return 0;
        }
        return st.st_size;
    }
    
    // 從路徑提取檔名
    static std::string getBasename(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos != std::string::npos) {
            return path.substr(pos + 1);
        }
        return path;
    }
    
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
            std::cerr << "FileTransfer: Data too large: " << len << std::endl;
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
    FileTransfer(Crypto& c) : crypto(c), encryptionEnabled(true) {}
    
    void setEncryption(bool enabled) {
        encryptionEnabled = enabled;
    }
    
    /**
     * 發送檔案
     * 
     * @param targetIP 目標 IP
     * @param targetPort 目標 Port
     * @param filepath 檔案路徑
     * @param senderName 發送者名稱
     * @return 是否成功
     */
    bool sendFile(const std::string& targetIP, int targetPort, 
                  const std::string& filepath, const std::string& senderName) {
        
        // 檢查檔案是否存在
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "❌ Cannot open file: " << filepath << std::endl;
            return false;
        }
        
        size_t fileSize = getFileSize(filepath);
        std::string filename = getBasename(filepath);
        
        std::cout << "📁 Preparing to send file: " << filename << std::endl;
        std::cout << "   Size: " << fileSize << " bytes" << std::endl;
        
        // 建立連接
        int targetSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (targetSocket < 0) {
            std::cerr << "❌ Failed to create socket" << std::endl;
            return false;
        }
        
        struct sockaddr_in targetAddr;
        targetAddr.sin_family = AF_INET;
        targetAddr.sin_port = htons(targetPort);
        
        if (inet_pton(AF_INET, targetIP.c_str(), &targetAddr.sin_addr) <= 0) {
            std::cerr << "❌ Invalid IP address" << std::endl;
            close(targetSocket);
            return false;
        }
        
        if (connect(targetSocket, (struct sockaddr*)&targetAddr, sizeof(targetAddr)) < 0) {
            std::cerr << "❌ Failed to connect to " << targetIP << ":" << targetPort << std::endl;
            close(targetSocket);
            return false;
        }
        
        std::cout << "📤 Connected, starting file transfer..." << std::endl;
        
        try {
            // 發送檔案傳輸請求
            // 格式: FILE_TRANSFER:sender:filename:filesize:encrypted
            std::string header = "FILE_TRANSFER:" + senderName + ":" + filename + ":" + 
                                std::to_string(fileSize) + ":" + 
                                (encryptionEnabled ? "1" : "0");
            
            if (!sendWithLength(targetSocket, header)) {
                std::cerr << "❌ Failed to send header" << std::endl;
                close(targetSocket);
                return false;
            }
            
            // 等待確認
            std::string response;
            if (!recvWithLength(targetSocket, response)) {
                std::cerr << "❌ Failed to receive response" << std::endl;
                close(targetSocket);
                return false;
            }
            
            if (response != "FILE_ACCEPT") {
                std::cerr << "❌ Transfer rejected: " << response << std::endl;
                close(targetSocket);
                return false;
            }
            
            // 分塊發送檔案
            std::vector<char> buffer(getChunkSize());
            size_t totalSent = 0;
            int chunkNum = 0;
            
            while (totalSent < fileSize) {
                // 讀取一個 chunk
                size_t toRead = std::min(getChunkSize(), fileSize - totalSent);
                file.read(buffer.data(), toRead);
                size_t actualRead = file.gcount();
                
                if (actualRead == 0) {
                    std::cerr << "❌ Failed to read file" << std::endl;
                    break;
                }
                
                std::string chunkData(buffer.data(), actualRead);
                
                // 加密 chunk（如果啟用）
                if (encryptionEnabled) {
                    std::string encrypted = crypto.encrypt(chunkData);
                    if (encrypted.empty()) {
                        std::cerr << "❌ Encryption failed" << std::endl;
                        close(targetSocket);
                        return false;
                    }
                    chunkData = encrypted;
                }
                
                // 發送 chunk
                if (!sendWithLength(targetSocket, chunkData)) {
                    std::cerr << "❌ Failed to send chunk " << chunkNum << std::endl;
                    close(targetSocket);
                    return false;
                }
                
                totalSent += actualRead;
                chunkNum++;
                
                // 顯示進度
                int progress = (int)((totalSent * 100) / fileSize);
                std::cout << "\r📤 Progress: " << progress << "% (" 
                          << totalSent << "/" << fileSize << " bytes)" << std::flush;
            }
            
            std::cout << std::endl;
            
            // 等待完成確認
            if (!recvWithLength(targetSocket, response)) {
                std::cerr << "❌ Failed to receive completion" << std::endl;
                close(targetSocket);
                return false;
            }
            
            if (response == "FILE_COMPLETE") {
                std::cout << "✅ File transfer completed successfully!" << std::endl;
                if (encryptionEnabled) {
                    std::cout << "🔒 File was encrypted during transfer" << std::endl;
                }
                close(targetSocket);
                return true;
            } else {
                std::cerr << "❌ Transfer failed: " << response << std::endl;
                close(targetSocket);
                return false;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "❌ Exception: " << e.what() << std::endl;
            close(targetSocket);
            return false;
        }
        
        close(targetSocket);
        return false;
    }
    
    /**
     * 處理檔案接收
     * 
     * @param clientSocket 客戶端 socket
     * @param header 已接收的 header
     * @param savePath 儲存路徑
     * @return 是否成功
     */
    bool handleFileReceive(int clientSocket, const std::string& header, 
                          const std::string& savePath = "./") {
        
        try {
            // 解析 header: FILE_TRANSFER:sender:filename:filesize:encrypted
            size_t pos1 = header.find(':', 14);  // 跳過 "FILE_TRANSFER:"
            size_t pos2 = header.find(':', pos1 + 1);
            size_t pos3 = header.find(':', pos2 + 1);
            
            if (pos1 == std::string::npos || pos2 == std::string::npos || 
                pos3 == std::string::npos) {
                std::cerr << "❌ Invalid file transfer header" << std::endl;
                sendWithLength(clientSocket, "FILE_REJECT:Invalid header");
                return false;
            }
            
            std::string sender = header.substr(14, pos1 - 14);
            std::string filename = header.substr(pos1 + 1, pos2 - pos1 - 1);
            size_t fileSize = std::stoull(header.substr(pos2 + 1, pos3 - pos2 - 1));
            bool isEncrypted = (header.substr(pos3 + 1) == "1");
            
            std::cout << std::endl;
            std::cout << "📥 Incoming file transfer from " << sender << std::endl;
            std::cout << "   Filename: " << filename << std::endl;
            std::cout << "   Size: " << fileSize << " bytes" << std::endl;
            std::cout << "   Encrypted: " << (isEncrypted ? "Yes" : "No") << std::endl;
            
            // 發送接受確認
            if (!sendWithLength(clientSocket, "FILE_ACCEPT")) {
                std::cerr << "❌ Failed to send accept" << std::endl;
                return false;
            }
            
            // 準備儲存檔案
            std::string fullPath = savePath + "/" + filename;
            std::ofstream outFile(fullPath, std::ios::binary);
            if (!outFile.is_open()) {
                std::cerr << "❌ Cannot create file: " << fullPath << std::endl;
                return false;
            }
            
            // 接收檔案內容
            size_t totalReceived = 0;
            
            while (totalReceived < fileSize) {
                std::string chunkData;
                if (!recvWithLength(clientSocket, chunkData)) {
                    std::cerr << "❌ Failed to receive chunk" << std::endl;
                    outFile.close();
                    return false;
                }
                
                // 解密（如果需要）
                std::string decryptedData;
                if (isEncrypted) {
                    decryptedData = crypto.decrypt(chunkData);
                    if (decryptedData.empty()) {
                        std::cerr << "❌ Decryption failed" << std::endl;
                        outFile.close();
                        return false;
                    }
                } else {
                    decryptedData = chunkData;
                }
                
                // 寫入檔案
                outFile.write(decryptedData.data(), decryptedData.size());
                totalReceived += decryptedData.size();
                
                // 顯示進度
                int progress = (int)((totalReceived * 100) / fileSize);
                std::cout << "\r📥 Progress: " << progress << "% (" 
                          << totalReceived << "/" << fileSize << " bytes)" << std::flush;
            }
            
            std::cout << std::endl;
            outFile.close();
            
            // 發送完成確認
            if (!sendWithLength(clientSocket, "FILE_COMPLETE")) {
                std::cerr << "❌ Failed to send completion" << std::endl;
                return false;
            }
            
            std::cout << "✅ File saved to: " << fullPath << std::endl;
            if (isEncrypted) {
                std::cout << "🔓 File was decrypted successfully" << std::endl;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "❌ Exception: " << e.what() << std::endl;
            return false;
        }
    }
    
    /**
     * 檢查是否為檔案傳輸請求
     */
    static bool isFileTransferRequest(const std::string& message) {
        return message.find("FILE_TRANSFER:") == 0;
    }
};

#endif // FILE_TRANSFER_H
