#pragma once

#include <string>

namespace anvil::core {

// .lic 的实际落盘格式（LibLicenseUI.sub_100057E0 的逆）：
//   明文 XML -> 两两展开(前位补 0) -> 与 "mythware" 循环异或 -> hex 文本
//
// 客户端读文件后先 hex2bin、异或、相邻两字节再异或，才交给 TinyXML。
class LicCodec {
public:
    static constexpr const char* kKey = "mythware";

    // 明文 XML -> .lic 文本（可直接写文件）
    static std::string Encode(const std::string& xml);

    // .lic 文本 -> 明文 XML，用于自检 / 查看
    static std::string Decode(const std::string& payload);
};

} // namespace anvil::core
