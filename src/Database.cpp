#include "Database.h"
#include <iostream>

Database::Database(const std::string& db_name) {
    if (sqlite3_open(db_name.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::executeQuery(const std::string& query) {
    char* errmsg;
    if (sqlite3_exec(db_, query.c_str(), 0, 0, &errmsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

bool Database::authenticateUser(const std::string& username, const std::string& password) {
    std::string query = "SELECT COUNT(*) FROM users WHERE username = '" + username + "' AND password = '" + password + "';";
    std::vector<std::vector<std::string>> results;
    if (executeSelectQuery(query, results)) {
        return results[0][0] == "1";
    }
    return false;
}

void Database::storeMessage(const std::string& sender, const std::string& receiver, const std::string& message) {
    std::string query = "INSERT INTO messages (sender, receiver, message) VALUES ('" + sender + "', '" + receiver + "', '" + message + "');";
    executeQuery(query);
}

std::vector<std::string> Database::retrieveMessages(const std::string& username) {
    std::string query = "SELECT message FROM messages WHERE receiver = '" + username + "';";
    std::vector<std::vector<std::string>> results;
    std::vector<std::string> messages;

    if (executeSelectQuery(query, results)) {
        for (const auto& row : results) {
            messages.push_back(row[0]);
        }
    }

    return messages;
}

void Database::storeFile(const std::string& sender, const std::string& receiver, const std::string& filename, const std::string& filepath) {
    std::string query = "INSERT INTO files (sender, receiver, filename, filepath) VALUES ('" + sender + "', '" + receiver + "', '" + filename + "', '" + filepath + "');";
    executeQuery(query);
}

bool Database::executeSelectQuery(const std::string& query, std::vector<std::vector<std::string>>& results) {
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to execute query: " << query << std::endl;
        return false;
    }

    int cols = sqlite3_column_count(stmt);
    while (true) {
        int ret = sqlite3_step(stmt);
        if (ret == SQLITE_ROW) {
            std::vector<std::string> row;
            for (int col = 0; col < cols; col++) {
                row.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, col)));
            }
            results.push_back(row);
        } else if (ret == SQLITE_DONE) {
            break;
        } else {
            std::cerr << "Failed to retrieve data from database" << std::endl;
            return false;
        }
    }
    sqlite3_finalize(stmt);
    return true;
}
