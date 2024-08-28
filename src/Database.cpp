#include "Database.h"
#include <iostream>
#include <sstream>
#include <random>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/rand.h>

DatabaseConnection::DatabaseConnection(const std::string& db_file) {
    if (sqlite3_open(db_file.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << std::endl;
        db_ = nullptr;
    }
}

DatabaseConnection::~DatabaseConnection() {
    if (db_) {
        sqlite3_close(db_);
    }
}

sqlite3* DatabaseConnection::getConnection() {
    return db_;
}

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

void Database::init(const std::string& db_file, int pool_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (int i = 0; i < pool_size; ++i) {
        auto conn = std::make_shared<DatabaseConnection>(db_file);
        connection_pool_.push(conn);
    }
}

std::shared_ptr<DatabaseConnection> Database::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.wait(lock, [this] { return !connection_pool_.empty(); });
    auto conn = connection_pool_.front();
    connection_pool_.pop();
    return conn;
}

std::optional<std::string> Database::registerUser(const std::string& password, const std::string& username) {
    auto conn = getConnection();
    std::string user_id = generateUUID();
    std::string salt = generateSalt();
    std::string password_hash = hashPassword(password, salt);

    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO users (user_id, password_hash, salt, username) VALUES (?, ?, ?, ?)";
    if (sqlite3_prepare_v2(conn->getConnection(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, username.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return success ? std::optional<std::string>{user_id} : std::nullopt;
}

bool Database::setUsername(const std::string& user_id, const std::string& username) {
    auto conn = getConnection();
    sqlite3_stmt* stmt;
    std::string query = "UPDATE users SET username = ? WHERE user_id = ?";
    if (sqlite3_prepare_v2(conn->getConnection(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return success;
}

std::optional<std::string> Database::authenticateUser(const std::string& uuid, const std::string& password) {
    auto conn = getConnection();
    sqlite3_stmt* stmt;
    std::string query = "SELECT username, password_hash, salt FROM users WHERE user_id = ?";
    if (sqlite3_prepare_v2(conn->getConnection(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, uuid.c_str(), -1, SQLITE_STATIC);

    std::optional<std::string> username;
    std::string stored_hash, salt;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        stored_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    }
    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    if (username && hashPassword(password, salt) == stored_hash) {
        return username;
    }
    return std::nullopt;
}

std::optional<std::string> Database::getUserNameById(const std::string& user_id) {
    auto conn = getConnection();
    sqlite3_stmt* stmt;
    std::string query = "SELECT username FROM users WHERE user_id = ?";
    if (sqlite3_prepare_v2(conn->getConnection(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);

    std::optional<std::string> username;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return username;
}
// 添加好友
bool Database::addFriend(const std::string& user_id, const std::string& friend_id) {
    auto conn = getConnection();
    sqlite3* db = conn->getConnection();
    //防止插入重复数据
    const char* sql = R"(
        INSERT INTO friends (user_id, friend_id, status)
        SELECT ?, ?, 0
        WHERE NOT EXISTS (
            SELECT 1 FROM friends WHERE user_id = ? AND friend_id = ? AND status = 1
        );
    )";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, friend_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, friend_id.c_str(), -1, SQLITE_STATIC);

    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return result;
}
bool Database::ackAddFriend(const std::string& user_id, const std::string& friend_id) {
    auto conn = getConnection();
    sqlite3* db = conn->getConnection();

    const char* sql = "UPDATE friends SET status = 1 WHERE user_id = ? AND friend_id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, friend_id.c_str(), -1, SQLITE_STATIC);

    bool result = (sqlite3_step(stmt) == SQLITE_DONE);

    sqlite3_finalize(stmt);
    
    connection_pool_.push(conn);
    cond_var_.notify_one();

    return result;
}
// 删除好友
bool Database::removeFriend(const std::string& user_id, const std::string& friend_id) {
    auto conn = getConnection();
    sqlite3* db = conn->getConnection();

    const char* sql = "DELETE FROM friends WHERE user_id = ? AND friend_id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, friend_id.c_str(), -1, SQLITE_STATIC);

    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return result;
}

// 获取好友列表username
std::vector<std::string> Database::getFriendListUsername(const std::string& user_id) {
    auto conn = getConnection();
    sqlite3* db = conn->getConnection();

     const char* sql = R"(
        SELECT u.username
        FROM friends f
        JOIN users u ON f.friend_id = u.user_id
        WHERE f.user_id = ? AND f.status = 1;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);

    std::vector<std::string> friendUsernames;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (username) {
            friendUsernames.push_back(username);
        }
    }
    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return friendUsernames;
}
//获取好友列表id
std::vector<std::string> Database::getFriendList(const std::string& user_id) {
    auto conn = getConnection();
    sqlite3* db = conn->getConnection();

    const char* sql = "SELECT friend_id FROM friends WHERE user_id = ? AND status = 1;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);

    std::vector<std::string> friends;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        friends.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return friends;
}
std::vector<std::pair<std::string, std::string>> Database::getFriendPair(const std::string& user_id) {
   auto friendId = getFriendList(user_id);
   auto friendUsername = getFriendListUsername(user_id);
   std::vector<std::pair<std::string, std::string>> friendPair;
   if(friendId.size() == friendUsername.size()) {
       for(int i = 0; i < friendId.size(); i++) {
           friendPair.emplace_back(friendId[i], friendUsername[i]);
       }
   }
   return friendPair;
}
// 存储消息
bool Database::storeMessage(const std::string& from_user_id, const std::string& to_user_id, const std::string& message_type, const std::string& content) {
    auto conn = getConnection();
    sqlite3* db = conn->getConnection();

    const char* sql = "INSERT INTO messages (from_user_id, to_user_id, message_type, content, timestamp) VALUES (?, ?, ?, ?, datetime('now'));";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, from_user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, to_user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, message_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, content.c_str(), -1, SQLITE_STATIC);

    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return result;
}

// 获取最近消息
std::vector<std::string> Database::getRecentMessages(const std::string& user_id, int days) {
    auto conn = getConnection();
    sqlite3* db = conn->getConnection();

    const char* sql = "SELECT content FROM messages WHERE (from_user_id = ? OR to_user_id = ?) AND timestamp >= datetime('now', ? || ' days');";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }

    std::string day_string = "-" + std::to_string(days);
    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, day_string.c_str(), -1, SQLITE_STATIC);

    std::vector<std::string> messages;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        messages.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return messages;
}

// 存储离线消息
bool Database::storeOfflineMessage(const std::string& user_id, const std::string& content) {
    auto conn = getConnection();
    sqlite3* db = conn->getConnection();

    const char* sql = "INSERT INTO offline_messages (user_id, content) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, content.c_str(), -1, SQLITE_STATIC);

    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return result;
}

// 获取离线消息
std::vector<std::string> Database::getOfflineMessages(const std::string& user_id) {
    auto conn = getConnection();
    sqlite3* db = conn->getConnection();

    const char* sql = "SELECT content FROM offline_messages WHERE user_id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);

    std::vector<std::string> messages;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        messages.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return messages;
}

// 标记消息为已读
bool Database::markMessagesAsRead(const std::string& user_id) {
    auto conn = getConnection();
    sqlite3* db = conn->getConnection();

    const char* sql = "DELETE FROM offline_messages WHERE user_id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_STATIC);

    bool result = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    connection_pool_.push(conn);
    cond_var_.notify_one();
    return result;
}
// Helper functions 9位uuid
std::string Database::generateUUID() {
    const std::string chars = "0123456789";
    std::random_device rd;  // 使用随机设备
    std::mt19937 gen(rd()); // 梅森旋转算法
    std::uniform_int_distribution<> dist(0, chars.size() - 1);

    std::string result;
    for (int i = 0; i < 9; ++i) {
        result += chars[dist(gen)];
    }
    return result;
}

std::string Database::generateSalt() {
    unsigned char salt[16];
    RAND_bytes(salt, sizeof(salt));
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)salt[i];
    }
    return ss.str();
}

std::string Database::hashPassword(const std::string& password, const std::string& salt) {
    std::string salted_password = password + salt;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(salted_password.c_str()), salted_password.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
