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

std::optional<std::string> Database::registerUser(const std::string& password) {
    auto conn = getConnection();
    int user_id = generateUUID();
    std::string salt = generateSalt();
    std::string password_hash = hashPassword(password, salt);

    sqlite3_stmt* stmt;
    std::string query = "INSERT INTO users (user_id, password_hash, salt) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(conn->getConnection(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    std::string user_id_string = std::to_string(user_id);
    sqlite3_bind_text(stmt, 1, user_id_string.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success ? std::optional<std::string>{user_id_string} : std::nullopt;
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
    return success;
}

std::optional<std::string> Database::authenticateUser(const std::string& username, const std::string& password) {
    auto conn = getConnection();
    sqlite3_stmt* stmt;
    std::string query = "SELECT user_id, password_hash, salt FROM users WHERE username = ?";
    if (sqlite3_prepare_v2(conn->getConnection(), query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    std::optional<std::string> user_id;
    std::string stored_hash, salt;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        stored_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    }
    sqlite3_finalize(stmt);

    if (user_id && hashPassword(password, salt) == stored_hash) {
        return user_id;
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

    return username;
}

// Helper functions
int Database::generateUUID() {
    std::random_device rd;  // 使用随机设备
    std::mt19937 gen(rd()); // 以随机设备为种子初始化随机数生成器
    std::uniform_int_distribution<int> dist(1000000000, 1410065407); // 10位整数的范围

    return dist(gen);
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
