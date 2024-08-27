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
    std::optional<std::string> registerUser(const std::string& password);
    bool setUsername(const std::string& user_id, const std::string& username);
    std::optional<std::string> authenticateUser(const std::string& username, const std::string& password);
    std::optional<std::string> getUserNameById(const std::string& user_id);

    // Friend operations
    bool addFriend(const std::string& user_id, const std::string& friend_id);
    bool removeFriend(const std::string& user_id, const std::string& friend_id);
    std::vector<std::string> getFriendList(const std::string& user_id);

    // Message operations
    bool storeMessage(const std::string& from_user_id, const std::string& to_user_id, const std::string& message_type, const std::string& content);
    std::vector<std::string> getRecentMessages(const std::string& user_id, int days);
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

    int generateUUID();
    std::string hashPassword(const std::string& password, const std::string& salt);
    std::string generateSalt();
};

#endif // DATABASE_H
