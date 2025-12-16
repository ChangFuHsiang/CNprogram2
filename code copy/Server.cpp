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
    
public:
    ChatServer(int port) : serverPort(port) {}
    
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
        return true;
    }
    
    void handleClient(int clientSocket, string clientIP, int clientId) {
        char buffer[1024];
        string currentUser = "";
        
        cout << "[Client " << clientId << "] Started handling " << clientIP << endl;
        
        try {
            while (true) {
                // 清空buffer
                memset(buffer, 0, sizeof(buffer));
                
                // 接收資料
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                
                if (bytesReceived <= 0) {
                    if (bytesReceived == 0) {
                        cout << "[Client " << clientId << "] Disconnected gracefully" << endl;
                    } else {
                        cout << "[Client " << clientId << "] Disconnected with error: " << strerror(errno) << endl;
                    }
                    break;
                }
                
                // 確保字串以null結尾
                buffer[bytesReceived] = '\0';
                string message(buffer);
                
                // 移除可能的換行符
                size_t pos = message.find_last_not_of(" \n\r\t");
                if (pos != string::npos) {
                    message.erase(pos + 1);
                }
                
                cout << "[Client " << clientId << "] Received: [" << message << "]" << endl;
                
                if (message.empty()) {
                    cout << "[Client " << clientId << "] Empty message, continuing..." << endl;
                    continue;
                }
                
                // 處理指令
                string response;
                try {
                    response = processCommand(message, currentUser, clientIP, clientId);
                } catch (const exception& e) {
                    cout << "[Client " << clientId << "] Command processing error: " << e.what() << endl;
                    response = "ERROR: Command processing failed";
                }
                
                cout << "[Client " << clientId << "] Sending: [" << response << "]" << endl;
                
                // 發送回應
                const char* responseData = response.c_str();
                size_t responseLen = response.length();
                
                ssize_t sent = send(clientSocket, responseData, responseLen, 0);
                if (sent < 0) {
                    cout << "[Client " << clientId << "] Send failed: " << strerror(errno) << endl;
                    break;
                } else if (sent == 0) {
                    cout << "[Client " << clientId << "] Connection closed by peer" << endl;
                    break;
                }
                
                // 檢查登出
                if (message.find("LOGOUT") == 0) {
                    cout << "[Client " << clientId << "] Logout requested, closing connection" << endl;
                    break;
                }
            }
        } catch (const exception& e) {
            cout << "[Client " << clientId << "] Exception: " << e.what() << endl;
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
        cout << "[Client " << clientId << "] Handler finished" << endl;
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
            
            cout << "[Client " << clientId << "] Login successful: " << username << endl;
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
    
    void run() {
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
            
            // 建立thread處理客戶端
            thread clientThread([this, clientSocket, clientIP, clientId]() {
                this->handleClient(clientSocket, clientIP, clientId);
            });
            
            clientThread.detach();
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
    
    cout << "Starting server on port " << port << endl;
    
    ChatServer server(port);
    
    if (!server.startServer()) {
        return 1;
    }
    
    server.run();
    
    return 0;
}