#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <exception>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "P2PClient.h"

using namespace std;

class ChatClient {
private:
    int clientSocket;
    string serverIP;
    int serverPort;
    int myListenPort;
    bool isLoggedIn;
    string currentUser;
    
    // Phase 2: P2P通訊支援
    unique_ptr<P2PClient> p2pClient;
    
public:
    ChatClient(string ip, int port) : serverIP(ip), serverPort(port), myListenPort(0), isLoggedIn(false) {
        cout << "=== Phase 2 Chat Client with P2P Support ===" << endl;
        cout << "Ready for direct peer-to-peer messaging!" << endl;
    }
    
    bool connectToServer() {
        // 建立socket
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            perror("Socket creation failed");
            return false;
        }
        
        // 設定server地址
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        
        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            perror("Invalid address");
            return false;
        }
        
        // 連接到server
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Connection failed");
            return false;
        }
        
        cout << "Connected to Phase 2 server " << serverIP << ":" << serverPort << endl;
        return true;
    }
    
    string sendCommand(const string& command) {
        try {
            cout << "Sending: [" << command << "]" << endl;
            
            // 發送指令
            const char* data = command.c_str();
            size_t len = command.length();
            
            ssize_t sent = send(clientSocket, data, len, 0);
            if (sent < 0) {
                cout << "Send error: " << strerror(errno) << endl;
                return "ERROR: Send failed";
            }
            if (sent == 0) {
                cout << "Connection closed by server during send" << endl;
                return "ERROR: Connection closed";
            }
            if ((size_t)sent != len) {
                cout << "Partial send: " << sent << "/" << len << " bytes" << endl;
                return "ERROR: Partial send";
            }
            
            // 接收回應
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            
            ssize_t received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (received < 0) {
                cout << "Receive error: " << strerror(errno) << endl;
                return "ERROR: Receive failed";
            }
            if (received == 0) {
                cout << "Server closed connection" << endl;
                return "ERROR: Server closed connection";
            }
            
            // 確保字串以null結尾
            buffer[received] = '\0';
            string response(buffer);
            
            cout << "Received: [" << response << "]" << endl;
            return response;
            
        } catch (const exception& e) {
            cout << "Communication exception: " << e.what() << endl;
            return "ERROR: Communication error";
        }
    }
    
    void displayMenu() {
        cout << "\n=== Phase 2 Chat Client ===" << endl;
        if (!isLoggedIn) {
            cout << "1. Register - REGISTER <username> <password>" << endl;
            cout << "2. Login - LOGIN <username> <password> <listen_port>" << endl;
            cout << "3. Quit - QUIT" << endl;
        } else {
            cout << "Logged in as: " << currentUser << " (P2P port: " << myListenPort << ")" << endl;
            cout << "1. List online users - LIST" << endl;
            cout << "2. Send server message - MESSAGE <your_message>" << endl;
            cout << "3. Get user info (P2P) - GET_USER_INFO <username>" << endl;
            cout << "4. Send P2P message - P2P_CHAT <username> <message>" << endl;
            cout << "5. Logout - LOGOUT" << endl;
        }
        cout << "Enter command: ";
    }
    
    bool handleRegister() {
        string username, password;
        cout << "Enter username: ";
        cin >> username;
        cout << "Enter password: ";
        cin >> password;
        
        string command = "REGISTER " + username + " " + password;
        string response = sendCommand(command);
        
        cout << "Server response: " << response << endl;
        return response == "REGISTER_SUCCESS";
    }
    
    bool handleLogin() {
        string username, password;
        int port;
        
        cout << "Enter username: ";
        cin >> username;
        cout << "Enter password: ";
        cin >> password;
        cout << "Enter your listening port (for P2P communication): ";
        cin >> port;
        
        if (port <= 1024 || port > 65535) {
            cout << "Error: Please use port number between 1025-65535" << endl;
            return false;
        }
        
        string command = "LOGIN " + username + " " + password + " " + to_string(port);
        string response = sendCommand(command);
        
        cout << "Server response: " << response << endl;
        
        if (response == "LOGIN_SUCCESS") {
            isLoggedIn = true;
            currentUser = username;
            myListenPort = port;
            
            // Phase 2: 啟動P2P監聽
            cout << "🚀 Starting P2P listener..." << endl;
            p2pClient.reset(new P2PClient(port, username));
            
            if (p2pClient->startP2PListener()) {
                cout << "✅ P2P system ready! You can now send/receive direct messages" << endl;
                cout << "💡 Use option 4 to send P2P messages to other users" << endl;
            } else {
                cout << "⚠️ P2P listener failed to start, but server login successful" << endl;
            }
            
            return true;
        }
        return false;
    }
    
    bool handleLogout() {
        string command = "LOGOUT";
        string response = sendCommand(command);
        
        cout << "Server response: " << response << endl;
        
        if (response == "LOGOUT_SUCCESS") {
            // Phase 2: 停止P2P監聽
            if (p2pClient) {
                cout << "🛑 Stopping P2P listener..." << endl;
                p2pClient.reset();
                cout << "✅ P2P system shutdown complete" << endl;
            }
            
            isLoggedIn = false;
            currentUser = "";
            myListenPort = 0;
            return true;
        }
        return false;
    }
    
    void handleListUsers() {
        string command = "LIST";
        string response = sendCommand(command);
        
        cout << "Online users: " << response << endl;
    }
    
    void handleMessage() {
        cin.ignore(); // 清除緩衝區的換行符
        string message;
        cout << "Enter your message: ";
        getline(cin, message);
        
        string command = "MESSAGE " + message;
        string response = sendCommand(command);
        
        cout << "Server response: " << response << endl;
    }
    
    // Phase 2新增：發送P2P訊息
    void handleP2PChat() {
        string targetUser;
        cout << "Enter target username: ";
        cin >> targetUser;
        
        if (targetUser.empty()) {
            cout << "Error: Target username cannot be empty" << endl;
            return;
        }
        
        if (targetUser == currentUser) {
            cout << "Error: Cannot send message to yourself" << endl;
            return;
        }
        
        // 首先從server獲取目標用戶的P2P資訊
        string command = "GET_USER_INFO " + targetUser;
        string response = sendCommand(command);
        
        if (response.find("USER_INFO:") != 0) {
            cout << "Error getting user info: " << response << endl;
            return;
        }
        
        // 解析用戶資訊：USER_INFO:IP:PORT
        string info = response.substr(10); // 去掉 "USER_INFO:" prefix
        size_t colonPos = info.find(':');
        if (colonPos == string::npos) {
            cout << "Error: Invalid user info format" << endl;
            return;
        }
        
        string targetIP = info.substr(0, colonPos);
        int targetPort = stoi(info.substr(colonPos + 1));
        
        cout << "📍 Target: " << targetUser << " at " << targetIP << ":" << targetPort << endl;
        
        // 獲取要發送的訊息
        cin.ignore(); // 清除緩衝區
        string message;
        cout << "Enter your P2P message: ";
        getline(cin, message);
        
        if (message.empty()) {
            cout << "Error: Message cannot be empty" << endl;
            return;
        }
        
        // 發送P2P訊息
        if (p2pClient) {
            cout << "📤 Sending P2P message..." << endl;
            if (p2pClient->sendP2PMessage(targetIP, targetPort, message)) {
                cout << "✅ P2P message sent successfully to " << targetUser << endl;
            } else {
                cout << "❌ Failed to send P2P message to " << targetUser << endl;
                cout << "💡 Make sure the target user is online and reachable" << endl;
            }
        } else {
            cout << "Error: P2P system not initialized" << endl;
        }
    }
    
    // Phase 2新增：查詢用戶P2P資訊
    void handleGetUserInfo() {
        string targetUser;
        cout << "Enter username to get P2P info: ";
        cin >> targetUser;
        
        if (targetUser.empty()) {
            cout << "Error: Username cannot be empty" << endl;
            return;
        }
        
        string command = "GET_USER_INFO " + targetUser;
        string response = sendCommand(command);
        
        if (response.find("USER_INFO:") == 0) {
            // 解析回應：USER_INFO:IP:PORT
            string info = response.substr(10); // 去掉 "USER_INFO:" prefix
            size_t colonPos = info.find(':');
            if (colonPos != string::npos) {
                string ip = info.substr(0, colonPos);
                string port = info.substr(colonPos + 1);
                
                cout << "✅ User " << targetUser << " P2P Info:" << endl;
                cout << "   IP: " << ip << endl;
                cout << "   Port: " << port << endl;
                cout << "   Status: Ready for P2P messaging" << endl;
            } else {
                cout << "Error: Invalid user info format" << endl;
            }
        } else {
            cout << "Server response: " << response << endl;
        }
    }
    
    void run() {
        string input;
        
        while (true) {
            try {
                displayMenu();
                
                if (!(cin >> input)) {
                    cout << "Input error occurred. Exiting..." << endl;
                    break;
                }
                
                if (!isLoggedIn) {
                    if (input == "REGISTER" || input == "1") {
                        handleRegister();
                    }
                    else if (input == "LOGIN" || input == "2") {
                        if (!handleLogin()) {
                            cout << "Login failed. ";
                        }
                    }
                    else if (input == "QUIT" || input == "3") {
                        cout << "Goodbye!" << endl;
                        break;
                    }
                    else {
                        cout << "Unknown command. Please try again." << endl;
                    }
                } else {
                    if (input == "LIST" || input == "1") {
                        handleListUsers();
                    }
                    else if (input == "MESSAGE" || input == "2") {
                        handleMessage();
                    }
                    else if (input == "GET_USER_INFO" || input == "3") {
                        handleGetUserInfo();
                    }
                    else if (input == "P2P_CHAT" || input == "4") {
                        handleP2PChat();
                    }
                    else if (input == "LOGOUT" || input == "5") {
                        if (handleLogout()) {
                            cout << "Logged out successfully." << endl;
                        } else {
                            cout << "Logout failed." << endl;
                        }
                    }
                    else {
                        cout << "Unknown command. Please try again." << endl;
                    }
                }
                
                // 檢查是否有嚴重錯誤導致連接中斷
                if (errno == EPIPE || errno == ECONNRESET) {
                    cout << "Connection lost. Please restart the client." << endl;
                    break;
                }
                
            } catch (const exception& e) {
                cout << "An error occurred: " << e.what() << endl;
                cout << "Please try again or restart the client." << endl;
            }
        }
    }
    
    ~ChatClient() {
        // Phase 2: 確保P2P系統正確關閉
        if (p2pClient) {
            cout << "🛑 Shutting down P2P system..." << endl;
            p2pClient.reset();
        }
        
        if (clientSocket >= 0) {
            close(clientSocket);
        }
        
        cout << "👋 Chat client shutdown complete" << endl;
    }
};

int main(int argc, char* argv[]) {
    string serverIP = "127.0.0.1"; // 預設localhost
    int serverPort = 8080; // 預設port
    
    if (argc > 1) {
        serverIP = argv[1];
    }
    if (argc > 2) {
        serverPort = atoi(argv[2]);
    }
    
    cout << "=== Phase 2 Chat Client with P2P Messaging ===" << endl;
    cout << "Features:" << endl;
    cout << "✅ Server-based user management" << endl;
    cout << "✅ Direct P2P messaging (bypasses server)" << endl;
    cout << "✅ Real-time message receiving" << endl;
    cout << "Connecting to server: " << serverIP << ":" << serverPort << endl;
    
    ChatClient client(serverIP, serverPort);
    
    if (!client.connectToServer()) {
        return 1;
    }
    
    client.run();
    
    return 0;
}