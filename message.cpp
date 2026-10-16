#include <sstream>
#include "message.h"
#include "byte_tools.h"
#include <stdexcept>

Message Message::Parse(const std::string& messageString) {
    if (messageString.size() == 4) {
        return {MessageId::KeepAlive, 0, ""};
    }
    
    if (messageString.size() < 5) {
        throw std::runtime_error("Message is too short to be parsed");
    }

    size_t length = BytesToInt(messageString.substr(0, 4));
    
    MessageId id = static_cast<MessageId>(messageString[4]);
    
    std::string payload = messageString.substr(5);

    return {id, length, payload};
}

Message Message::Init(MessageId id, const std::string& payload) {
    size_t length = 1 + payload.size();
    return {id, length, payload};
}

std::string Message::ToString() const {
    if (id == MessageId::KeepAlive) {
        return std::string(4, '\0');
    }

    std::string result = IntToBytes(messageLength);
    result += static_cast<char>(id);
    result += payload;

    return result;
}