#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>

class Database {
public:
    Database(const std::string& db_name);
    ~Database();

    bool executeQuery(const std::string& query);
    bool authenticateUser(const std::string& username, const std::string& password);
    void storeMessage(const std::string& sender, const std::string& receiver, const std::string& message);
    std::vector<std::string> retrieveMessages(const std::string& username);
    void storeFile(const std::string& sender, const std::string& receiver, const std::string& filename, const std::string& filepath);

private:
    sqlite3* db_;
    bool executeSelectQuery(const std::string& query, std::vector<std::vector<std::string>>& results);
};

#endif // DATABASE_H
