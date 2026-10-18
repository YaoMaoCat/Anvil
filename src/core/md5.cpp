#include "core/md5.h"

#include <cstring>

namespace anvil::core {
namespace {

constexpr uint32_t kShift[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

constexpr uint32_t kTable[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
    0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
    0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
    0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
    0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
    0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

inline uint32_t RotateLeft(uint32_t value, uint32_t count) {
    return (value << count) | (value >> (32 - count));
}

inline uint32_t LoadLE32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

void Transform(uint32_t state[4], const uint8_t block[64]) {
    uint32_t w[16];
    for (int i = 0; i < 16; ++i) {
        w[i] = LoadLE32(block + i * 4);
    }

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];

    for (uint32_t i = 0; i < 64; ++i) {
        uint32_t f = 0;
        uint32_t g = 0;
        if (i < 16) {
            f = (b & c) | (~b & d);
            g = i;
        } else if (i < 32) {
            f = (d & b) | (~d & c);
            g = (5 * i + 1) % 16;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = (3 * i + 5) % 16;
        } else {
            f = c ^ (b | ~d);
            g = (7 * i) % 16;
        }

        const uint32_t tmp = d;
        d = c;
        c = b;
        b = b + RotateLeft(a + f + kTable[i] + w[g], kShift[i]);
        a = tmp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

} // namespace

std::string md5_hex(const void* data, std::size_t size) {
    uint32_t state[4] = {0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u};

    const auto* bytes = static_cast<const uint8_t*>(data);
    std::size_t offset = 0;

    for (; offset + 64 <= size; offset += 64) {
        Transform(state, bytes + offset);
    }

    uint8_t tail[128] = {};
    const std::size_t remain = size - offset;
    if (remain > 0) {
        std::memcpy(tail, bytes + offset, remain);
    }
    tail[remain] = 0x80;

    // remain + 1(0x80) + 8(长度) 必须落在一个 64 字节块内
    const std::size_t tail_size = (remain < 56) ? 64 : 128;
    const uint64_t bit_length = static_cast<uint64_t>(size) * 8;
    for (int i = 0; i < 8; ++i) {
        tail[tail_size - 8 + i] = static_cast<uint8_t>((bit_length >> (8 * i)) & 0xFF);
    }

    Transform(state, tail);
    if (tail_size == 128) {
        Transform(state, tail + 64);
    }

    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(32, '0');
    for (int i = 0; i < 4; ++i) {
        for (int byte = 0; byte < 4; ++byte) {
            const uint8_t value = static_cast<uint8_t>((state[i] >> (8 * byte)) & 0xFF);
            out[(i * 4 + byte) * 2] = kHex[value >> 4];
            out[(i * 4 + byte) * 2 + 1] = kHex[value & 0x0F];
        }
    }
    return out;
}

std::string md5_hex(const std::string& text) {
    return md5_hex(text.data(), text.size());
}

} // namespace anvil::core
