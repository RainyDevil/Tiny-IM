#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <mutex>
#include <queue>
#include <memory>
#include <condition_variable>
#include <vector>
#include <unordered_map>
#include <optional>
#include "Message.h"
//  CREATE TABLE users (
//     user_id TEXT PRIMARY KEY,  -- 使用 UUID 作为主键
//     username TEXT,             -- 用户名可以为空或重复
//     password_hash TEXT NOT NULL,  -- 存储哈希后的密码
//     salt TEXT NOT NULL,           -- 每个用户不同的盐值
//     status INTEGER DEFAULT 0      -- 0: offline, 1: online
// );

// CREATE TABLE friends (
//     user_id TEXT,
//     friend_id TEXT,
//     status INTEGER DEFAULT 0, -- 0: pending, 1: accepted
//     PRIMARY KEY (user_id, friend_id),
//     FOREIGN KEY (user_id) REFERENCES users(user_id),
//     FOREIGN KEY (friend_id) REFERENCES users(user_id)
// );

// CREATE TABLE messages (
//     message_id INTEGER PRIMARY KEY AUTOINCREMENT,
//     from_user_id TEXT,
//     to_user_id TEXT,
//     message_type TEXT,
//     content TEXT,
//     timestamp INTEGER,
//     FOREIGN KEY (from_user_id) REFERENCES users(user_id),
//     FOREIGN KEY (to_user_id) REFERENCES users(user_id)
// );

// CREATE TABLE offline_messages (
//     message_id INTEGER PRIMARY KEY AUTOINCREMENT,
//     user_id TEXT,
//     content TEXT,
//     timestamp INTEGER,
//     FOREIGN KEY (user_id) REFERENCES users(user_id)
// );
class DatabaseConnection {
public:
    DatabaseConnection(const std::string& db_file);
    ~DatabaseConnection();
    sqlite3* getConnection();

private:
    sqlite3* db_;
};

class Database {
public:
    static Database& getInstance();
    void init(const std::string& db_file, int pool_size);
    std::shared_ptr<DatabaseConnection> getConnection();

    // User operations
    std::optional<std::string> registerUser(const std::string& password, const std::string& username);
    bool setUsername(const std::string& user_id, const std::string& username);
    std::optional<std::string> authenticateUser(const std::string& username, const std::string& password);
    std::optional<std::string> getUserNameById(const std::string& user_id);

    // Friend operations
    bool addFriend(const std::string& user_id, const std::string& friend_id);
    bool removeFriend(const std::string& user_id, const std::string& friend_id);
    bool ackAddFriend(const std::string& user_id, const std::string& friend_id);
    std::vector<std::string> getFriendList(const std::string& user_id);
    std::vector<std::string> getFriendListUsername(const std::string& user_id);
    std::vector<std::pair<std::string, std::string>> getFriendPair(const std::string& user_id);

    // Message operations
    bool storeMessage(const std::string& from_user_id, const std::string& to_user_id, const std::string& message_type, const std::string& content);
    std::vector<Message> getRecentMessages(const std::string& user_id, int days);
    bool storeOfflineMessage(const std::string& user_id, const std::string& content);
    std::vector<std::string> getOfflineMessages(const std::string& user_id);
    bool markMessagesAsRead(const std::string& user_id);

private:
    Database() = default;
    ~Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    std::queue<std::shared_ptr<DatabaseConnection>> connection_pool_;
    std::mutex mutex_;
    std::condition_variable cond_var_;

    std::string generateUUID();
    std::string hashPassword(const std::string& password, const std::string& salt);
    std::string generateSalt();
};

#endif // DATABASE_H
