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
            // 0 ������������
            if(to_userId == 0)
            {
                sendGroupMessage(from_userId, to_userId, j.dump(), session);
            } else{
                sendMessageToUser(from_userId, to_userId, j.dump());
            }
            break;
        }
        case Message::MessageType::ADD_FRIEND:
            addFriend(from_userId, to_userId); // messageId ������ friendId
            break;
        case Message::MessageType::FRIEND_LIST:{
            //auto friends = getFriendList(from_userId);
            //�������б��ظ��ͻ���
            nlohmann::json friendsList = nlohmann::json::array(); // ����һ�� JSON ����
            auto friends = getFriendList(from_userId);
            if(friends.size() != 0){ 
                for(auto & f : friends)
                {
                    // TODO �����nameӦ�ô����ݿ����
                    nlohmann::json friendInfo = {
                        {"id", f},
                        {"name", std::to_string(f)}, // ���������Ӧ�ô����ݿ��ѯ�õ�
                        {"avatar", "./default.png"} // ע�� spelling �� avator ��Ϊ avatar
                    };
                    friendsList.push_back(friendInfo);
                }
            }
            // for test 
            friendsList.push_back({
                {"id", 131255},
                {"name", "Bob"},
                {"avatar", "./bob_avatar.png"}
            });
            friendsList.push_back({
                {"id", 131256},
                {"name", "KK"},
                {"avatar", "./KK.png"}
            });
            Message response(from_userId, to_userId, Message::MessageType::FRIEND_LIST, 0, friendsList.dump());
            session->send(response);
            break;
        }
        case Message::MessageType::FRIEND_REQUEST_RESPONSE:
            ackAddFriend(from_userId, to_userId, msg.getContent());
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

// ��Ӻ���
void BusinessHandler::addFriend(int userId, int friendId) {

    //����
    if (userFriends_.find(userId) == userFriends_.end()) {
        userFriends_[userId] = std::vector<int>();
    }
    auto& friendsList = userFriends_[userId];
    if (std::find(friendsList.begin(), friendsList.end(), friendId) == friendsList.end()) {
        auto it = userSessions_.find(friendId);
        if (it != userSessions_.end()) {
            Message msg(userId, friendId, Message::MessageType::FRIEND_REQUEST, 1, std::to_string(userId));// TODO �����ݿ��name
            it->second->send(msg);
            std::cout << "Sent ADD_FRIEND_REQUEST from User " << userId << " to User " << friendId << std::endl;
        } 
        else {
            std::cerr << friendId << " is not online." << std::endl;
        }
    } 
    else {
        std::cerr << "User " << userId << " is already a friend of User " << friendId << "." << std::endl;
    }

    /// TODO �����ݿ������


}
void BusinessHandler::ackAddFriend(int fromUserId, int toUserId, const std::string& content) {
    //����
    //�������Ա��Ӻ���һ����ȷ�ϣ�ע������toUserId�Ǻ�����ӵķ���
    if(content == "reject") return ;
    //ͬʱ����fromUserId �� toUserId �ĺ����б�
    if (userFriends_.find(toUserId) == userFriends_.end()) {
        userFriends_[toUserId] = std::vector<int>();
    }
    if (userFriends_.find(fromUserId) == userFriends_.end()) {
        userFriends_[fromUserId] = std::vector<int>();
    }
    auto& friendsListTo = userFriends_[toUserId];
    if (std::find(friendsListTo.begin(), friendsListTo.end(), fromUserId) == friendsListTo.end()) {
        friendsListTo.push_back(fromUserId);
        std::cout << "User " << toUserId << " added as a friend for User " << fromUserId << "." << std::endl;
    } else {
        std::cerr << "User " << toUserId << " is already a friend of User " << fromUserId << "." << std::endl;
    }
    auto& friendsListFrom = userFriends_[fromUserId];
    if (std::find(friendsListFrom.begin(), friendsListFrom.end(), fromUserId) == friendsListFrom.end()) {
        friendsListFrom.push_back(toUserId);
        std::cout << "User " << fromUserId << " added as a friend for User " << toUserId << "." << std::endl;
    } else {
        std::cerr << "User " << fromUserId << " is already a friend of User " << toUserId << "." << std::endl;
    }

    //���ݿ����
}
// ����˽��
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

// Ⱥ����Ϣ
void BusinessHandler::sendGroupMessage(int fromUserId, int groupId, const std::string& content, const std::shared_ptr<Session>& session) {
    for (auto it = userSessions_.begin(); it != userSessions_.end(); ) {
        if(it->second != session){
            Message msg(fromUserId, groupId,Message::MessageType::TEXT, groupId, content);
            it->second->send(msg);
        }
        it++;
        std::cout << "Sent group message from User " << fromUserId << " to Group " << groupId << ": " << content << std::endl;
    }
    // auto it = groupChats_.find(groupId);
    // if (it != groupChats_.end()) {
    //     for (int memberId : it->second) {
    //         sendMessageToUser(fromUserId, memberId, content);
    //     }
    //     std::cout << "Sent group message from User " << fromUserId << " to Group " << groupId << ": " << content << std::endl;
    // } else {
    //     std::cerr << "Group " << groupId << " not found." << std::endl;
    // }
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

// ��ȡ�����б�
std::vector<int> BusinessHandler::getFriendList(int userId) {
    if (userFriends_.find(userId) != userFriends_.end()) {
        return userFriends_[userId];
    }
    return std::vector<int>();
}
