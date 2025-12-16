#include <iostream>
#include <string>
#include <cstring>
#include <cerrno>
#include <exception>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

class ChatClient {
private:
    int clientSocket;
    string serverIP;
    int serverPort;
    int myListenPort;
    bool isLoggedIn;
    string currentUser;
    
public:
    ChatClient(string ip, int port) : serverIP(ip), serverPort(port), myListenPort(0), isLoggedIn(false) {}
    
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
        
        cout << "Connected to server " << serverIP << ":" << serverPort << endl;
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
        cout << "\n=== Chat Client ===" << endl;
        if (!isLoggedIn) {
            cout << "1. Register - REGISTER <username> <password>" << endl;
            cout << "2. Login - LOGIN <username> <password> <listen_port>" << endl;
            cout << "3. Quit - QUIT" << endl;
        } else {
            cout << "Logged in as: " << currentUser << endl;
            cout << "1. List online users - LIST" << endl;
            cout << "2. Send message - MESSAGE <your_message>" << endl;
            cout << "3. Logout - LOGOUT" << endl;
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
        cout << "Enter your listening port (for future P2P communication): ";
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
            return true;
        }
        return false;
    }
    
    bool handleLogout() {
        string command = "LOGOUT";
        string response = sendCommand(command);
        
        cout << "Server response: " << response << endl;
        
        if (response == "LOGOUT_SUCCESS") {
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
                            // 登入失敗，檢查連接是否還正常
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
                    else if (input == "LOGOUT" || input == "3") {
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
        if (clientSocket >= 0) {
            close(clientSocket);
        }
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
    
    ChatClient client(serverIP, serverPort);
    
    if (!client.connectToServer()) {
        return 1;
    }
    
    client.run();
    
    return 0;
}