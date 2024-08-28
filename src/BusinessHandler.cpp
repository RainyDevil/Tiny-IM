#include "BusinessHandler.h"
#include "Session.h"

void BusinessHandler::handleIncomingMessage(const std::shared_ptr<Session>& session, const Message& msg) {    
    int from_userId = msg.getFromUserId();
    int to_userId = msg.getToUserId();
    int messageId = msg.getMessageId();
    switch (msg.getMessageType()) {
        case Message::MessageType::SIGN_UP:
            signUp(msg, session);
            std::cout << Utils::unixTimestampToDate(msg.getTimestamp()) << std::endl;
            break;
        case Message::MessageType::LOGIN:
            loginUser(msg, session);
            std::cout << Utils::unixTimestampToDate(msg.getTimestamp()) << std::endl;
            break;
        case Message::MessageType::LOGOUT:
            logoutUser(from_userId, session);
            break;
        case Message::MessageType::TEXT:{
            /// TODO 
            // 0 代表公共聊天室
            if(to_userId == 0)
            {
                sendGroupMessage(from_userId, to_userId, msg.getContent(), session);
            } else{
                sendMessageToUser(from_userId, to_userId, msg.getContent());
            }
            break;
        }
        case Message::MessageType::ADD_FRIEND:
            addFriend(msg); // messageId 被用作 friendId
            break;
        case Message::MessageType::FRIEND_LIST:{
            sendFriendList(msg, session);
            break;
        }
        case Message::MessageType::FRIEND_REQUEST_RESPONSE:
            ackAddFriend(msg);
            break;   
        case Message::MessageType::PRIVATE_CHAT:
            sendMessageToUser(from_userId, to_userId, msg.getContent());
            break;
        case Message::MessageType::GROUP_CHAT:
            sendGroupMessage(from_userId, to_userId, msg.getContent(), session);
            break;
        default:
            std::cerr << "Unknown message type received" << std::endl;
    }
}
//sign up
void BusinessHandler::signUp(const Message& msg,const std::shared_ptr<Session>& session) {
    nlohmann::json j = nlohmann::json::parse(msg.getContent());
    std::string password = j["password"];
    std::string username = j["username"];
    Database& db = Database::getInstance();
    std::optional<std::string> uuid = db.registerUser(password, username);
    if(uuid.has_value()){
        Message msg(-1, -1, Message::MessageType::SIGN_UP_RESPONSE, msg.getMessageId() + 1, uuid.value());
        session->send(msg);
    } else {
        return;
    }
}
// login
void BusinessHandler::loginUser(const Message& msg, const std::shared_ptr<Session>& session) {
    Database& db = Database::getInstance();
    int userId = msg.getFromUserId();
    std::optional<std::string> username = db.authenticateUser(std::to_string(userId), msg.getContent());//content 只有password，非JSON格式
    //只有一人可以登号
    if(username.has_value()){
        if (userSessions_.find(userId) != userSessions_.end()) {
            std::cerr << "User " << userId << " is already logged in." << std::endl;
            nlohmann::json j;
            j["status"] = "fail";
            j["username"] = " ";
            j["reason"] = "repeated login";
            Message msg(userId, 0,Message::MessageType::LOGIN_RESPONSE,  msg.getMessageId() + 1, j.dump());
            session->send(msg);
            return;
        } else {
            userSessions_[userId] = session;
            nlohmann::json j;
            j["status"] = "success";
            j["username"] = username.value();
            Message msg(userId, 0,Message::MessageType::LOGIN_RESPONSE, msg.getMessageId() + 1, j.dump());
            session->send(msg);
            std::cout << "User " << userId << " logged in." << std::endl;
        }
    }
    else{
        nlohmann::json j;
        j["status"] = "fail";
        j["username"] = " ";
        j["reason"] = "authenticate fail";
        Message msg(userId, 0,Message::MessageType::LOGIN_RESPONSE,  msg.getMessageId() + 1, j.dump());
        session->send(msg);
    }

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
void BusinessHandler::addFriend(const Message& msg) {
    int userId = msg.getFromUserId();
    int friendId = msg.getToUserId();
    Database& db = Database::getInstance();
    if(!db.addFriend(std::to_string(userId), std::to_string(friendId))) {
        std::cout << "Database excute [addFriend()] failed ! " << std::endl;
    };
    std::optional<std::string> username = db.getUserNameById(std::to_string(userId));
    if(username.has_value()){
        //如果在线
        auto it = userSessions_.find(friendId);
        if( it != userSessions_.end()){
            Message msg(userId, friendId, Message::MessageType::FRIEND_REQUEST, msg.getMessageId() + 1, username.value());
            it->second->send(msg);
            std::cout << "Sent ADD_FRIEND_REQUEST from User " << userId << " to User " << friendId << std::endl;   
        }
        //在登陆时推送好友信息
    }
}
void BusinessHandler::ackAddFriend(const Message& msg) {
    //在线
    //这是来自被加好友一方的确认，注意这里toUserId是好友添加的发起方
    int userId = msg.getFromUserId();
    int friendId = msg.getToUserId();
    std::string content = msg.getContent();
    Database& db = Database::getInstance();
    if(content == "reject") {
        if(db.removeFriend(std::to_string(friendId), std::to_string(userId))){
            std::cout << "Database excute [removeFriend()] sucess ! " << std::endl;
            std::cout << userId << "reject" << friendId << "friend request !" << std::endl;
            return ;
        } else {
            std::cout << "Database excute [removeFriend()] failed ! " << std::endl;
        }
    } else if(content == "accept"){
        if(db.ackAddFriend(std::to_string(friendId), std::to_string(userId)))
        {
            std::cout << "Database excute [ackAddFriend()] sucess ! " << std::endl;
            std::cout << "User " << friendId << " added as a friend for User " << userId << "." << std::endl;
        } else {
            std::cout << "Database excute [ackAddFriend()] failed ! " << std::endl;
        }
    }
    //同时更新fromUserId 和 toUserId 的好友列表
    // if (userFriends_.find(toUserId) == userFriends_.end()) {
    //     userFriends_[toUserId] = std::vector<int>();
    // }
    // if (userFriends_.find(fromUserId) == userFriends_.end()) {
    //     userFriends_[fromUserId] = std::vector<int>();
    // }
    // auto& friendsListTo = userFriends_[toUserId];
    // if (std::find(friendsListTo.begin(), friendsListTo.end(), fromUserId) == friendsListTo.end()) {
    //     friendsListTo.push_back(fromUserId);
    //     std::cout << "User " << toUserId << " added as a friend for User " << fromUserId << "." << std::endl;
    // } else {
    //     std::cerr << "User " << toUserId << " is already a friend of User " << fromUserId << "." << std::endl;
    // }
    // auto& friendsListFrom = userFriends_[fromUserId];
    // if (std::find(friendsListFrom.begin(), friendsListFrom.end(), fromUserId) == friendsListFrom.end()) {
    //     friendsListFrom.push_back(toUserId);
    //     std::cout << "User " << fromUserId << " added as a friend for User " << toUserId << "." << std::endl;
    // } else {
    //     std::cerr << "User " << fromUserId << " is already a friend of User " << toUserId << "." << std::endl;
    // }
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
void BusinessHandler::sendGroupMessage(int fromUserId, int groupId, const std::string& content, const std::shared_ptr<Session>& session) {
    for (auto it = userSessions_.begin(); it != userSessions_.end(); ) {
        if(it->second != session){
            Message msg(fromUserId, groupId,Message::MessageType::TEXT, groupId, content);
            it->second->send(msg);
        }
        it++;
        std::cout << "Sent group message from User " << fromUserId << " to Group " << groupId << ": " << content << std::endl;
    }
}
void BusinessHandler::handleDisconnection(const std::shared_ptr<Session>& session) {
    for (auto it = userSessions_.begin(); it != userSessions_.end(); ) {
        if (it->second == session) {
            it = userSessions_.erase(it);
        } else {
            ++it;
        }
    }
}
void BusinessHandler::sendFriendList(const Message& msg, const std::shared_ptr<Session>& session) {
    Database& db = Database::getInstance();
    int from_userId = msg.getFromUserId();
    int to_userId = msg.getToUserId();
    std::vector<std::pair<std::string, std::string>> friendPair = db.getFriendPair(std::to_string(from_userId));
    nlohmann::json friendsList = nlohmann::json::array(); // 创建一个 JSON 数组
    for(auto & f : friendPair)
    {
        nlohmann::json friendInfo = {
            {"id", std::stoi(f.first)},
            {"name", f.second},
            {"avatar", "./default.png"} 
        };
        friendsList.push_back(friendInfo);
    }
    Message response(from_userId, to_userId, Message::MessageType::FRIEND_LIST, msg.getMessageId() + 1, friendsList.dump());
    session->send(response);
}