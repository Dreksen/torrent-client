#include "byte_tools.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <openssl/sha.h>

int BytesToInt(std::string_view bytes) {
    if (bytes.size() < 4) {
        throw std::runtime_error("BytesToInt requires at least 4 bytes");
    }

    uint32_t b1 = static_cast<unsigned char>(bytes[0]);
    uint32_t b2 = static_cast<unsigned char>(bytes[1]);
    uint32_t b3 = static_cast<unsigned char>(bytes[2]);
    uint32_t b4 = static_cast<unsigned char>(bytes[3]);
    uint32_t result = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;

    return static_cast<int>(result);
}

std::string IntToBytes(size_t value) {
    std::string bytes(4, '\0');
    bytes[0] = static_cast<char>((value >> 24) & 0xFF); 
    bytes[1] = static_cast<char>((value >> 16) & 0xFF);
    bytes[2] = static_cast<char>((value >> 8) & 0xFF);
    bytes[3] = static_cast<char>(value & 0xFF);         
    
    return bytes;
}

std::string CalculateSHA1(const std::string& msg) {
    unsigned char hash[20];
    SHA1(reinterpret_cast<const unsigned char*>(msg.c_str()), msg.size(), hash);
    return std::string(reinterpret_cast<char*>(hash), 20);
}
