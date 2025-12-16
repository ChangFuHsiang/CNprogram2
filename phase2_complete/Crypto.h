#ifndef CRYPTO_H
#define CRYPTO_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

/**
 * Phase 2: Message Encryption with OpenSSL
 * 
 * 使用 AES-256-CBC 對稱加密
 * - 支援 P2P 訊息加密
 * - 支援 Client-Server 通訊加密
 * - 自動處理 IV (Initialization Vector)
 */

class Crypto {
private:
    // AES-256 需要 32 bytes key
    static const int KEY_SIZE = 32;
    // AES block size 是 16 bytes
    static const int IV_SIZE = 16;
    static const int BLOCK_SIZE = 16;
    
    // 預設金鑰 (實際應用中應該用更安全的金鑰交換)
    // 這是一個簡化版本，使用固定金鑰
    unsigned char key[KEY_SIZE];
    bool keyInitialized;
    
    // 錯誤處理
    static void handleErrors() {
        unsigned long errCode;
        while ((errCode = ERR_get_error())) {
            char* err = ERR_error_string(errCode, NULL);
            std::cerr << "OpenSSL Error: " << err << std::endl;
        }
    }
    
    // Base64 編碼表
    static const std::string base64_chars;
    
    // Base64 編碼
    static std::string base64_encode(const unsigned char* data, size_t len) {
        std::string ret;
        int i = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        
        while (len--) {
            char_array_3[i++] = *(data++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                
                for (i = 0; i < 4; i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }
        
        if (i) {
            for (int j = i; j < 3; j++)
                char_array_3[j] = '\0';
            
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            
            for (int j = 0; j < i + 1; j++)
                ret += base64_chars[char_array_4[j]];
            
            while (i++ < 3)
                ret += '=';
        }
        
        return ret;
    }
    
    // Base64 解碼
    static std::vector<unsigned char> base64_decode(const std::string& encoded_string) {
        size_t in_len = encoded_string.size();
        int i = 0;
        size_t in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> ret;
        
        while (in_len-- && encoded_string[in_] != '=' && is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = base64_chars.find(char_array_4[i]);
                
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                
                for (i = 0; i < 3; i++)
                    ret.push_back(char_array_3[i]);
                i = 0;
            }
        }
        
        if (i) {
            for (int j = i; j < 4; j++)
                char_array_4[j] = 0;
            
            for (int j = 0; j < 4; j++)
                char_array_4[j] = base64_chars.find(char_array_4[j]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            
            for (int j = 0; j < i - 1; j++)
                ret.push_back(char_array_3[j]);
        }
        
        return ret;
    }
    
    static inline bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }

public:
    Crypto() : keyInitialized(false) {
        // 初始化 OpenSSL
        OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();
        
        // 設定預設金鑰 (從固定字串派生)
        setDefaultKey();
    }
    
    // 設定預設金鑰
    void setDefaultKey() {
        // 使用固定字串作為金鑰基礎 (簡化版本)
        const char* keyStr = "Phase2ChatEncryptionKey2025!!!!";
        memcpy(key, keyStr, KEY_SIZE);
        keyInitialized = true;
        std::cout << "🔐 Crypto: Default encryption key initialized" << std::endl;
    }
    
    // 設定自定義金鑰
    bool setKey(const std::string& keyString) {
        if (keyString.length() < KEY_SIZE) {
            std::cerr << "Crypto: Key must be at least " << KEY_SIZE << " bytes" << std::endl;
            return false;
        }
        memcpy(key, keyString.c_str(), KEY_SIZE);
        keyInitialized = true;
        std::cout << "🔐 Crypto: Custom encryption key set" << std::endl;
        return true;
    }
    
    // 設定金鑰 (從 bytes)
    bool setKey(const unsigned char* keyData, size_t keyLen) {
        if (keyLen < KEY_SIZE) {
            std::cerr << "Crypto: Key must be at least " << KEY_SIZE << " bytes" << std::endl;
            return false;
        }
        memcpy(key, keyData, KEY_SIZE);
        keyInitialized = true;
        return true;
    }
    
    /**
     * 加密訊息
     * 
     * @param plaintext 明文
     * @return Base64編碼的密文 (包含 IV)，格式: IV:CIPHERTEXT
     */
    std::string encrypt(const std::string& plaintext) {
        if (!keyInitialized) {
            std::cerr << "Crypto: Key not initialized" << std::endl;
            return "";
        }
        
        try {
            // 生成隨機 IV
            unsigned char iv[IV_SIZE];
            if (RAND_bytes(iv, IV_SIZE) != 1) {
                handleErrors();
                return "";
            }
            
            // 準備加密
            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            if (!ctx) {
                handleErrors();
                return "";
            }
            
            // 初始化加密操作
            if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                handleErrors();
                return "";
            }
            
            // 準備輸出緩衝區
            std::vector<unsigned char> ciphertext(plaintext.length() + BLOCK_SIZE);
            int len = 0;
            int ciphertext_len = 0;
            
            // 加密
            if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                                  (unsigned char*)plaintext.c_str(), plaintext.length()) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                handleErrors();
                return "";
            }
            ciphertext_len = len;
            
            // 完成加密 (處理 padding)
            if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                handleErrors();
                return "";
            }
            ciphertext_len += len;
            
            EVP_CIPHER_CTX_free(ctx);
            
            // 組合 IV 和密文，然後 Base64 編碼
            std::string ivBase64 = base64_encode(iv, IV_SIZE);
            std::string ciphertextBase64 = base64_encode(ciphertext.data(), ciphertext_len);
            
            // 格式: IV:CIPHERTEXT
            return ivBase64 + ":" + ciphertextBase64;
            
        } catch (const std::exception& e) {
            std::cerr << "Crypto encrypt exception: " << e.what() << std::endl;
            return "";
        }
    }
    
    /**
     * 解密訊息
     * 
     * @param encryptedData Base64編碼的密文，格式: IV:CIPHERTEXT
     * @return 解密後的明文
     */
    std::string decrypt(const std::string& encryptedData) {
        if (!keyInitialized) {
            std::cerr << "Crypto: Key not initialized" << std::endl;
            return "";
        }
        
        try {
            // 分離 IV 和密文
            size_t colonPos = encryptedData.find(':');
            if (colonPos == std::string::npos) {
                std::cerr << "Crypto: Invalid encrypted data format" << std::endl;
                return "";
            }
            
            std::string ivBase64 = encryptedData.substr(0, colonPos);
            std::string ciphertextBase64 = encryptedData.substr(colonPos + 1);
            
            // Base64 解碼
            std::vector<unsigned char> iv = base64_decode(ivBase64);
            std::vector<unsigned char> ciphertext = base64_decode(ciphertextBase64);
            
            if (iv.size() != IV_SIZE) {
                std::cerr << "Crypto: Invalid IV size" << std::endl;
                return "";
            }
            
            // 準備解密
            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            if (!ctx) {
                handleErrors();
                return "";
            }
            
            // 初始化解密操作
            if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv.data()) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                handleErrors();
                return "";
            }
            
            // 準備輸出緩衝區
            std::vector<unsigned char> plaintext(ciphertext.size() + BLOCK_SIZE);
            int len = 0;
            int plaintext_len = 0;
            
            // 解密
            if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                                  ciphertext.data(), ciphertext.size()) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                handleErrors();
                return "";
            }
            plaintext_len = len;
            
            // 完成解密 (處理 padding)
            if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                handleErrors();
                return "";
            }
            plaintext_len += len;
            
            EVP_CIPHER_CTX_free(ctx);
            
            return std::string((char*)plaintext.data(), plaintext_len);
            
        } catch (const std::exception& e) {
            std::cerr << "Crypto decrypt exception: " << e.what() << std::endl;
            return "";
        }
    }
    
    /**
     * 檢查是否為加密訊息
     * 加密訊息格式: ENC:IV:CIPHERTEXT
     */
    static bool isEncryptedMessage(const std::string& message) {
        return message.find("ENC:") == 0;
    }
    
    /**
     * 包裝加密訊息 (添加 ENC: 前綴)
     */
    std::string encryptMessage(const std::string& plaintext) {
        std::string encrypted = encrypt(plaintext);
        if (encrypted.empty()) {
            return "";
        }
        return "ENC:" + encrypted;
    }
    
    /**
     * 解包並解密訊息
     */
    std::string decryptMessage(const std::string& encryptedMessage) {
        if (!isEncryptedMessage(encryptedMessage)) {
            // 不是加密訊息，返回原始內容
            return encryptedMessage;
        }
        // 去掉 "ENC:" 前綴
        std::string encryptedData = encryptedMessage.substr(4);
        return decrypt(encryptedData);
    }
    
    // 測試加密功能
    bool selfTest() {
        std::cout << "🧪 Running Crypto self-test..." << std::endl;
        
        std::string testMessage = "Hello, this is a test message for encryption!";
        
        // 測試加密
        std::string encrypted = encryptMessage(testMessage);
        if (encrypted.empty()) {
            std::cerr << "❌ Self-test failed: encryption returned empty" << std::endl;
            return false;
        }
        std::cout << "   Encrypted: " << encrypted.substr(0, 50) << "..." << std::endl;
        
        // 測試解密
        std::string decrypted = decryptMessage(encrypted);
        if (decrypted != testMessage) {
            std::cerr << "❌ Self-test failed: decrypted message doesn't match" << std::endl;
            std::cerr << "   Expected: " << testMessage << std::endl;
            std::cerr << "   Got: " << decrypted << std::endl;
            return false;
        }
        
        std::cout << "✅ Crypto self-test passed!" << std::endl;
        return true;
    }
    
    ~Crypto() {
        // 清理 OpenSSL
        EVP_cleanup();
        ERR_free_strings();
    }
};

// Base64 字元表
const std::string Crypto::base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

#endif // CRYPTO_H
