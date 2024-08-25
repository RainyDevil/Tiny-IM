#include "BusinessHandler.h"
#include "Session.h"

void BusinessHandler::handleIncomingMessage(const std::shared_ptr<Session>& session, const Message& msg) {
    int from_userId = msg.getFromUserId();
    int to_userId = msg.getToUserId();
    switch (msg.getMessageType()) {
        case Message::MessageType::LOGIN:
            loginUser(from_userId, session);
            std::cout << Utils::unixTimestampToDate(msg.getTimestamp()) << std::endl;
            break;
        case Message::MessageType::LOGOUT:
            logoutUser(from_userId, session);
            break;
        case Message::MessageType::TEXT:{
            nlohmann::json j;
            j["userName"] = std::to_string(from_userId);
            j["text"] = msg.getContent();
            sendMessageToUser(from_userId, to_userId, j.dump());
            break;
        }
        case Message::MessageType::ADD_FRIEND:
            addFriend(from_userId, to_userId); // messageId 被用作 friendId
            break;
        case Message::MessageType::FRIEND_LIST:{
            // auto friends = getFriendList(from_userId);
            // //将好友列表返回给客户端
            // Message response(from_userId, to_userId, Message::MessageType::FRIEND_LIST, 0, friends);
            // session->send(response);
            break;
        }   
        case Message::MessageType::PRIVATE_CHAT:
            sendMessageToUser(from_userId, to_userId, msg.getContent());
            break;
        case Message::MessageType::GROUP_CHAT:
            sendGroupMessage(from_userId, to_userId, msg.getContent());
            break;
        default:
            std::cerr << "Unknown message type received" << std::endl;
    }
}
// login
void BusinessHandler::loginUser(int userId, const std::shared_ptr<Session>& session) {
    if (userSessions_.find(userId) != userSessions_.end()) {
        std::cerr << "User " << userId << " is already logged in." << std::endl;
        Message msg(userId, 0,Message::MessageType::LOGIN_RESPONSE, 1, "notfind");
        session->send(msg);
        return;
    }
    userSessions_[userId] = session;
    Message msg(userId, 0,Message::MessageType::LOGIN_RESPONSE, 1, "success");
    session->send(msg);
    std::cout << "User " << userId << " logged in." << std::endl;
}

// logout
void BusinessHandler::logoutUser(int userId,const std::shared_ptr<Session>& session) {
    auto it = userSessions_.find(userId);
    if (it != userSessions_.end()) {
        userSessions_.erase(it);
        session->close();
        std::cout << "User " << userId << " logged out." << std::endl;
    } else {
        session->close();
        std::cerr << "Logout failed. User " << userId << " not found." << std::endl;
    }
}

// 添加好友
void BusinessHandler::addFriend(int userId, int friendId) {
    if (userFriends_.find(userId) == userFriends_.end()) {
        userFriends_[userId] = std::vector<int>();
    }
    auto& friendsList = userFriends_[userId];
    if (std::find(friendsList.begin(), friendsList.end(), friendId) == friendsList.end()) {
        friendsList.push_back(friendId);
        std::cout << "User " << friendId << " added as a friend for User " << userId << "." << std::endl;
    } else {
        std::cerr << "User " << friendId << " is already a friend of User " << userId << "." << std::endl;
    }
}

// 发送私信
void BusinessHandler::sendMessageToUser(int fromUserId, int toUserId, const std::string& content) {
    auto it = userSessions_.find(toUserId);
    if (it != userSessions_.end()) {
        Message msg(fromUserId, toUserId,Message::MessageType::TEXT, toUserId, content);
        it->second->send(msg);
        std::cout << "Sent message from User " << fromUserId << " to User " << toUserId << ": " << content << std::endl;
    } else {
        std::cerr << "Failed to send message. User " << toUserId << " not found." << std::endl;
    }
}

// 群聊消息
void BusinessHandler::sendGroupMessage(int fromUserId, int groupId, const std::string& content) {
    auto it = groupChats_.find(groupId);
    if (it != groupChats_.end()) {
        for (int memberId : it->second) {
            sendMessageToUser(fromUserId, memberId, content);
        }
        std::cout << "Sent group message from User " << fromUserId << " to Group " << groupId << ": " << content << std::endl;
    } else {
        std::cerr << "Group " << groupId << " not found." << std::endl;
    }
}
void BusinessHandler::handleDisconnection(const std::shared_ptr<Session>& session) {
    for (auto it = userSessions_.begin(); it != userSessions_.end(); ) {
        if (it->second == session) {
            it = userSessions_.erase(it);
        } else {
            // 否则，只移动迭代器
            ++it;
        }
    }
    for (const auto& pair : userSessions_) {
        std::cout << pair.first  << std::endl;
    }

}
// 获取好友列表
std::vector<int> BusinessHandler::getFriendList(int userId) {
    if (userFriends_.find(userId) != userFriends_.end()) {
        return userFriends_[userId];
    }
    return std::vector<int>();
}
