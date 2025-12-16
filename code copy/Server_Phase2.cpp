#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <exception>
#include <thread>
#include <mutex>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "ThreadPool.h"
#include "Crypto.h"

using namespace std;

struct User {
    string username;
    string password;
    bool isOnline;
    string clientIP;
    int clientPort;
    
    User() : isOnline(false), clientPort(0) {}
    User(string user, string pass) : username(user), password(pass), isOnline(false), clientPort(0) {}
};

class ChatServer {
private:
    int serverSocket;
    int serverPort;
    map<string, User> users;
    mutable mutex users_mutex;
    atomic<int> clientCounter{0};
    
    // Phase 2: Professional ThreadPool
    ThreadPool thread_pool;
    
    // Phase 2: 加密模組
    Crypto crypto;
    bool encryptionEnabled;
    
public:
    ChatServer(int port) : serverPort(port), thread_pool(10), encryptionEnabled(true) {
        cout << "=== Phase 2 ChatServer with Encryption ===" << endl;
        cout << "Initialized with Professional ThreadPool (10 workers)" << endl;
        
        // 測試加密功能
        if (crypto.selfTest()) {
            cout << "🔐 Server encryption enabled (AES-256-CBC)" << endl;
        } else {
            cerr << "⚠️ Encryption self-test failed, disabling encryption" << endl;
            encryptionEnabled = false;
        }
    }
    
    bool startServer() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            perror("Socket creation failed");
            return false;
        }
        
        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt failed");
            return false;
        }
        
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(serverPort);
        
        if (::bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Bind failed");
            return false;
        }
        
        if (listen(serverSocket, 10) < 0) {
            perror("Listen failed");
            return false;
        }
        
        cout << "Phase 2 Server started on port " << serverPort << endl;
        cout << "Worker Pool Status: " << thread_pool.getWorkerCount() << " workers ready" << endl;
        cout << "Encryption Status: " << (encryptionEnabled ? "✅ Enabled" : "❌ Disabled") << endl;
        return true;
    }
    
    void handleClient(int clientSocket, string clientIP, int clientId) {
        char buffer[4096];  // 增大緩衝區以容納加密訊息
        string currentUser = "";
        bool clientEncryptionEnabled = false;
        
        cout << "[Client " << clientId << "] Started handling " << clientIP 
             << " (Worker Thread: " << this_thread::get_id() << ")" << endl;
        
        try {
            while (true) {
                memset(buffer, 0, sizeof(buffer));
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                
                if (bytesReceived <= 0) {
                    if (bytesReceived == 0) {
                        cout << "[Client " << clientId << "] Disconnected gracefully" << endl;
                    } else {
                        cout << "[Client " << clientId << "] Disconnected with error: " << strerror(errno) << endl;
                    }
                    break;
                }
                
                buffer[bytesReceived] = '\0';
                string message(buffer);
                
                // 清理訊息
                size_t pos = message.find_last_not_of(" \n\r\t");
                if (pos != string::npos) {
                    message.erase(pos + 1);
                }
                
                // 檢查是否為加密訊息
                string decryptedMessage = message;
                bool wasEncrypted = false;
                
                if (Crypto::isEncryptedMessage(message)) {
                    decryptedMessage = crypto.decryptMessage(message);
                    wasEncrypted = true;
                    if (decryptedMessage.empty()) {
                        cout << "[Client " << clientId << "] Decryption failed" << endl;
                        string errorResponse = "ERROR: Decryption failed";
                        send(clientSocket, errorResponse.c_str(), errorResponse.length(), 0);
                        continue;
                    }
                    clientEncryptionEnabled = true;
                }
                
                cout << "[Client " << clientId << "] Received: [" << decryptedMessage << "]";
                if (wasEncrypted) cout << " (decrypted)";
                cout << " (Queue size: " << thread_pool.getQueueSize() << ")" << endl;
                
                if (decryptedMessage.empty()) {
                    cout << "[Client " << clientId << "] Empty message, continuing..." << endl;
                    continue;
                }
                
                // 處理指令
                string response;
                try {
                    response = processCommand(decryptedMessage, currentUser, clientIP, clientId);
                } catch (const exception& e) {
                    cout << "[Client " << clientId << "] Command processing error: " << e.what() << endl;
                    response = "ERROR: Command processing failed";
                }
                
                // 如果客戶端使用加密，回應也加密
                string finalResponse = response;
                if (clientEncryptionEnabled && encryptionEnabled) {
                    string encryptedResponse = crypto.encryptMessage(response);
                    if (!encryptedResponse.empty()) {
                        finalResponse = encryptedResponse;
                        cout << "[Client " << clientId << "] Sending: [" << response << "] (encrypted)" << endl;
                    } else {
                        cout << "[Client " << clientId << "] Sending: [" << response << "] (encryption failed, sending plain)" << endl;
                    }
                } else {
                    cout << "[Client " << clientId << "] Sending: [" << response << "]" << endl;
                }
                
                // 發送回應
                const char* responseData = finalResponse.c_str();
                size_t responseLen = finalResponse.length();
                
                ssize_t sent = send(clientSocket, responseData, responseLen, 0);
                if (sent < 0) {
                    cout << "[Client " << clientId << "] Send failed: " << strerror(errno) << endl;
                    break;
                } else if (sent == 0) {
                    cout << "[Client " << clientId << "] Connection closed by peer" << endl;
                    break;
                }
                
                // 檢查登出
                if (decryptedMessage.find("LOGOUT") == 0) {
                    cout << "[Client " << clientId << "] Logout requested, closing connection" << endl;
                    break;
                }
            }
        } catch (const exception& e) {
            cout << "[Client " << clientId << "] Exception in handleClient: " << e.what() << endl;
        }
        
        // 清理資源
        if (!currentUser.empty()) {
            try {
                lock_guard<mutex> lock(users_mutex);
                auto it = users.find(currentUser);
                if (it != users.end()) {
                    it->second.isOnline = false;
                    cout << "[Client " << clientId << "] Cleaned up user: " << currentUser << endl;
                }
            } catch (const exception& e) {
                cout << "[Client " << clientId << "] Cleanup error: " << e.what() << endl;
            }
        }
        
        close(clientSocket);
        cout << "[Client " << clientId << "] Handler finished (Worker: " 
             << this_thread::get_id() << ")" << endl;
    }
    
    string processCommand(const string& command, string& currentUser, const string& clientIP, int clientId) {
        try {
            stringstream ss(command);
            string cmd;
            ss >> cmd;
            
            if (cmd.empty()) {
                return "ERROR: Empty command";
            }
            
            // 轉大寫
            transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
            
            cout << "[Client " << clientId << "] Processing: " << cmd << " for user: [" << currentUser << "]" << endl;
            
            if (cmd == "REGISTER") {
                string username, password;
                ss >> username >> password;
                return handleRegister(username, password, clientId);
            }
            else if (cmd == "LOGIN") {
                string username, password;
                int port;
                ss >> username >> password >> port;
                
                if (ss.fail()) {
                    return "ERROR: Invalid login format";
                }
                
                string result = handleLogin(username, password, clientIP, port, clientId);
                if (result == "LOGIN_SUCCESS") {
                    currentUser = username;
                    cout << "[Client " << clientId << "] User logged in: " << currentUser << endl;
                }
                return result;
            }
            else if (cmd == "LOGOUT") {
                string result = handleLogout(currentUser, clientId);
                if (result == "LOGOUT_SUCCESS") {
                    currentUser = "";
                }
                return result;
            }
            else if (cmd == "LIST") {
                return handleListUsers(clientId);
            }
            else if (cmd == "MESSAGE") {
                string msg;
                getline(ss, msg);
                return handleMessage(currentUser, msg, clientId);
            }
            // Phase 2：P2P用戶資訊查詢
            else if (cmd == "GET_USER_INFO") {
                string targetUser;
                ss >> targetUser;
                return handleGetUserInfo(targetUser, currentUser, clientId);
            }
            // Phase 2：加密狀態查詢
            else if (cmd == "ENCRYPTION_STATUS") {
                return handleEncryptionStatus(clientId);
            }
            else {
                return "ERROR: Unknown command: " + cmd;
            }
        } catch (const exception& e) {
            cout << "[Client " << clientId << "] ProcessCommand exception: " << e.what() << endl;
            return "ERROR: Command processing failed";
        }
    }
    
    string handleRegister(const string& username, const string& password, int clientId) {
        try {
            cout << "[Client " << clientId << "] Register attempt: " << username << endl;
            
            if (username.empty() || password.empty()) {
                cout << "[Client " << clientId << "] Register failed: empty username/password" << endl;
                return "ERROR: Username and password cannot be empty";
            }
            
            lock_guard<mutex> lock(users_mutex);
            
            auto it = users.find(username);
            if (it != users.end()) {
                cout << "[Client " << clientId << "] Register failed: user exists" << endl;
                return "ERROR: Username already exists";
            }
            
            users[username] = User(username, password);
            cout << "[Client " << clientId << "] User registered successfully: " << username << endl;
            return "REGISTER_SUCCESS";
            
        } catch (const exception& e) {
            cout << "[Client " << clientId << "] Register exception: " << e.what() << endl;
            return "ERROR: Registration failed";
        }
    }
    
    string handleLogin(const string& username, const string& password, const string& clientIP, int port, int clientId) {
        try {
            cout << "[Client " << clientId << "] Login attempt: " << username << ":" << port << endl;
            
            if (username.empty() || password.empty()) {
                return "ERROR: Username and password cannot be empty";
            }
            
            if (port <= 1024 || port > 65535) {
                return "ERROR: Port must be between 1025 and 65535";
            }
            
            lock_guard<mutex> lock(users_mutex);
            
            auto it = users.find(username);
            if (it == users.end()) {
                cout << "[Client " << clientId << "] Login failed: user not found" << endl;
                return "ERROR: User not found";
            }
            
            if (it->second.password != password) {
                cout << "[Client " << clientId << "] Login failed: wrong password" << endl;
                return "ERROR: Wrong password";
            }
            
            if (it->second.isOnline) {
                cout << "[Client " << clientId << "] Login failed: already online" << endl;
                return "ERROR: User already logged in";
            }
            
            // 檢查port衝突
            for (const auto& pair : users) {
                if (pair.second.isOnline && pair.second.clientPort == port && pair.first != username) {
                    cout << "[Client " << clientId << "] Login failed: port in use" << endl;
                    return "ERROR: Port already in use";
                }
            }
            
            it->second.isOnline = true;
            it->second.clientIP = clientIP;
            it->second.clientPort = port;
            
            cout << "[Client " << clientId << "] Login successful: " << username 
                 << " (P2P endpoint: " << clientIP << ":" << port << ")" << endl;
            return "LOGIN_SUCCESS";
            
        } catch (const exception& e) {
            cout << "[Client " << clientId << "] Login exception: " << e.what() << endl;
            return "ERROR: Login failed";
        }
    }
    
    string handleLogout(const string& username, int clientId) {
        try {
            if (username.empty()) {
                return "ERROR: Not logged in";
            }
            
            lock_guard<mutex> lock(users_mutex);
            
            auto it = users.find(username);
            if (it != users.end()) {
                it->second.isOnline = false;
                it->second.clientIP = "";
                it->second.clientPort = 0;
                cout << "[Client " << clientId << "] User logged out: " << username << endl;
            }
            
            return "LOGOUT_SUCCESS";
            
        } catch (const exception& e) {
            cout << "[Client " << clientId << "] Logout exception: " << e.what() << endl;
            return "ERROR: Logout failed";
        }
    }
    
    string handleListUsers(int clientId) {
        try {
            lock_guard<mutex> lock(users_mutex);
            
            string result = "ONLINE_USERS:";
            bool hasOnlineUsers = false;
            
            for (const auto& pair : users) {
                if (pair.second.isOnline) {
                    result += " " + pair.first + "(" + pair.second.clientIP + ":" + to_string(pair.second.clientPort) + ")";
                    hasOnlineUsers = true;
                }
            }
            
            if (!hasOnlineUsers) {
                result = "No users online";
            }
            
            cout << "[Client " << clientId << "] Listed users: " << result << endl;
            return result;
            
        } catch (const exception& e) {
            cout << "[Client " << clientId << "] List exception: " << e.what() << endl;
            return "ERROR: List failed";
        }
    }
    
    string handleMessage(const string& sender, const string& message, int clientId) {
        try {
            if (sender.empty()) {
                return "ERROR: Not logged in";
            }
            
            cout << "[Client " << clientId << "] Message from " << sender << ":" << message << endl;
            return "MESSAGE_RECEIVED";
            
        } catch (const exception& e) {
            cout << "[Client " << clientId << "] Message exception: " << e.what() << endl;
            return "ERROR: Message failed";
        }
    }
    
    // Phase 2：獲取用戶P2P連接資訊
    string handleGetUserInfo(const string& targetUser, const string& requester, int clientId) {
        try {
            cout << "[Client " << clientId << "] GetUserInfo request: " << requester << " asking for " << targetUser << endl;
            
            if (requester.empty()) {
                return "ERROR: Not logged in";
            }
            
            if (targetUser.empty()) {
                return "ERROR: Target username cannot be empty";
            }
            
            lock_guard<mutex> lock(users_mutex);
            
            auto it = users.find(targetUser);
            if (it == users.end()) {
                cout << "[Client " << clientId << "] GetUserInfo failed: user not found" << endl;
                return "ERROR: User not found";
            }
            
            if (!it->second.isOnline) {
                cout << "[Client " << clientId << "] GetUserInfo failed: user not online" << endl;
                return "ERROR: User not online";
            }
            
            string result = "USER_INFO:" + it->second.clientIP + ":" + to_string(it->second.clientPort);
            cout << "[Client " << clientId << "] Provided user info for P2P: " << result << endl;
            return result;
            
        } catch (const exception& e) {
            cout << "[Client " << clientId << "] GetUserInfo exception: " << e.what() << endl;
            return "ERROR: Failed to get user info";
        }
    }
    
    // Phase 2：加密狀態查詢
    string handleEncryptionStatus(int clientId) {
        string status = "ENCRYPTION_STATUS:";
        status += encryptionEnabled ? "ENABLED:AES-256-CBC" : "DISABLED";
        cout << "[Client " << clientId << "] Encryption status: " << status << endl;
        return status;
    }
    
    void run() {
        cout << "=== Phase 2 Server Running ===" << endl;
        cout << "ThreadPool Status: " << thread_pool.getWorkerCount() << " workers available" << endl;
        cout << "Encryption: " << (encryptionEnabled ? "✅ AES-256-CBC" : "❌ Disabled") << endl;
        cout << "Ready for concurrent client connections..." << endl;
        
        while (true) {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSocket < 0) {
                perror("Accept failed");
                continue;
            }
            
            string clientIP = inet_ntoa(clientAddr.sin_addr);
            int clientId = ++clientCounter;
            
            cout << "[Client " << clientId << "] New connection from " << clientIP 
                 << " - assigning to ThreadPool (Queue: " << thread_pool.getQueueSize() << ")" << endl;
            
            // Phase 2: 使用Professional ThreadPool
            try {
                thread_pool.enqueue([this, clientSocket, clientIP, clientId]() {
                    this->handleClient(clientSocket, clientIP, clientId);
                });
                cout << "[Client " << clientId << "] Task enqueued successfully" << endl;
            } catch (const exception& e) {
                cout << "[Client " << clientId << "] Failed to enqueue task: " << e.what() << endl;
                close(clientSocket);
            }
        }
    }
    
    ~ChatServer() {
        cout << "=== ChatServer Shutdown ===" << endl;
        if (serverSocket >= 0) {
            close(serverSocket);
        }
        cout << "ThreadPool will cleanup automatically..." << endl;
    }
};

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0) {
            cout << "Invalid port number" << endl;
            return 1;
        }
    }
    
    cout << "=== Phase 2 Socket Programming Server ===" << endl;
    cout << "Features:" << endl;
    cout << "  ✅ Professional ThreadPool (10 workers)" << endl;
    cout << "  ✅ P2P User Discovery" << endl;
    cout << "  ✅ OpenSSL Encryption (AES-256-CBC)" << endl;
    cout << "Starting on port " << port << endl;
    
    ChatServer server(port);
    
    if (!server.startServer()) {
        return 1;
    }
    
    server.run();
    
    return 0;
}