#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace anvil::core {

// 安装目录里的 Feature.dat 是一个 DES-CBC(IV=0) 加密的键值表：
//   "PCLE" + 版本(0x00020004) + 记录数，随后每条 260 字节（键名 256 字节 + uint32 值）
// 产品号就是其中的 UninstId0..UninstId(N-1)，每个版本/版本类型各自不同。
class FeatureData {
public:
    // 从注册表 Topdomain\e-Learning Class V6.0 的 TargetDirectory 定位安装目录
    static std::string DefaultPath();

    bool Load(const std::string& path);

    bool ok() const { return ok_; }
    const std::string& error() const { return error_; }

    int ProductIdCount() const;              // UninstIdCount
    std::vector<int> ProductIds() const;     // UninstId0..UninstId(N-1)
    int TrailDays() const;                   // 试用天数
    std::uint32_t Get(const std::string& key) const;

private:
    bool ok_ = false;
    std::string error_;
    std::map<std::string, std::uint32_t> values_;
};

} // namespace anvil::core
