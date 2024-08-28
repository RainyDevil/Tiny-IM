#pragma once
#include "Message.h"
#include "Utils.h"
#include "Database.h"
#include "Config.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
class Session; // Forward declaration

class BusinessHandler {
public: 
    BusinessHandler(){
        Database& db = Database::getInstance();
        Config& config = Config::getInstance();
        db.init(config.getDbName(), config.getConnectionSize());
    }
public:
    void handleIncomingMessage(const std::shared_ptr<Session>& session, const Message& msg);
    void handleDisconnection(const std::shared_ptr<Session>& session);
private:
    void signUp(const Message& msg,const std::shared_ptr<Session>& session);
    void loginUser(const Message& msg, const std::shared_ptr<Session>& session);
    void logoutUser(int userId,const std::shared_ptr<Session>& session);
    void addFriend(const Message& msg);
    void sendMessageToUser(int fromUserId, int toUserId, const std::string& content);
    void sendGroupMessage(int fromUserId, int groupId, const std::string& content, const std::shared_ptr<Session>& session);
    void ackAddFriend(const Message& msg);
    void sendFriendList(const Message& msg, const std::shared_ptr<Session>& session);

private:
    std::unordered_map<int, std::shared_ptr<Session>> userSessions_; // �����û��ĻỰ
    std::unordered_map<int, std::vector<int>> userFriends_; // �û�ID�������б��ӳ��
    std::unordered_map<int, std::vector<int>> groupChats_; // Ⱥ��ID���û�ID�б��ӳ��
};
