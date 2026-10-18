#include "core/lic_codec.h"

#include "core/hex.h"

#include <cstdint>
#include <vector>

namespace anvil::core {
namespace {

constexpr std::size_t kKeyLength = 8; // strlen("mythware")

void ApplyKey(std::vector<uint8_t>& data) {
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] ^= static_cast<uint8_t>(LicCodec::kKey[i % kKeyLength]);
    }
}

} // namespace

std::string LicCodec::Encode(const std::string& xml) {
    std::vector<uint8_t> expanded;
    expanded.resize(xml.size() * 2);
    for (std::size_t i = 0; i < xml.size(); ++i) {
        expanded[i * 2] = 0x00;
        expanded[i * 2 + 1] = static_cast<uint8_t>(xml[i]);
    }
    ApplyKey(expanded);
    return ToHex(expanded);
}

std::string LicCodec::Decode(const std::string& payload) {
    std::vector<uint8_t> raw = FromHex(payload);
    ApplyKey(raw);

    std::string out;
    out.reserve(raw.size() / 2);
    for (std::size_t i = 0; i + 1 < raw.size(); i += 2) {
        out.push_back(static_cast<char>(raw[i] ^ raw[i + 1]));
    }
    return out;
}

} // namespace anvil::core
