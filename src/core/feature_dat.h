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
    // 所有可能装了 Feature.dat 的路径：注册表 Topdomain 下每个子键的 TargetDirectory
    // + C:\Program Files (x86)\Mythware\* 各个版本目录，按存在性过滤、去重
    static std::vector<std::string> Candidates();

    // 第一个存在的候选（注册表优先）
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
