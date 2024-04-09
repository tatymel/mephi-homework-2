#include "byte_tools.h"
#include <openssl/sha.h>
#include <vector>
#include <sstream>

int BytesToInt(std::string_view bytes) {
    int result = int((unsigned char)(bytes[0]) << 24 |
                     (unsigned char)(bytes[1]) << 16 |
                     (unsigned char)(bytes[2]) << 8 |
                     (unsigned char)(bytes[3]));
    return result;
}
std::string IntToBytes(int number){
    unsigned char bytes[4];
    bytes[0] = (unsigned char)(number >> 24);
    bytes[1] = (unsigned char)(number >> 16);
    bytes[2] = (unsigned char)(number >> 8);
    bytes[3] = (unsigned char)(number);
    std::string result;
    for(auto elem : bytes)
        result += elem;
    return result;
}

std::string CalculateSHA1(const std::string& msg) {
    unsigned char sha[20];
    std::string infoHash;
    SHA1((unsigned char *) msg.c_str(), msg.size(), sha);
    for(unsigned char i : sha)
        infoHash += (char)i;
    return infoHash;
}


/*
 * Представить массив байтов в виде строки, содержащей только символы, соответствующие цифрам в шестнадцатеричном исчислении.
 * Конкретный формат выходной строки не важен. Важно то, чтобы выходная строка не содержала символов, которые нельзя
 * было бы представить в кодировке utf-8. Данная функция будет использована для вывода SHA1 хеш-суммы в лог.
 */
std::string HexEncode(const std::string& input){
    std::stringstream hexStr;
    for(auto& elem : input){
        hexStr << std::hex << (int)elem;
    }
    return hexStr.str();
}
