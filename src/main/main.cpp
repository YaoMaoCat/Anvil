#include "core/client_probe.h"
#include "core/feature_dat.h"
#include "core/lic_codec.h"
#include "core/license.h"
#include "core/machine_id.h"
#include "core/serial.h"
#include "ui/app.h"

#include <windows.h>
#include <shellapi.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

// 控制台给的 argv 是 ANSI 的，中文路径会变成 '?'；统一改从宽字符命令行取 UTF-8
std::vector<std::string> WideArgs() {
    std::vector<std::string> args;
    int count = 0;
    LPWSTR* wide = CommandLineToArgvW(GetCommandLineW(), &count);
    if (wide == nullptr) {
        return args;
    }
    for (int i = 1; i < count; ++i) {
        const int size = WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, nullptr, 0, nullptr, nullptr);
        std::string item;
        if (size > 1) {
            item.assign(static_cast<std::size_t>(size - 1), ' ');
            WideCharToMultiByte(CP_UTF8, 0, wide[i], -1, item.data(), size, nullptr, nullptr);
        }
        args.push_back(item);
    }
    LocalFree(wide);
    return args;
}

std::wstring Utf8ToWide(const std::string& text) {
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring out(static_cast<std::size_t>(size > 0 ? size - 1 : 0), L' ');
    if (size > 1) {
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), size);
    }
    return out;
}

void PrintUsage() {
    std::cout <<
        "Anvil - 授权文件生成器\n"
        "\n"
        "  Anvil                       打开图形界面\n"
        "  Anvil --feature [文件|目录]  列出/解析 Feature.dat（不填则扫描全部已装版本）\n"
        "  Anvil --probe               从运行中的教师端读取产品号/机器码\n"
        "  Anvil --dump <file.lic>     解密并打印明文 XML\n"
        "  Anvil --code <机器码> [选项] 命令行生成\n"
        "\n"
        "生成选项：\n"
        "  --out <file>       输出路径，默认 <序列号>.lic\n"
        "  --serial <20位>    指定序列号，默认随机\n"
        "  --product <n>      产品号，默认从 Feature.dat 自动读取\n"
        "  --students <n>     学生席位，默认 1\n"
        "  --period <时间>    到期时间，默认 2099-12-31 23:59:59\n";
}

int RunDump(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "无法打开: " << path << "\n";
        return 2;
    }
    const std::string payload((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    std::cout << anvil::core::LicCodec::Decode(payload) << "\n";
    return 0;
}

// 传目录时自动补 Feature.dat
std::string NormalizeFeaturePath(const std::string& path) {
    if (path.empty()) {
        return path;
    }
    const bool looks_like_file = path.size() > 4 && path.compare(path.size() - 4, 4, ".dat") == 0;
    if (looks_like_file) {
        return path;
    }
    std::string full = path;
    if (full.back() != '\\' && full.back() != '/') {
        full.push_back('\\');
    }
    return full + "Feature.dat";
}

int ReportFeature(const std::string& path) {
    if (GetFileAttributesW(Utf8ToWide(path).c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::cout << "[--] " << path << "\n     该目录没有 Feature.dat（版本可能未完整安装）\n";
        return 1;
    }
    anvil::core::FeatureData data;
    if (!data.Load(path)) {
        std::cout << "[!!] " << path << "\n     " << data.error() << "\n";
        return 1;
    }
    std::cout << "[OK] " << path << "\n"
              << "     试用天数 " << data.TrailDays() << "，产品表 " << data.ProductIdCount() << " 项：";
    for (int id : data.ProductIds()) {
        std::cout << " " << id;
    }
    std::cout << "\n";
    return 0;
}

int RunFeature(const std::string& given_path) {
    if (!given_path.empty()) {
        return ReportFeature(NormalizeFeaturePath(given_path)) == 0 ? 0 : 4;
    }

    const std::vector<std::string> paths = anvil::core::FeatureData::Candidates();
    if (paths.empty()) {
        std::cerr << "没有找到任何 Feature.dat（注册表 Topdomain 与 Mythware 目录里都没有）\n";
        return 4;
    }
    std::cout << "共找到 " << paths.size() << " 个已安装版本：\n\n";
    int bad = 0;
    for (const std::string& path : paths) {
        bad += ReportFeature(path);
    }
    return bad == 0 ? 0 : 0;
}

int RunProbe() {
    const anvil::core::ClientProbe probe = anvil::core::ReadFromTeacher(3000);
    if (!probe.ok) {
        std::cerr << "读取失败: " << probe.error << "\n";
        return 4;
    }
    std::cout << "进程       : " << probe.pid << "\n"
              << "产品号     : " << probe.product << "\n"
              << "mainver    : " << probe.mainver << "\n"
              << "eclass 掩码: " << probe.mask << "\n"
              << "机器特征码 : " << probe.machine_code << "\n"
              << "CPU        : " << probe.cpu << "\n"
              << "网卡       : " << probe.macs << "\n";
    return 0;
}

int RunGenerate(const std::vector<std::string>& args) {
    anvil::core::LicenseSpec spec;
    std::string output;
    bool product_set = false;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& key = args[i];
        const bool has_value = i + 1 < args.size();
        if (key == "--code" && has_value) {
            spec.machine_code = args[++i];
        } else if (key == "--out" && has_value) {
            output = args[++i];
        } else if (key == "--serial" && has_value) {
            spec.serial = anvil::core::Serial::Normalize(args[++i]);
        } else if (key == "--product" && has_value) {
            spec.product = std::atoi(args[++i].c_str());
            spec.subver = spec.product;
            product_set = true;
        } else if (key == "--students" && has_value) {
            spec.eclass_value = std::atoi(args[++i].c_str());
        } else if (key == "--period" && has_value) {
            spec.period = args[++i];
        }
    }


    if (!product_set) {
        anvil::core::FeatureData data;
        if (data.Load(anvil::core::FeatureData::DefaultPath())) {
            const std::vector<int> ids = data.ProductIds();
            if (!ids.empty()) {
                spec.product = ids.front();
                spec.subver = ids.front();
                std::cout << "产品号取自 Feature.dat: " << spec.product << "\n";
            }
        }
    }
    if (spec.serial.empty()) {
        spec.serial = anvil::core::Serial::Random();
    }
    if (!anvil::core::Serial::IsValid(spec.serial)) {
        std::cerr << "序列号必须是 20 位字母数字（不带横杠）\n";
        return 2;
    }
    if (spec.machine_code.empty()) {
        spec.machine_code = anvil::core::MachineId::Local();
        std::cout << "未指定 --code，使用本机特征码: " << spec.machine_code << "\n";
    }
    if (output.empty()) {
        output = spec.serial + ".lic";
    }

    std::string error;
    if (!anvil::core::License::Write(spec, output, &error)) {
        std::cerr << "生成失败: " << error << "\n";
        return 3;
    }

    std::cout << "产品号   : " << spec.product << "\n"
              << "机器码   : " << spec.machine_code << "\n"
              << "序列号   : " << anvil::core::Serial::Pretty(spec.serial) << "\n"
              << "学生席位 : " << spec.eclass_value << "\n"
              << "到期     : " << spec.period << "\n"
              << "已写出   : " << output << "\n";
    return 0;
}

} // namespace

int main() {
    const std::vector<std::string> args = WideArgs();

    if (args.empty()) {
        return anvil::ui::Run();
    }
    if (args[0] == "--help" || args[0] == "-h") {
        PrintUsage();
        return 0;
    }
    if (args[0] == "--feature") {
        return RunFeature(args.size() >= 2 ? args[1] : std::string{});
    }
    if (args[0] == "--probe") {
        return RunProbe();
    }
    if (args[0] == "--dump" && args.size() >= 2) {
        return RunDump(args[1]);
    }
    return RunGenerate(args);
}
