#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <exception>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <memory>
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
    int clientSocket;  // 用於群組訊息推送
    
    User() : isOnline(false), clientPort(0), clientSocket(-1) {}
    User(string user, string pass) : username(user), password(pass), isOnline(false), clientPort(0), clientSocket(-1) {}
};

// 群組結構
struct ChatRoom {
    string roomName;
    string creator;
    set<string> members;
    vector<pair<string, string>> messageHistory;  // (sender, message)
    mutable shared_ptr<mutex> room_mutex;
    
    ChatRoom() : room_mutex(make_shared<mutex>()) {}
    ChatRoom(const string& name, const string& owner) 
        : roomName(name), creator(owner), room_mutex(make_shared<mutex>()) {
        members.insert(owner);
    }
};

class ChatServer {
private:
    int serverSocket;
    int serverPort;
    map<string, User> users;
    mutable mutex users_mutex;
    atomic<int> clientCounter{0};
    
    // Phase 2: 群組聊天
    map<string, ChatRoom> chatRooms;
    mutable mutex rooms_mutex;
    
    // Phase 2: Professional ThreadPool
    ThreadPool thread_pool;
    
    // Phase 2: 加密模組
    Crypto crypto;
    bool encryptionEnabled;
    
    // 客戶端 socket 映射（用於訊息推送）
    map<string, int> userSockets;
    mutable mutex sockets_mutex;
    
public:
    ChatServer(int port) : serverPort(port), thread_pool(10), encryptionEnabled(true) {
        cout << "=== Phase 2 ChatServer (Complete) ===" << endl;
        cout << "Features:" << endl;
        cout << "  ✅ Professional ThreadPool (10 workers)" << endl;
        cout << "  ✅ P2P User Discovery" << endl;
        cout << "  ✅ OpenSSL Encryption (AES-256-CBC)" << endl;
        cout << "  ✅ Group Chat (Relay Mode)" << endl;
        
        // 測試加密功能
        if (crypto.selfTest()) {
            cout << "🔐 Server encryption enabled" << endl;
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
        
        cout << "Server started on port " << serverPort << endl;
        cout << "Worker Pool: " << thread_pool.getWorkerCount() << " workers ready" << endl;
        return true;
    }
    
    void handleClient(int clientSocket, string clientIP, int clientId) {
        char buffer[4096];
        string currentUser = "";
        
        cout << "[Client " << clientId << "] Started handling " << clientIP 
             << " (Worker: " << this_thread::get_id() << ")" << endl;
        
        try {
            while (true) {
                memset(buffer, 0, sizeof(buffer));
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                
                if (bytesReceived <= 0) {
                    if (bytesReceived == 0) {
                        cout << "[Client " << clientId << "] Disconnected gracefully" << endl;
                    } else {
                        cout << "[Client " << clientId << "] Disconnected with error" << endl;
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
                        string errorResponse = "ERROR: Decryption failed";
                        send(clientSocket, errorResponse.c_str(), errorResponse.length(), 0);
                        continue;
                    }
                }
                
                cout << "[Client " << clientId << "] Received: [" << decryptedMessage << "]";
                if (wasEncrypted) cout << " (decrypted)";
                cout << endl;
                
                if (decryptedMessage.empty()) continue;
                
                // 處理指令
                string response;
                try {
                    response = processCommand(decryptedMessage, currentUser, clientIP, clientId, clientSocket);
                } catch (const exception& e) {
                    response = "ERROR: Command processing failed";
                }
                
                // 加密回應（如果需要）
                string finalResponse = response;
                if (wasEncrypted && encryptionEnabled) {
                    string encrypted = crypto.encryptMessage(response);
                    if (!encrypted.empty()) {
                        finalResponse = encrypted;
                    }
                }
                
                cout << "[Client " << clientId << "] Sending: [" << response << "]" << endl;
                
                ssize_t sent = send(clientSocket, finalResponse.c_str(), finalResponse.length(), 0);
                if (sent <= 0) break;
                
                if (decryptedMessage.find("LOGOUT") == 0) {
                    break;
                }
            }
        } catch (const exception& e) {
            cout << "[Client " << clientId << "] Exception: " << e.what() << endl;
        }
        
        // 清理
        if (!currentUser.empty()) {
            // 離開所有群組
            leaveAllRooms(currentUser);
            
            // 移除 socket 映射
            {
                lock_guard<mutex> lock(sockets_mutex);
                userSockets.erase(currentUser);
            }
            
            // 更新用戶狀態
            {
                lock_guard<mutex> lock(users_mutex);
                auto it = users.find(currentUser);
                if (it != users.end()) {
                    it->second.isOnline = false;
                    it->second.clientSocket = -1;
                }
            }
        }
        
        close(clientSocket);
        cout << "[Client " << clientId << "] Handler finished" << endl;
    }
    
    string processCommand(const string& command, string& currentUser, const string& clientIP, 
                         int clientId, int clientSocket) {
        stringstream ss(command);
        string cmd;
        ss >> cmd;
        
        if (cmd.empty()) return "ERROR: Empty command";
        
        transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
        
        // 基本命令
        if (cmd == "REGISTER") {
            string username, password;
            ss >> username >> password;
            return handleRegister(username, password, clientId);
        }
        else if (cmd == "LOGIN") {
            string username, password;
            int port;
            ss >> username >> password >> port;
            if (ss.fail()) return "ERROR: Invalid login format";
            
            string result = handleLogin(username, password, clientIP, port, clientId, clientSocket);
            if (result == "LOGIN_SUCCESS") {
                currentUser = username;
                // 儲存 socket 映射
                lock_guard<mutex> lock(sockets_mutex);
                userSockets[username] = clientSocket;
            }
            return result;
        }
        else if (cmd == "LOGOUT") {
            string result = handleLogout(currentUser, clientId);
            if (result == "LOGOUT_SUCCESS") {
                leaveAllRooms(currentUser);
                lock_guard<mutex> lock(sockets_mutex);
                userSockets.erase(currentUser);
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
        else if (cmd == "GET_USER_INFO") {
            string targetUser;
            ss >> targetUser;
            return handleGetUserInfo(targetUser, currentUser, clientId);
        }
        else if (cmd == "ENCRYPTION_STATUS") {
            return encryptionEnabled ? "ENCRYPTION_STATUS:ENABLED:AES-256-CBC" : "ENCRYPTION_STATUS:DISABLED";
        }
        // ========== 群組聊天命令 ==========
        else if (cmd == "CREATE_ROOM") {
            string roomName;
            ss >> roomName;
            return handleCreateRoom(roomName, currentUser, clientId);
        }
        else if (cmd == "JOIN_ROOM") {
            string roomName;
            ss >> roomName;
            return handleJoinRoom(roomName, currentUser, clientId);
        }
        else if (cmd == "LEAVE_ROOM") {
            string roomName;
            ss >> roomName;
            return handleLeaveRoom(roomName, currentUser, clientId);
        }
        else if (cmd == "LIST_ROOMS") {
            return handleListRooms(clientId);
        }
        else if (cmd == "ROOM_MEMBERS") {
            string roomName;
            ss >> roomName;
            return handleRoomMembers(roomName, currentUser, clientId);
        }
        else if (cmd == "ROOM_MSG") {
            string roomName;
            ss >> roomName;
            string msg;
            getline(ss, msg);
            // 去除前導空格
            if (!msg.empty() && msg[0] == ' ') msg = msg.substr(1);
            return handleRoomMessage(roomName, currentUser, msg, clientId);
        }
        else if (cmd == "ROOM_HISTORY") {
            string roomName;
            ss >> roomName;
            return handleRoomHistory(roomName, currentUser, clientId);
        }
        else {
            return "ERROR: Unknown command: " + cmd;
        }
    }
    
    // ========== 基本用戶管理 ==========
    
    string handleRegister(const string& username, const string& password, int clientId) {
        if (username.empty() || password.empty()) {
            return "ERROR: Username and password cannot be empty";
        }
        
        lock_guard<mutex> lock(users_mutex);
        if (users.find(username) != users.end()) {
            return "ERROR: Username already exists";
        }
        
        users[username] = User(username, password);
        cout << "[Client " << clientId << "] Registered: " << username << endl;
        return "REGISTER_SUCCESS";
    }
    
    string handleLogin(const string& username, const string& password, const string& clientIP, 
                      int port, int clientId, int clientSocket) {
        if (username.empty() || password.empty()) {
            return "ERROR: Username and password cannot be empty";
        }
        
        if (port <= 1024 || port > 65535) {
            return "ERROR: Port must be between 1025 and 65535";
        }
        
        lock_guard<mutex> lock(users_mutex);
        
        auto it = users.find(username);
        if (it == users.end()) return "ERROR: User not found";
        if (it->second.password != password) return "ERROR: Wrong password";
        if (it->second.isOnline) return "ERROR: User already logged in";
        
        // 檢查port衝突
        for (const auto& pair : users) {
            if (pair.second.isOnline && pair.second.clientPort == port && pair.first != username) {
                return "ERROR: Port already in use";
            }
        }
        
        it->second.isOnline = true;
        it->second.clientIP = clientIP;
        it->second.clientPort = port;
        it->second.clientSocket = clientSocket;
        
        cout << "[Client " << clientId << "] Login: " << username << " (" << clientIP << ":" << port << ")" << endl;
        return "LOGIN_SUCCESS";
    }
    
    string handleLogout(const string& username, int clientId) {
        if (username.empty()) return "ERROR: Not logged in";
        
        lock_guard<mutex> lock(users_mutex);
        auto it = users.find(username);
        if (it != users.end()) {
            it->second.isOnline = false;
            it->second.clientIP = "";
            it->second.clientPort = 0;
            it->second.clientSocket = -1;
        }
        
        cout << "[Client " << clientId << "] Logout: " << username << endl;
        return "LOGOUT_SUCCESS";
    }
    
    string handleListUsers(int clientId) {
        lock_guard<mutex> lock(users_mutex);
        
        string result = "ONLINE_USERS:";
        bool hasOnline = false;
        
        for (const auto& pair : users) {
            if (pair.second.isOnline) {
                result += " " + pair.first + "(" + pair.second.clientIP + ":" + 
                         to_string(pair.second.clientPort) + ")";
                hasOnline = true;
            }
        }
        
        return hasOnline ? result : "No users online";
    }
    
    string handleMessage(const string& sender, const string& message, int clientId) {
        if (sender.empty()) return "ERROR: Not logged in";
        cout << "[Client " << clientId << "] Message from " << sender << ":" << message << endl;
        return "MESSAGE_RECEIVED";
    }
    
    string handleGetUserInfo(const string& targetUser, const string& requester, int clientId) {
        if (requester.empty()) return "ERROR: Not logged in";
        if (targetUser.empty()) return "ERROR: Target username cannot be empty";
        
        lock_guard<mutex> lock(users_mutex);
        auto it = users.find(targetUser);
        if (it == users.end()) return "ERROR: User not found";
        if (!it->second.isOnline) return "ERROR: User not online";
        
        return "USER_INFO:" + it->second.clientIP + ":" + to_string(it->second.clientPort);
    }
    
    // ========== 群組聊天功能 ==========
    
    string handleCreateRoom(const string& roomName, const string& creator, int clientId) {
        if (creator.empty()) return "ERROR: Not logged in";
        if (roomName.empty()) return "ERROR: Room name cannot be empty";
        
        lock_guard<mutex> lock(rooms_mutex);
        
        if (chatRooms.find(roomName) != chatRooms.end()) {
            return "ERROR: Room already exists";
        }
        
        chatRooms[roomName] = ChatRoom(roomName, creator);
        cout << "[Client " << clientId << "] Created room: " << roomName << " by " << creator << endl;
        return "ROOM_CREATED:" + roomName;
    }
    
    string handleJoinRoom(const string& roomName, const string& username, int clientId) {
        if (username.empty()) return "ERROR: Not logged in";
        if (roomName.empty()) return "ERROR: Room name cannot be empty";
        
        lock_guard<mutex> lock(rooms_mutex);
        
        auto it = chatRooms.find(roomName);
        if (it == chatRooms.end()) {
            return "ERROR: Room not found";
        }
        
        lock_guard<mutex> roomLock(*it->second.room_mutex);
        
        if (it->second.members.find(username) != it->second.members.end()) {
            return "ERROR: Already in room";
        }
        
        it->second.members.insert(username);
        
        // 通知其他成員
        broadcastToRoom(roomName, "ROOM_NOTIFICATION:" + roomName + ":" + username + " joined the room", username);
        
        cout << "[Client " << clientId << "] " << username << " joined room: " << roomName << endl;
        return "ROOM_JOINED:" + roomName;
    }
    
    string handleLeaveRoom(const string& roomName, const string& username, int clientId) {
        if (username.empty()) return "ERROR: Not logged in";
        if (roomName.empty()) return "ERROR: Room name cannot be empty";
        
        lock_guard<mutex> lock(rooms_mutex);
        
        auto it = chatRooms.find(roomName);
        if (it == chatRooms.end()) {
            return "ERROR: Room not found";
        }
        
        lock_guard<mutex> roomLock(*it->second.room_mutex);
        
        if (it->second.members.find(username) == it->second.members.end()) {
            return "ERROR: Not in room";
        }
        
        it->second.members.erase(username);
        
        // 通知其他成員
        broadcastToRoom(roomName, "ROOM_NOTIFICATION:" + roomName + ":" + username + " left the room", username);
        
        cout << "[Client " << clientId << "] " << username << " left room: " << roomName << endl;
        return "ROOM_LEFT:" + roomName;
    }
    
    string handleListRooms(int clientId) {
        lock_guard<mutex> lock(rooms_mutex);
        
        if (chatRooms.empty()) {
            return "No rooms available";
        }
        
        string result = "ROOMS:";
        for (const auto& pair : chatRooms) {
            result += " " + pair.first + "(" + to_string(pair.second.members.size()) + " members)";
        }
        
        return result;
    }
    
    string handleRoomMembers(const string& roomName, const string& username, int clientId) {
        if (username.empty()) return "ERROR: Not logged in";
        
        lock_guard<mutex> lock(rooms_mutex);
        
        auto it = chatRooms.find(roomName);
        if (it == chatRooms.end()) {
            return "ERROR: Room not found";
        }
        
        lock_guard<mutex> roomLock(*it->second.room_mutex);
        
        if (it->second.members.find(username) == it->second.members.end()) {
            return "ERROR: Not in room";
        }
        
        string result = "ROOM_MEMBERS:" + roomName + ":";
        for (const string& member : it->second.members) {
            result += " " + member;
        }
        
        return result;
    }
    
    string handleRoomMessage(const string& roomName, const string& sender, const string& message, int clientId) {
        if (sender.empty()) return "ERROR: Not logged in";
        if (roomName.empty()) return "ERROR: Room name cannot be empty";
        if (message.empty()) return "ERROR: Message cannot be empty";
        
        lock_guard<mutex> lock(rooms_mutex);
        
        auto it = chatRooms.find(roomName);
        if (it == chatRooms.end()) {
            return "ERROR: Room not found";
        }
        
        lock_guard<mutex> roomLock(*it->second.room_mutex);
        
        if (it->second.members.find(sender) == it->second.members.end()) {
            return "ERROR: Not in room";
        }
        
        // 儲存訊息歷史
        it->second.messageHistory.push_back({sender, message});
        
        // 廣播訊息給所有成員（包括發送者，讓他知道訊息已發送）
        string broadcastMsg = "ROOM_MSG:" + roomName + ":" + sender + ":" + message;
        broadcastToRoom(roomName, broadcastMsg, "");  // 空字串表示發給所有人
        
        cout << "[Client " << clientId << "] Room message in " << roomName << " from " << sender << endl;
        return "ROOM_MSG_SENT";
    }
    
    string handleRoomHistory(const string& roomName, const string& username, int clientId) {
        if (username.empty()) return "ERROR: Not logged in";
        
        lock_guard<mutex> lock(rooms_mutex);
        
        auto it = chatRooms.find(roomName);
        if (it == chatRooms.end()) {
            return "ERROR: Room not found";
        }
        
        lock_guard<mutex> roomLock(*it->second.room_mutex);
        
        if (it->second.members.find(username) == it->second.members.end()) {
            return "ERROR: Not in room";
        }
        
        if (it->second.messageHistory.empty()) {
            return "ROOM_HISTORY:" + roomName + ":No messages";
        }
        
        string result = "ROOM_HISTORY:" + roomName + ":";
        // 只返回最後 20 條訊息
        size_t start = it->second.messageHistory.size() > 20 ? it->second.messageHistory.size() - 20 : 0;
        for (size_t i = start; i < it->second.messageHistory.size(); ++i) {
            result += "\n  [" + it->second.messageHistory[i].first + "]: " + 
                     it->second.messageHistory[i].second;
        }
        
        return result;
    }
    
    // 廣播訊息給群組成員
    void broadcastToRoom(const string& roomName, const string& message, const string& excludeUser) {
        // 需要在已經持有 rooms_mutex 的情況下調用
        auto it = chatRooms.find(roomName);
        if (it == chatRooms.end()) return;
        
        lock_guard<mutex> sockLock(sockets_mutex);
        
        for (const string& member : it->second.members) {
            if (member == excludeUser) continue;
            
            auto sockIt = userSockets.find(member);
            if (sockIt != userSockets.end() && sockIt->second >= 0) {
                // 注意：這裡使用非阻塞方式發送，避免死鎖
                // 實際應用中可能需要更複雜的訊息佇列機制
                string encMsg = message;
                if (encryptionEnabled) {
                    string encrypted = crypto.encryptMessage(message);
                    if (!encrypted.empty()) {
                        encMsg = encrypted;
                    }
                }
                string msgToSend = encMsg + "\n";
                send(sockIt->second, msgToSend.c_str(), msgToSend.length(), MSG_NOSIGNAL);
            }
        }
    }
    
    // 離開所有群組
    void leaveAllRooms(const string& username) {
        lock_guard<mutex> lock(rooms_mutex);
        
        for (auto& pair : chatRooms) {
            lock_guard<mutex> roomLock(*pair.second.room_mutex);
            pair.second.members.erase(username);
        }
    }
    
    void run() {
        cout << "\n=== Server Running ===" << endl;
        cout << "Ready for connections..." << endl;
        
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
            
            cout << "[Client " << clientId << "] New connection from " << clientIP << endl;
            
            try {
                thread_pool.enqueue([this, clientSocket, clientIP, clientId]() {
                    this->handleClient(clientSocket, clientIP, clientId);
                });
            } catch (const exception& e) {
                cout << "[Client " << clientId << "] Failed to enqueue: " << e.what() << endl;
                close(clientSocket);
            }
        }
    }
    
    ~ChatServer() {
        if (serverSocket >= 0) {
            close(serverSocket);
        }
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
    
    cout << "=== Phase 2 Complete Server ===" << endl;
    cout << "Starting on port " << port << endl;
    
    ChatServer server(port);
    
    if (!server.startServer()) {
        return 1;
    }
    
    server.run();
    
    return 0;
}
