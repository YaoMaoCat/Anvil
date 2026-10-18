#pragma once

#include <string>

namespace anvil::core {

// 序列号规则（客户端 sub_100062C0），按**带横杠后的整串长度**判断：
//   20 位：必须全是 [0-9A-Z]，不允许出现横杠（客户端自己按 (i+1)%5 插入）
//   24 位：横杠只能出现在索引 4 / 9 / 14 / 19，即 XXXX-XXXX-XXXX-XXXX-XXXX
// 不合法时客户端返回 0，最终落到错误码 4。
class Serial {
public:
    static bool IsValid(const std::string& serial);

    // 从用户输入提取 20 位字母数字；无法确定时返回空串
    static std::string Normalize(const std::string& raw);

    // 随机 20 位（A-Z0-9）
    static std::string Random();

    // 20 位 -> 5 组 4 位，用于界面展示
    static std::string Pretty(const std::string& serial);
};

} // namespace anvil::core
