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

class P2PClient {
private:
    int listenSocket;
    int listenPort;
    std::thread listenThread;
    std::atomic<bool> isListening{false};
    std::string myUsername;
    mutable std::mutex p2p_mutex;
    
public:
    P2PClient(int port, const std::string& username) : listenPort(port), myUsername(username) {}
    
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
            std::cout << "📨 P2P connection from: " << clientIP << std::endl;
            
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
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                std::cout << "P2P: Failed to receive data from " << clientIP << std::endl;
                close(clientSocket);
                return;
            }
            
            buffer[bytesReceived] = '\0';
            std::string message(buffer);
            
            // 解析P2P訊息格式: P2P_MSG:sender:content
            if (message.find("P2P_MSG:") == 0) {
                size_t firstColon = message.find(':', 8);
                if (firstColon != std::string::npos) {
                    std::string sender = message.substr(8, firstColon - 8);
                    std::string content = message.substr(firstColon + 1);
                    
                    std::lock_guard<std::mutex> lock(p2p_mutex);
                    std::cout << "\n💬 [P2P] " << sender << ": " << content << std::endl;
                    std::cout << "Press Enter to continue...";
                    std::cout.flush();
                    
                    // 發送確認
                    std::string ack = "P2P_ACK:" + myUsername;
                    send(clientSocket, ack.c_str(), ack.length(), 0);
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "P2P: Exception handling connection: " << e.what() << std::endl;
        }
        
        close(clientSocket);
    }
    
    // 發送P2P訊息
    bool sendP2PMessage(const std::string& targetIP, int targetPort, const std::string& message) {
        try {
            std::cout << "📤 Sending P2P message to " << targetIP << ":" << targetPort << std::endl;
            
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
            
            // 構造P2P訊息格式
            std::string p2pMessage = "P2P_MSG:" + myUsername + ":" + message;
            
            // 發送訊息
            ssize_t sent = send(targetSocket, p2pMessage.c_str(), p2pMessage.length(), 0);
            if (sent < 0) {
                std::cerr << "P2P: Failed to send message" << std::endl;
                close(targetSocket);
                return false;
            }
            
            // 等待確認
            char ackBuffer[256];
            memset(ackBuffer, 0, sizeof(ackBuffer));
            int ackReceived = recv(targetSocket, ackBuffer, sizeof(ackBuffer) - 1, 0);
            
            if (ackReceived > 0) {
                ackBuffer[ackReceived] = '\0';
                std::string ack(ackBuffer);
                if (ack.find("P2P_ACK:") == 0) {
                    std::cout << "✅ P2P message delivered successfully" << std::endl;
                } else {
                    std::cout << "⚠️ Unexpected response: " << ack << std::endl;
                }
            }
            
            close(targetSocket);
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "P2P: Exception in sendP2PMessage: " << e.what() << std::endl;
            return false;
        }
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