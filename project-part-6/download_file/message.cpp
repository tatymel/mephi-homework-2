#include "message.h"
#include "byte_tools.h"
#include <string_view>
#include <string>
#include <iostream>

/*
 * Выделяем тип сообщения и длину и создаем объект типа Message.
 * Подразумевается, что здесь в качестве `messageString` будет приниматься строка, прочитанная из TCP-сокета
 */
Message Message::Parse(const std::string& messageString){
    std::cout << "we are in Message::Parse" << std::endl;
    Message result;
    result.id = static_cast<MessageId> (messageString[0]);
    result.payload =  std::string(&messageString[1], messageString.size() - 1);
    if(result.id == MessageId::Have){
        std::cout << "message - have and piece has downloaded: " << BytesToInt(result.payload) << std::endl;
    }
    result.messageLength = messageString.size();
    return result;
}

/*
 * Создаем сообщение с заданным типом и содержимым. Длина вычисляется автоматически
 */
Message Message::Init(MessageId id, const std::string& payload){
    std::cout << "we are in Message::Init" << std::endl;
    Message result;
    result.id = id;
    result.payload = payload;
    std::cout << "result.messageLength: " << payload.size() + 1 << std::endl;
    result.messageLength = 1 + payload.size();
    return result;
}

/*
 * Формируем строку с сообщением, которую можно будет послать пиру в соответствии с протоколом.
 * Получается строка вида "<1 + payload length><message id><payload>"
 * Секция с длиной сообщения занимает 4 байта и представляет собой целое число в формате big-endian
 * id сообщения занимает 1 байт и может принимать значения от 0 до 9 включительно
 */
std::string Message::ToString() const{
    std::cout << "we are in Message::ToString" << std::endl;
    return (IntToBytes(messageLength) + static_cast<char>(id) + payload);
}

