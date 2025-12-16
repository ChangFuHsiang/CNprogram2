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
#include "Crypto.h"

using namespace std;

class ChatClient
{
private:
    int clientSocket;
    string serverIP;
    int serverPort;
    int myListenPort;
    bool isLoggedIn;
    string currentUser;

    // Phase 2: P2P通訊支援
    unique_ptr<P2PClient> p2pClient;

    // Phase 2: 加密模組
    Crypto crypto;
    bool encryptionEnabled;
    bool serverSupportsEncryption;

public:
    ChatClient(string ip, int port)
        : serverIP(ip), serverPort(port), myListenPort(0), isLoggedIn(false),
          encryptionEnabled(true), serverSupportsEncryption(false)
    {

        cout << "=== Phase 2 Chat Client with Encryption ===" << endl;

        // 測試加密功能
        if (crypto.selfTest())
        {
            cout << "🔐 Client encryption enabled (AES-256-CBC)" << endl;
        }
        else
        {
            cerr << "⚠️ Encryption self-test failed, disabling encryption" << endl;
            encryptionEnabled = false;
        }
    }

    bool connectToServer()
    {
        // 建立socket
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0)
        {
            perror("Socket creation failed");
            return false;
        }

        // 設定server地址
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);

        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0)
        {
            perror("Invalid address");
            return false;
        }

        // 連接到server
        if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            perror("Connection failed");
            return false;
        }

        cout << "Connected to Phase 2 server " << serverIP << ":" << serverPort << endl;

        // 查詢服務器加密狀態
        if (encryptionEnabled)
        {
            checkServerEncryption();
        }

        return true;
    }

    // 檢查服務器是否支援加密
    void checkServerEncryption()
    {
        string response = sendCommandRaw("ENCRYPTION_STATUS");
        if (response.find("ENCRYPTION_STATUS:ENABLED") != string::npos)
        {
            serverSupportsEncryption = true;
            cout << "🔒 Server supports encryption - secure communication enabled" << endl;
        }
        else
        {
            serverSupportsEncryption = false;
            cout << "⚠️ Server does not support encryption - using plain text" << endl;
        }
    }

    // 發送原始命令（不加密）
    string sendCommandRaw(const string &command)
    {
        try
        {
            const char *data = command.c_str();
            size_t len = command.length();

            ssize_t sent = send(clientSocket, data, len, 0);
            if (sent < 0 || (size_t)sent != len)
            {
                return "ERROR: Send failed";
            }

            char buffer[4096];
            memset(buffer, 0, sizeof(buffer));

            ssize_t received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (received <= 0)
            {
                return "ERROR: Receive failed";
            }

            buffer[received] = '\0';
            return string(buffer);
        }
        catch (const exception &e)
        {
            return "ERROR: Communication error";
        }
    }

    string sendCommand(const string &command)
    {
        try
        {
            string messageToSend = command;
            bool useEncryption = encryptionEnabled && serverSupportsEncryption;

            // 加密命令（如果啟用）
            if (useEncryption)
            {
                string encrypted = crypto.encryptMessage(command);
                if (!encrypted.empty())
                {
                    messageToSend = encrypted;
                    cout << "🔒 Sending encrypted command" << endl;
                }
                else
                {
                    cout << "⚠️ Encryption failed, sending plain text" << endl;
                }
            }

            cout << "Sending: [" << command << "]" << endl;

            // 發送指令
            const char *data = messageToSend.c_str();
            size_t len = messageToSend.length();

            ssize_t sent = send(clientSocket, data, len, 0);
            if (sent < 0)
            {
                cout << "Send error: " << strerror(errno) << endl;
                return "ERROR: Send failed";
            }
            if (sent == 0)
            {
                cout << "Connection closed by server during send" << endl;
                return "ERROR: Connection closed";
            }
            if ((size_t)sent != len)
            {
                cout << "Partial send: " << sent << "/" << len << " bytes" << endl;
                return "ERROR: Partial send";
            }

            // 接收回應
            char buffer[4096];
            memset(buffer, 0, sizeof(buffer));

            ssize_t received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (received < 0)
            {
                cout << "Receive error: " << strerror(errno) << endl;
                return "ERROR: Receive failed";
            }
            if (received == 0)
            {
                cout << "Server closed connection" << endl;
                return "ERROR: Server closed connection";
            }

            // 確保字串以null結尾
            buffer[received] = '\0';
            string response(buffer);

            // 解密回應（如果是加密的）
            if (Crypto::isEncryptedMessage(response))
            {
                string decrypted = crypto.decryptMessage(response);
                if (!decrypted.empty())
                {
                    response = decrypted;
                    cout << "🔓 Received encrypted response (decrypted)" << endl;
                }
                else
                {
                    cout << "⚠️ Failed to decrypt response" << endl;
                }
            }

            cout << "Received: [" << response << "]" << endl;
            return response;
        }
        catch (const exception &e)
        {
            cout << "Communication exception: " << e.what() << endl;
            return "ERROR: Communication error";
        }
    }

    void displayMenu()
    {
        cout << "\n=== Phase 2 Chat Client (Encrypted) ===" << endl;
        if (!isLoggedIn)
        {
            cout << "Encryption: " << (encryptionEnabled ? "🔒 Enabled" : "🔓 Disabled") << endl;
            cout << "1. Register - REGISTER <username> <password>" << endl;
            cout << "2. Login - LOGIN <username> <password> <listen_port>" << endl;
            cout << "3. Toggle Encryption" << endl;
            cout << "4. Quit - QUIT" << endl;
        }
        else
        {
            cout << "Logged in as: " << currentUser << " (P2P port: " << myListenPort << ")" << endl;
            cout << "Encryption: " << (encryptionEnabled ? "🔒 Enabled" : "🔓 Disabled") << endl;
            cout << "P2P Encryption: " << (p2pClient && p2pClient->isEncryptionEnabled() ? "🔒 Enabled" : "🔓 Disabled") << endl;
            cout << "1. List online users - LIST" << endl;
            cout << "2. Send server message - MESSAGE <your_message>" << endl;
            cout << "3. Get user info (P2P) - GET_USER_INFO <username>" << endl;
            cout << "4. Send P2P message (encrypted) - P2P_CHAT <username> <message>" << endl;
            cout << "5. Toggle P2P Encryption" << endl;
            cout << "6. Logout - LOGOUT" << endl;
        }
        cout << "Enter command: ";
    }

    bool handleRegister()
    {
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

    bool handleLogin()
    {
        string username, password;
        int port;

        cout << "Enter username: ";
        cin >> username;
        cout << "Enter password: ";
        cin >> password;
        cout << "Enter your listening port (for P2P communication): ";
        cin >> port;

        if (port <= 1024 || port > 65535)
        {
            cout << "Error: Please use port number between 1025-65535" << endl;
            return false;
        }

        string command = "LOGIN " + username + " " + password + " " + to_string(port);
        string response = sendCommand(command);

        cout << "Server response: " << response << endl;

        if (response == "LOGIN_SUCCESS")
        {
            isLoggedIn = true;
            currentUser = username;
            myListenPort = port;

            // Phase 2: 啟動P2P監聽（支援加密）
            cout << "🚀 Starting P2P listener with encryption..." << endl;
            p2pClient.reset(new P2PClient(port, username));

            // 設定P2P加密狀態
            p2pClient->setEncryption(encryptionEnabled);

            if (p2pClient->startP2PListener())
            {
                cout << "✅ P2P system ready! Encrypted direct messages enabled" << endl;
            }
            else
            {
                cout << "⚠️ P2P listener failed to start, but server login successful" << endl;
            }

            return true;
        }
        return false;
    }

    bool handleLogout()
    {
        string command = "LOGOUT";
        string response = sendCommand(command);

        cout << "Server response: " << response << endl;

        if (response == "LOGOUT_SUCCESS")
        {
            // Phase 2: 停止P2P監聽
            if (p2pClient)
            {
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

    void handleListUsers()
    {
        string command = "LIST";
        string response = sendCommand(command);

        cout << "Online users: " << response << endl;
    }

    void handleMessage()
    {
        cin.ignore(); // 清除緩衝區的換行符
        string message;
        cout << "Enter your message: ";
        getline(cin, message);

        string command = "MESSAGE " + message;
        string response = sendCommand(command);

        cout << "Server response: " << response << endl;
    }

    // Phase 2新增：發送加密P2P訊息
    void handleP2PChat()
    {
        string targetUser;
        cout << "Enter target username: ";
        cin >> targetUser;

        if (targetUser.empty())
        {
            cout << "Error: Target username cannot be empty" << endl;
            return;
        }

        if (targetUser == currentUser)
        {
            cout << "Error: Cannot send message to yourself" << endl;
            return;
        }

        // 首先從server獲取目標用戶的P2P資訊
        string command = "GET_USER_INFO " + targetUser;
        string response = sendCommand(command);

        if (response.find("USER_INFO:") != 0)
        {
            cout << "Error getting user info: " << response << endl;
            return;
        }

        // 解析用戶資訊：USER_INFO:IP:PORT
        string info = response.substr(10); // 去掉 "USER_INFO:" prefix
        size_t colonPos = info.find(':');
        if (colonPos == string::npos)
        {
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

        if (message.empty())
        {
            cout << "Error: Message cannot be empty" << endl;
            return;
        }

        // 發送加密P2P訊息
        if (p2pClient)
        {
            cout << "📤 Sending encrypted P2P message..." << endl;
            if (p2pClient->sendP2PMessage(targetIP, targetPort, message))
            {
                cout << "✅ Encrypted P2P message sent successfully to " << targetUser << endl;
            }
            else
            {
                cout << "❌ Failed to send P2P message to " << targetUser << endl;
                cout << "💡 Make sure the target user is online and reachable" << endl;
            }
        }
        else
        {
            cout << "Error: P2P system not initialized" << endl;
        }
    }

    // Phase 2新增：查詢用戶P2P資訊
    void handleGetUserInfo()
    {
        string targetUser;
        cout << "Enter username to get P2P info: ";
        cin >> targetUser;

        if (targetUser.empty())
        {
            cout << "Error: Username cannot be empty" << endl;
            return;
        }

        string command = "GET_USER_INFO " + targetUser;
        string response = sendCommand(command);

        if (response.find("USER_INFO:") == 0)
        {
            // 解析回應：USER_INFO:IP:PORT
            string info = response.substr(10); // 去掉 "USER_INFO:" prefix
            size_t colonPos = info.find(':');
            if (colonPos != string::npos)
            {
                string ip = info.substr(0, colonPos);
                string port = info.substr(colonPos + 1);

                cout << "✅ User " << targetUser << " P2P Info:" << endl;
                cout << "   IP: " << ip << endl;
                cout << "   Port: " << port << endl;
                cout << "   Status: Ready for encrypted P2P messaging" << endl;
            }
            else
            {
                cout << "Error: Invalid user info format" << endl;
            }
        }
        else
        {
            cout << "Server response: " << response << endl;
        }
    }

    // 切換Client-Server加密
    void toggleEncryption()
    {
        encryptionEnabled = !encryptionEnabled;
        cout << "🔐 Client-Server encryption " << (encryptionEnabled ? "enabled" : "disabled") << endl;

        if (encryptionEnabled && !serverSupportsEncryption)
        {
            checkServerEncryption();
        }
    }

    // 切換P2P加密
    void toggleP2PEncryption()
    {
        if (p2pClient)
        {
            bool newState = !p2pClient->isEncryptionEnabled();
            p2pClient->setEncryption(newState);
            cout << "🔐 P2P encryption " << (newState ? "enabled" : "disabled") << endl;
        }
        else
        {
            cout << "⚠️ P2P system not initialized (login first)" << endl;
        }
    }

    void run()
    {
        string input;

        while (true)
        {
            try
            {
                displayMenu();

                if (!(cin >> input))
                {
                    cout << "Input error occurred. Exiting..." << endl;
                    break;
                }

                if (!isLoggedIn)
                {
                    if (input == "REGISTER" || input == "1")
                    {
                        handleRegister();
                    }
                    else if (input == "LOGIN" || input == "2")
                    {
                        if (!handleLogin())
                        {
                            cout << "Login failed. ";
                        }
                    }
                    else if (input == "3")
                    {
                        toggleEncryption();
                    }
                    else if (input == "QUIT" || input == "4")
                    {
                        cout << "Goodbye!" << endl;
                        break;
                    }
                    else
                    {
                        cout << "Unknown command. Please try again." << endl;
                    }
                }
                else
                {
                    if (input == "LIST" || input == "1")
                    {
                        handleListUsers();
                    }
                    else if (input == "MESSAGE" || input == "2")
                    {
                        handleMessage();
                    }
                    else if (input == "GET_USER_INFO" || input == "3")
                    {
                        handleGetUserInfo();
                    }
                    else if (input == "P2P_CHAT" || input == "4")
                    {
                        handleP2PChat();
                    }
                    else if (input == "5")
                    {
                        toggleP2PEncryption();
                    }
                    else if (input == "LOGOUT" || input == "6")
                    {
                        if (handleLogout())
                        {
                            cout << "Logged out successfully." << endl;
                        }
                        else
                        {
                            cout << "Logout failed." << endl;
                        }
                    }
                    else
                    {
                        cout << "Unknown command. Please try again." << endl;
                    }
                }

                // 檢查是否有嚴重錯誤導致連接中斷
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    cout << "Connection lost. Please restart the client." << endl;
                    break;
                }
            }
            catch (const exception &e)
            {
                cout << "An error occurred: " << e.what() << endl;
                cout << "Please try again or restart the client." << endl;
            }
        }
    }

    ~ChatClient()
    {
        // Phase 2: 確保P2P系統正確關閉
        if (p2pClient)
        {
            cout << "🛑 Shutting down P2P system..." << endl;
            p2pClient.reset();
        }

        if (clientSocket >= 0)
        {
            close(clientSocket);
        }

        cout << "👋 Chat client shutdown complete" << endl;
    }
};

int main(int argc, char *argv[])
{
    string serverIP = "127.0.0.1"; // 預設localhost
    int serverPort = 8080;         // 預設port

    if (argc > 1)
    {
        serverIP = argv[1];
    }
    if (argc > 2)
    {
        serverPort = atoi(argv[2]);
    }

    cout << "=== Phase 2 Chat Client with OpenSSL Encryption ===" << endl;
    cout << "Features:" << endl;
    cout << "  ✅ Server-based user management" << endl;
    cout << "  ✅ Direct P2P messaging" << endl;
    cout << "  ✅ AES-256-CBC Encryption (Client-Server)" << endl;
    cout << "  ✅ AES-256-CBC Encryption (P2P)" << endl;
    cout << "  ✅ Real-time encrypted message receiving" << endl;
    cout << "Connecting to server: " << serverIP << ":" << serverPort << endl;

    ChatClient client(serverIP, serverPort);

    if (!client.connectToServer())
    {
        return 1;
    }

    client.run();

    return 0;
}