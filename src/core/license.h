#pragma once

#include <string>

namespace anvil::core {

// 一条 wks 授权。字段含义全部来自实测（见 README 的判定链）：
//   mainver  -> 与 CLicenseUI+0x3B760 比较，必须为 1，否则错误码 7
//   subver   -> 拿去做产品表查表，必须等于 Feature.dat 里的 UninstId，否则错误码 8
//   signature-> 机器特征码，客户端直接比对，不符 0x66
struct LicenseSpec {
    std::string machine_code;
    std::string serial;      // 20 位字母数字，不带横杠
    int product = 10652;     // <software><product number>
    int mainver = 1;
    int subver = 10652;      // 默认同 product
    int version = 0;
    int wks_value = 0;       // 0 表示跟随 product
    int eclass_type = 1;     // 1=站点 2=学校；掩码为 3，其它值 0x65
    int eclass_value = 1;    // 学生授权数
    int eclass_teacher = 1;
    int bindcount = 1;
    std::string period = "2099-12-31 23:59:59";
    std::string gettime;     // 空则取当前本地时间
};

class License {
public:
    static std::string Now();

    static std::string BuildXml(const LicenseSpec& spec);

    // 生成并写出 .lic；失败时通过 error 返回原因
    static bool Write(const LicenseSpec& spec, const std::string& path, std::string* error);
};

} // namespace anvil::core
