#include "byte_tools.h"

int BytesToInt(std::string_view bytes) {
    int result = int((unsigned char)(bytes[0]) << 24 |   (unsigned char)(bytes[1]) << 16 |
                     (unsigned char)(bytes[2]) << 8 |    (unsigned char)(bytes[3]));
    return result;
}
