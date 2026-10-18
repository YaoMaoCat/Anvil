#pragma once

#include <string>
#include <vector>

namespace anvil::core {

// 机器特征码（客户端 sub_100060D0 / sub_1000B870 / sub_1000B5C0）：
//   seg = digits( MD5(cpu_brand + mac) )
//   code = seg1 '.' seg2 '.' seg3        （最多 3 块网卡）
// digits 取 MD5 十六进制串偶数位字符的 **ASCII 码 % 10**，不是「两位当十六进制」。
class MachineId {
public:
    // CPUID 0x80000002..0x80000004 的品牌串，去掉尾部空格
    static std::string CpuBrand();

    // 物理以太网 / 无线网卡，MAC 大写无分隔符，按系统枚举顺序
    static std::vector<std::string> MacAddresses();

    static std::string Segment(const std::string& cpu, const std::string& mac);

    static std::string Code(const std::string& cpu, const std::vector<std::string>& macs);

    // 本机机器特征码；cpu_out 非空时回传 CPU 串
    static std::string Local(std::string* cpu_out = nullptr);
};

} // namespace anvil::core
