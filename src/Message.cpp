#include "Message.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Constructors
Message::Message(int from_userId, int to_userId,MessageType type, int messageId, const std::string& content)
    : fromUserId_(from_userId), toUserId_(to_userId),messageType_(type), messageId_(messageId), content_(content), timestamp_(std::time(nullptr)) {}

// Getters
int Message::getFromUserId() const {
    return fromUserId_;
}
int Message::getToUserId() const {
    return toUserId_;
}

Message::MessageType Message::getMessageType() const {
    return messageType_;
}

int Message::getMessageId() const {
    return messageId_;
}

const std::string& Message::getContent() const {
    return content_;
}

std::time_t Message::getTimestamp() const {
    return timestamp_;
}

// Setters
void Message::setFromUserId(int from_userId) {
    fromUserId_ = from_userId;
}

void Message::setToUserId(int to_userId){
    toUserId_ = to_userId;
}

void Message::setMessageType(MessageType type) {
    messageType_ = type;
}

void Message::setMessageId(int messageId) {
    messageId_ = messageId;
}

void Message::setContent(const std::string& content) {
    content_ = content;
}

void Message::setTimestamp(std::time_t timestamp) {
    timestamp_ = timestamp;
}

// JSON Serialization
std::string Message::toJson() const {
    json j;
    j["fromUserId"] = fromUserId_;
    j["toUserId"] = toUserId_;
    j["messageType"] = messageTypeToString(messageType_);
    j["messageId"] = messageId_;
    j["content"] = content_;
    j["timestamp"] = timestamp_;
    return j.dump();
}

// JSON Deserialization
Message Message::fromJson(const std::string& jsonString) {
    json j = json::parse(jsonString);
    Message msg;
    msg.setFromUserId(j["fromUserId"]);
    msg.setToUserId(j["toUserId"]);
    msg.setMessageType(stringToMessageType(j["messageType"]));
    msg.setMessageId(j["messageId"]);
    msg.setContent(j["content"]);
    msg.setTimestamp(j["timestamp"]);
    return msg;
}

// Helper function to convert string to MessageType
Message::MessageType Message::stringToMessageType(const std::string& typeStr) {
    if (typeStr == "LOGIN") return MessageType::LOGIN;
    if (typeStr == "LOGIN_RESPONSE") return MessageType::LOGIN_RESPONSE;
    if (typeStr == "LOGOUT") return MessageType::LOGOUT;
    if (typeStr == "TEXT") return MessageType::TEXT;
    if (typeStr == "ADD_FRIEND") return MessageType::ADD_FRIEND;
    if (typeStr == "FRIEND_LIST") return MessageType::FRIEND_LIST;
    if (typeStr == "PRIVATE_CHAT") return MessageType::PRIVATE_CHAT;
    if (typeStr == "GROUP_CHAT") return MessageType::GROUP_CHAT;
    if (typeStr == "FRIEND_REQUEST") return MessageType::FRIEND_REQUEST;
    if (typeStr == "FRIEND_REQUEST_RESPONSE") return MessageType::FRIEND_REQUEST_RESPONSE;
    // Add more mappings as needed
    throw std::invalid_argument("Unknown message type");
}

// Helper function to convert MessageType to string
std::string Message::messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::LOGIN: return "LOGIN";
        case MessageType::LOGIN_RESPONSE: return "LOGIN_RESPONSE";
        case MessageType::LOGOUT: return "LOGOUT";
        case MessageType::TEXT: return "TEXT";
        case MessageType::ADD_FRIEND: return "ADD_FRIEND";
        case MessageType::FRIEND_LIST: return "FRIEND_LIST";
        case MessageType::PRIVATE_CHAT: return "PRIVATE_CHAT";
        case MessageType::GROUP_CHAT: return "GROUP_CHAT";
        case MessageType::FRIEND_REQUEST: return "FRIEND_REQUEST";
        case MessageType::FRIEND_REQUEST_RESPONSE: return "FRIEND_REQUEST_RESPONSE";
        // Add more mappings as needed
        default: throw std::invalid_argument("Unknown message type");
    }
}
