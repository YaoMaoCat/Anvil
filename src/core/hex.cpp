#include "core/hex.h"

namespace anvil::core {
namespace {

int Nibble(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

} // namespace

std::string ToHex(const std::vector<uint8_t>& bytes) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.resize(bytes.size() * 2);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        out[i * 2] = kHex[bytes[i] >> 4];
        out[i * 2 + 1] = kHex[bytes[i] & 0x0F];
    }
    return out;
}

std::vector<uint8_t> FromHex(const std::string& text) {
    std::vector<uint8_t> out;
    out.reserve(text.size() / 2);
    for (std::size_t i = 0; i + 1 < text.size(); i += 2) {
        const int high = Nibble(text[i]);
        const int low = Nibble(text[i + 1]);
        if (high < 0 || low < 0) {
            break;
        }
        out.push_back(static_cast<uint8_t>((high << 4) | low));
    }
    return out;
}

} // namespace anvil::core
