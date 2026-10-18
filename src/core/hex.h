#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace anvil::core {

std::string ToHex(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> FromHex(const std::string& text);

} // namespace anvil::core
