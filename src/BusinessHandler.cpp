#include "BusinessHandler.h"
#include "Session.h"

void BusinessHandler::handleIncomingMessage(const std::shared_ptr<Session>& session, const Message& msg) {    
    int from_userId = msg.getFromUserId();
    int to_userId = msg.getToUserId();
    switch (msg.getMessageType()) {
        case Message::MessageType::SIGN_UP:
            signUp(msg, session);
            break;
        case Message::MessageType::LOGIN:
            loginUser(msg, session);
            break;
        case Message::MessageType::LOGOUT:
            logoutUser(from_userId, session);
            break;
        case Message::MessageType::TEXT:{
            sendMessageToUser(msg);
            break;
        }
        case Message::MessageType::ADD_FRIEND:
            addFriend(msg); 
            break;
        case Message::MessageType::FRIEND_LIST:{
            sendFriendList(msg, session);
            break;
        }
        case Message::MessageType::FRIEND_REQUEST_RESPONSE:
            ackAddFriend(msg);
            break;   
        case Message::MessageType::PRIVATE_CHAT:
            sendMessageToUser(msg);
            break;
        case Message::MessageType::GROUP_CHAT:
            sendGroupMessage(from_userId, to_userId, msg.getContent(), session);
            break;
        case Message::MessageType::PULL_MESSAGE:
            pushMessage(msg, session);
            break;
        default:
           LOG_ERROR("Unknown message type received");
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
            LOG_ERROR("User {} is already logged in." , userId);
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
            LOG_INFO("User {} logged in." , userId);
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
        LOG_INFO("User {} logged out.", userId);
    } else {
        session->close();
       LOG_ERROR("Logout failed. User {} not found." , userId);
    }
}

// 添加好友
void BusinessHandler::addFriend(const Message& msg) {
    int userId = msg.getFromUserId();
    int friendId = msg.getToUserId();
    if( userId == friendId){
        LOG_WARNING("User {} add himself !", userId);
        return;}  
    Database& db = Database::getInstance();
    if(!db.addFriend(std::to_string(userId), std::to_string(friendId))) {
        LOG_WARNING("Database excute [addFriend()] failed ! ");
        return ;
    };
    std::optional<std::string> username = db.getUserNameById(std::to_string(userId));
    if(username.has_value()){
        //如果在线
        auto it = userSessions_.find(friendId);
        if( it != userSessions_.end()){
            Message msg(userId, friendId, Message::MessageType::FRIEND_REQUEST, msg.getMessageId() + 1, username.value());
            it->second->send(msg);
            LOG_INFO("Sent ADD_FRIEND_REQUEST from User {} to User {}", userId, friendId);   
        }
        //TODO 在登陆时推送好友信息
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
            LOG_INFO("Database excute [removeFriend()] sucess ! ");
            LOG_INFO("{} reject {} friend request !", userId, friendId);
            return ;
        } else {
            LOG_WARNING("Database excute [removeFriend()] failed ! ");
        }
    } else if(content == "accept"){
        if(db.ackAddFriend(std::to_string(friendId), std::to_string(userId)))
        {
            LOG_INFO("Database excute [ackAddFriend()] sucess ! ");
            LOG_INFO("User {}  added as a friend for User {}", friendId, userId);
        } else {
            LOG_WARNING("Database excute [ackAddFriend()] failed ! ");
        }
    }
}
// 发送私信
void BusinessHandler::sendMessageToUser(const Message& msg) {
    int fromUserId = msg.getFromUserId();
    int toUserId = msg.getToUserId();
    Database& db = Database::getInstance();
    std::string content = msg.getContent();
    auto it = userSessions_.find(toUserId);
    if(db.storeMessage(std::to_string(fromUserId), 
       std::to_string(toUserId), 
       Message::messageTypeToString(Message::MessageType::TEXT), 
       content)){
       LOG_INFO("Database excute [storeMessage()] sucess ! ");
    } else {
       LOG_WARNING("Database excute [storeMessage()] fail ! ");
    }
    if (it != userSessions_.end()) {
        Message msg1(fromUserId, toUserId, Message::MessageType::TEXT, msg.getMessageId() + 1, content);
        it->second->send(msg1);
        LOG_INFO("Sent message from User {} to User {} ", fromUserId, toUserId);
    } else {
       LOG_WARNING("Failed to send message. User {} not found.",  toUserId);
    }
}

// 群聊消息
void BusinessHandler::sendGroupMessage(int fromUserId, int groupId, const std::string& content, const std::shared_ptr<Session>& session) {
    auto it = userSessions_.find(fromUserId);
    if(it == userSessions_.end()) {
        LOG_ERROR("User {} not login but send message !");
    } 
    for (auto it = userSessions_.begin(); it != userSessions_.end(); ) {
        if(it->second != session){
            Message msg(fromUserId, groupId,Message::MessageType::GROUP_CHAT, groupId, content);
            it->second->send(msg);
        }
        it++;
        LOG_INFO("Sent group message from User {}  to Group {} ", fromUserId, groupId);
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
void BusinessHandler::pushMessage(const Message& msg,const std::shared_ptr<Session>& session){
    Database& db = Database::getInstance();
    int fromUserId = msg.getFromUserId();
    auto messges = db.getRecentMessages(std::to_string(fromUserId), 1);//days = 1
    for(auto m : messges) {
        session->send(m);
    }
}