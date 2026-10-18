#pragma once

#include <string>

namespace anvil::core {

// 从正在运行的教师端读取授权相关参数。
// 这些值来自安装目录的 Feature.dat（每个版本/版本类型各自不同），
// 所以不能在代码里写死，只能问运行中的客户端要。
struct ClientProbe {
    bool ok = false;
    std::string error;
    int product = 0;    // 产品号（Feature.dat 的 UninstId%d，实测本机 10652）
    int mainver = 0;    // <wks mainver> 必须等于它（实测 1）
    int mask = 0;       // eclass type 掩码（实测 3 → 只能 1/2）
    std::string machine_code;
    std::string cpu;
    std::string macs;
    int pid = 0;
};

// 进程没起、模块没加载、偏移对不上都会返回 ok=false 并写明原因
ClientProbe ReadFromTeacher(int timeout_ms = 0);

} // namespace anvil::core
