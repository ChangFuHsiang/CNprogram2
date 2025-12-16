#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <exception>
#include <memory>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include "P2PClient.h"
#include "Crypto.h"

using namespace std;

class ChatClient {
private:
    int clientSocket;
    string serverIP;
    int serverPort;
    int myListenPort;
    bool isLoggedIn;
    string currentUser;
    
    // P2P通訊支援
    unique_ptr<P2PClient> p2pClient;
    
    // 加密模組
    Crypto crypto;
    bool encryptionEnabled;
    bool serverSupportsEncryption;
    
    // 非同步訊息接收
    thread receiveThread;
    atomic<bool> receiving{false};
    
public:
    ChatClient(string ip, int port) 
        : serverIP(ip), serverPort(port), myListenPort(0), isLoggedIn(false),
          encryptionEnabled(true), serverSupportsEncryption(false) {
        
        cout << "=== Phase 2 Complete Chat Client ===" << endl;
        cout << "Features:" << endl;
        cout << "  ✅ P2P Direct Messaging" << endl;
        cout << "  ✅ OpenSSL Encryption (AES-256-CBC)" << endl;
        cout << "  ✅ Group Chat" << endl;
        cout << "  ✅ File Transfer" << endl;
        
        if (crypto.selfTest()) {
            cout << "🔐 Encryption enabled" << endl;
        } else {
            cerr << "⚠️ Encryption failed, disabling" << endl;
            encryptionEnabled = false;
        }
    }
    
    bool connectToServer() {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            perror("Socket creation failed");
            return false;
        }
        
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        
        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            perror("Invalid address");
            return false;
        }
        
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Connection failed");
            return false;
        }
        
        cout << "Connected to server " << serverIP << ":" << serverPort << endl;
        
        if (encryptionEnabled) {
            checkServerEncryption();
        }
        
        return true;
    }
    
    void checkServerEncryption() {
        string response = sendCommandRaw("ENCRYPTION_STATUS");
        if (response.find("ENCRYPTION_STATUS:ENABLED") != string::npos) {
            serverSupportsEncryption = true;
            cout << "🔒 Server encryption enabled" << endl;
        } else {
            serverSupportsEncryption = false;
            cout << "⚠️ Server encryption not available" << endl;
        }
    }
    
    string sendCommandRaw(const string& command) {
        const char* data = command.c_str();
        ssize_t sent = send(clientSocket, data, command.length(), 0);
        if (sent <= 0) return "ERROR: Send failed";
        
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        ssize_t received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) return "ERROR: Receive failed";
        
        return string(buffer);
    }
    
    string sendCommand(const string& command) {
        string messageToSend = command;
        bool useEncryption = encryptionEnabled && serverSupportsEncryption;
        
        if (useEncryption) {
            string encrypted = crypto.encryptMessage(command);
            if (!encrypted.empty()) {
                messageToSend = encrypted;
            }
        }
        
        const char* data = messageToSend.c_str();
        ssize_t sent = send(clientSocket, data, messageToSend.length(), 0);
        if (sent <= 0) return "ERROR: Send failed";
        
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        ssize_t received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) return "ERROR: Receive failed";
        
        string response(buffer);
        
        // 解密回應
        if (Crypto::isEncryptedMessage(response)) {
            string decrypted = crypto.decryptMessage(response);
            if (!decrypted.empty()) {
                response = decrypted;
            }
        }
        
        return response;
    }
    
    // 啟動非同步接收群組訊息
    void startReceiving() {
        receiving = true;
        receiveThread = thread([this]() {
            fd_set readfds;
            struct timeval tv;
            char buffer[4096];
            
            while (receiving) {
                FD_ZERO(&readfds);
                FD_SET(clientSocket, &readfds);
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                
                int result = select(clientSocket + 1, &readfds, NULL, NULL, &tv);
                if (result > 0 && FD_ISSET(clientSocket, &readfds)) {
                    memset(buffer, 0, sizeof(buffer));
                    int received = recv(clientSocket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
                    if (received > 0) {
                        string msg(buffer);
                        
                        // 解密
                        if (Crypto::isEncryptedMessage(msg)) {
                            string decrypted = crypto.decryptMessage(msg);
                            if (!decrypted.empty()) {
                                msg = decrypted;
                            }
                        }
                        
                        // 處理群組訊息
                        if (msg.find("ROOM_MSG:") == 0 || msg.find("ROOM_NOTIFICATION:") == 0) {
                            cout << "\n📢 " << msg << endl;
                            cout << "Enter command: " << flush;
                        }
                    }
                }
            }
        });
    }
    
    void stopReceiving() {
        receiving = false;
        if (receiveThread.joinable()) {
            receiveThread.join();
        }
    }
    
    void displayMenu() {
        cout << "\n";
        cout << "╔══════════════════════════════════════════╗" << endl;
        cout << "║       Phase 2 Complete Chat Client       ║" << endl;
        cout << "╚══════════════════════════════════════════╝" << endl;
        
        if (!isLoggedIn) {
            cout << "┌─ Account ─────────────────────────────────┐" << endl;
            cout << "│ 1. Register                               │" << endl;
            cout << "│ 2. Login                                  │" << endl;
            cout << "│ 3. Quit                                   │" << endl;
            cout << "└───────────────────────────────────────────┘" << endl;
        } else {
            cout << "┌─ User: " << currentUser << " (Port: " << myListenPort << ") ─────────┐" << endl;
            cout << "│                                           │" << endl;
            cout << "│ ── Basic ──                               │" << endl;
            cout << "│  1. List online users                     │" << endl;
            cout << "│  2. Logout                                │" << endl;
            cout << "│                                           │" << endl;
            cout << "│ ── P2P Messaging ──                       │" << endl;
            cout << "│  3. Get user info                         │" << endl;
            cout << "│  4. Send P2P message (encrypted)          │" << endl;
            cout << "│                                           │" << endl;
            cout << "│ ── Group Chat ──                          │" << endl;
            cout << "│  5. List rooms                            │" << endl;
            cout << "│  6. Create room                           │" << endl;
            cout << "│  7. Join room                             │" << endl;
            cout << "│  8. Leave room                            │" << endl;
            cout << "│  9. Send room message                     │" << endl;
            cout << "│ 10. View room history                     │" << endl;
            cout << "│ 11. View room members                     │" << endl;
            cout << "│                                           │" << endl;
            cout << "│ ── File Transfer ──                       │" << endl;
            cout << "│ 12. Send file (encrypted)                 │" << endl;
            cout << "│ 13. Set download path                     │" << endl;
            cout << "└───────────────────────────────────────────┘" << endl;
        }
        cout << "Enter command: ";
    }
    
    // ========== 基本功能 ==========
    
    bool handleRegister() {
        string username, password;
        cout << "Enter username: ";
        cin >> username;
        cout << "Enter password: ";
        cin >> password;
        
        string response = sendCommand("REGISTER " + username + " " + password);
        cout << "Server: " << response << endl;
        return response == "REGISTER_SUCCESS";
    }
    
    bool handleLogin() {
        string username, password;
        int port;
        
        cout << "Enter username: ";
        cin >> username;
        cout << "Enter password: ";
        cin >> password;
        cout << "Enter P2P listening port (1025-65535): ";
        cin >> port;
        
        if (port <= 1024 || port > 65535) {
            cout << "❌ Invalid port number" << endl;
            return false;
        }
        
        string response = sendCommand("LOGIN " + username + " " + password + " " + to_string(port));
        cout << "Server: " << response << endl;
        
        if (response == "LOGIN_SUCCESS") {
            isLoggedIn = true;
            currentUser = username;
            myListenPort = port;
            
            // 啟動P2P監聽
            cout << "🚀 Starting P2P listener..." << endl;
            p2pClient.reset(new P2PClient(port, username));
            p2pClient->setEncryption(encryptionEnabled);
            
            if (p2pClient->startP2PListener()) {
                cout << "✅ P2P ready for messages and file transfers" << endl;
            }
            
            // 啟動非同步接收
            startReceiving();
            
            return true;
        }
        return false;
    }
    
    bool handleLogout() {
        string response = sendCommand("LOGOUT");
        cout << "Server: " << response << endl;
        
        if (response == "LOGOUT_SUCCESS") {
            stopReceiving();
            if (p2pClient) {
                p2pClient.reset();
            }
            isLoggedIn = false;
            currentUser = "";
            myListenPort = 0;
            return true;
        }
        return false;
    }
    
    void handleListUsers() {
        string response = sendCommand("LIST");
        cout << "📋 " << response << endl;
    }
    
    // ========== P2P 功能 ==========
    
    void handleGetUserInfo() {
        string targetUser;
        cout << "Enter username: ";
        cin >> targetUser;
        
        string response = sendCommand("GET_USER_INFO " + targetUser);
        
        if (response.find("USER_INFO:") == 0) {
            string info = response.substr(10);
            size_t colonPos = info.find(':');
            if (colonPos != string::npos) {
                cout << "✅ User: " << targetUser << endl;
                cout << "   IP: " << info.substr(0, colonPos) << endl;
                cout << "   Port: " << info.substr(colonPos + 1) << endl;
            }
        } else {
            cout << "❌ " << response << endl;
        }
    }
    
    void handleP2PChat() {
        string targetUser;
        cout << "Enter target username: ";
        cin >> targetUser;
        
        if (targetUser == currentUser) {
            cout << "❌ Cannot send to yourself" << endl;
            return;
        }
        
        string response = sendCommand("GET_USER_INFO " + targetUser);
        if (response.find("USER_INFO:") != 0) {
            cout << "❌ " << response << endl;
            return;
        }
        
        string info = response.substr(10);
        size_t colonPos = info.find(':');
        string targetIP = info.substr(0, colonPos);
        int targetPort = stoi(info.substr(colonPos + 1));
        
        cin.ignore();
        string message;
        cout << "Enter message: ";
        getline(cin, message);
        
        if (p2pClient && p2pClient->sendP2PMessage(targetIP, targetPort, message)) {
            cout << "✅ Message sent!" << endl;
        } else {
            cout << "❌ Failed to send message" << endl;
        }
    }
    
    // ========== 群組聊天 ==========
    
    void handleListRooms() {
        string response = sendCommand("LIST_ROOMS");
        cout << "📋 " << response << endl;
    }
    
    void handleCreateRoom() {
        string roomName;
        cout << "Enter room name: ";
        cin >> roomName;
        
        string response = sendCommand("CREATE_ROOM " + roomName);
        if (response.find("ROOM_CREATED:") == 0) {
            cout << "✅ Room '" << roomName << "' created!" << endl;
        } else {
            cout << "❌ " << response << endl;
        }
    }
    
    void handleJoinRoom() {
        string roomName;
        cout << "Enter room name: ";
        cin >> roomName;
        
        string response = sendCommand("JOIN_ROOM " + roomName);
        if (response.find("ROOM_JOINED:") == 0) {
            cout << "✅ Joined room '" << roomName << "'!" << endl;
        } else {
            cout << "❌ " << response << endl;
        }
    }
    
    void handleLeaveRoom() {
        string roomName;
        cout << "Enter room name: ";
        cin >> roomName;
        
        string response = sendCommand("LEAVE_ROOM " + roomName);
        if (response.find("ROOM_LEFT:") == 0) {
            cout << "✅ Left room '" << roomName << "'!" << endl;
        } else {
            cout << "❌ " << response << endl;
        }
    }
    
    void handleRoomMessage() {
        string roomName;
        cout << "Enter room name: ";
        cin >> roomName;
        
        cin.ignore();
        string message;
        cout << "Enter message: ";
        getline(cin, message);
        
        string response = sendCommand("ROOM_MSG " + roomName + " " + message);
        if (response == "ROOM_MSG_SENT") {
            cout << "✅ Message sent to room!" << endl;
        } else {
            cout << "❌ " << response << endl;
        }
    }
    
    void handleRoomHistory() {
        string roomName;
        cout << "Enter room name: ";
        cin >> roomName;
        
        string response = sendCommand("ROOM_HISTORY " + roomName);
        cout << "📜 " << response << endl;
    }
    
    void handleRoomMembers() {
        string roomName;
        cout << "Enter room name: ";
        cin >> roomName;
        
        string response = sendCommand("ROOM_MEMBERS " + roomName);
        cout << "👥 " << response << endl;
    }
    
    // ========== 檔案傳輸 ==========
    
    void handleSendFile() {
        string targetUser;
        cout << "Enter target username: ";
        cin >> targetUser;
        
        if (targetUser == currentUser) {
            cout << "❌ Cannot send to yourself" << endl;
            return;
        }
        
        string response = sendCommand("GET_USER_INFO " + targetUser);
        if (response.find("USER_INFO:") != 0) {
            cout << "❌ " << response << endl;
            return;
        }
        
        string info = response.substr(10);
        size_t colonPos = info.find(':');
        string targetIP = info.substr(0, colonPos);
        int targetPort = stoi(info.substr(colonPos + 1));
        
        cin.ignore();
        string filepath;
        cout << "Enter file path: ";
        getline(cin, filepath);
        
        if (p2pClient && p2pClient->sendFile(targetIP, targetPort, filepath)) {
            cout << "✅ File transfer complete!" << endl;
        } else {
            cout << "❌ File transfer failed" << endl;
        }
    }
    
    void handleSetDownloadPath() {
        cin.ignore();
        string path;
        cout << "Enter download path: ";
        getline(cin, path);
        
        if (p2pClient) {
            p2pClient->setDownloadPath(path);
            cout << "✅ Download path set to: " << path << endl;
        } else {
            cout << "❌ Not logged in" << endl;
        }
    }
    
    void run() {
        string input;
        
        while (true) {
            displayMenu();
            
            if (!(cin >> input)) {
                break;
            }
            
            if (!isLoggedIn) {
                if (input == "1") handleRegister();
                else if (input == "2") handleLogin();
                else if (input == "3") {
                    cout << "Goodbye!" << endl;
                    break;
                }
                else cout << "Unknown command" << endl;
            } else {
                if (input == "1") handleListUsers();
                else if (input == "2") {
                    if (handleLogout()) cout << "✅ Logged out" << endl;
                }
                else if (input == "3") handleGetUserInfo();
                else if (input == "4") handleP2PChat();
                else if (input == "5") handleListRooms();
                else if (input == "6") handleCreateRoom();
                else if (input == "7") handleJoinRoom();
                else if (input == "8") handleLeaveRoom();
                else if (input == "9") handleRoomMessage();
                else if (input == "10") handleRoomHistory();
                else if (input == "11") handleRoomMembers();
                else if (input == "12") handleSendFile();
                else if (input == "13") handleSetDownloadPath();
                else cout << "Unknown command" << endl;
            }
        }
    }
    
    ~ChatClient() {
        stopReceiving();
        if (p2pClient) p2pClient.reset();
        if (clientSocket >= 0) close(clientSocket);
    }
};

int main(int argc, char* argv[]) {
    string serverIP = "127.0.0.1";
    int serverPort = 8080;
    
    if (argc > 1) serverIP = argv[1];
    if (argc > 2) serverPort = atoi(argv[2]);
    
    cout << "Connecting to " << serverIP << ":" << serverPort << endl;
    
    ChatClient client(serverIP, serverPort);
    
    if (!client.connectToServer()) {
        return 1;
    }
    
    client.run();
    
    return 0;
}
