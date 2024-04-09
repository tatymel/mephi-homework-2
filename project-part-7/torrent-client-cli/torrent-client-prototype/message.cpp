#include "message.h"
#include "byte_tools.h"
#include <string_view>
#include <string>
#include <iostream>

Message Message::Parse(const std::string& messageString){
    Message result;
    result.id = static_cast<MessageId> (messageString[0]);
    result.payload =  std::string(&messageString[1], messageString.size() - 1);
    result.messageLength = messageString.size();
    return result;
}

Message Message::Init(MessageId id, const std::string& payload){
    Message result;
    result.id = id;
    result.payload = payload;
    result.messageLength = 1 + payload.size();
    return result;
}

std::string Message::ToString() const{
    return (IntToBytes(messageLength) + static_cast<char>(id) + payload);
}

