#pragma once
#include "Message.h"
#include "Utils.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>
class Session; // Forward declaration

class BusinessHandler {
public:
    void handleIncomingMessage(const std::shared_ptr<Session>& session, const Message& msg);
    void handleDisconnection(const std::shared_ptr<Session>& session);
private:
    void loginUser(int userId, const std::shared_ptr<Session>& session);
    void logoutUser(int userId,const std::shared_ptr<Session>& session);
    void addFriend(int userId, int friendId);
    void sendMessageToUser(int fromUserId, int toUserId, const std::string& content);
    void sendGroupMessage(int fromUserId, int groupId, const std::string& content);
    std::vector<int> getFriendList(int userId);

private:
    std::unordered_map<int, std::shared_ptr<Session>> userSessions_; // 管理用户的会话
    std::unordered_map<int, std::vector<int>> userFriends_; // 用户ID到好友列表的映射
    std::unordered_map<int, std::vector<int>> groupChats_; // 群聊ID到用户ID列表的映射
};
