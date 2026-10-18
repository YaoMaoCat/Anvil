#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace anvil::core {

// 标准 MD5，输出 32 位小写十六进制。
// 客户端用的就是它（sub_1000B5C0 里的 IV 是标准 MD5 初始化向量）。
std::string md5_hex(const void* data, std::size_t size);
std::string md5_hex(const std::string& text);

} // namespace anvil::core
