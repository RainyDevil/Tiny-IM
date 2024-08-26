#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <ctime>
#include <nlohmann/json.hpp>

class Message {
public:
    enum class MessageType {
        LOGIN,
        LOGIN_RESPONSE,
        LOGOUT,
        TEXT,
        ADD_FRIEND,              //  user1  ---ADD_FRIEND-------------->  服务器
        FRIEND_REQUEST,          //  服务器  ---FRIEND_REQUEST---------->  user2
        FRIEND_REQUEST_RESPONSE, //  user2  ---FRIEND_REQUEST_RESPONSE--> 服务器
        FRIEND_LIST,              
        PRIVATE_CHAT,
        GROUP_CHAT,
        // ...
    };

    // Constructors
    Message() = default;
    Message(int fromUserId, int toUserId, MessageType type, int messageId, const std::string& content);

    // Getters
    int getFromUserId() const;
    int getToUserId() const;
    MessageType getMessageType() const;
    int getMessageId() const;
    const std::string& getContent() const;
    std::time_t getTimestamp() const;

    // Setters
    void setFromUserId(int from_userId);
    void setToUserId(int to_userId);
    void setMessageType(MessageType type);
    void setMessageId(int messageId);
    void setContent(const std::string& content);
    void setTimestamp(std::time_t timestamp);

    // JSON Serialization/Deserialization
    std::string toJson() const;
    static Message fromJson(const std::string& jsonString);

private:
    int fromUserId_;
    int toUserId_;
    MessageType messageType_;
    int messageId_;
    std::string content_;
    std::time_t timestamp_;

    // Helper function for MessageType conversion
    static MessageType stringToMessageType(const std::string& typeStr);
    static std::string messageTypeToString(MessageType type);
};

#endif // MESSAGE_H
