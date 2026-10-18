#include "core/client_probe.h"

#include <windows.h>
#include <tlhelp32.h>

#include <chrono>
#include <string>
#include <cstdio>
#include <thread>

namespace anvil::core {
namespace {

// CLicenseUI 实例内的字段偏移与模块内全局指针（LibLicenseUI.dll 6.0 2017）
constexpr DWORD kGlobalObject = 0x33780;
constexpr DWORD kOffMainVer = 0x3B760;
constexpr DWORD kOffProduct = 0x3B764;
constexpr DWORD kOffCount = 0x3B76C;
constexpr DWORD kOffMask = 0x3B770;
constexpr DWORD kOffMachineCode = 0x3B77C;
constexpr DWORD kOffCpu = 0x3B2C0;
constexpr DWORD kOffMacs = 0x3B560;

DWORD FindProcessId(const wchar_t* exe_name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    DWORD pid = 0;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, exe_name) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return pid;
}

// 32 位目标也能读：SNAPMODULE32 让 64 位进程枚举到 WOW64 模块
DWORD ModuleBase(DWORD pid, const wchar_t* module_name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    MODULEENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    DWORD base = 0;
    if (Module32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szModule, module_name) == 0) {
                base = static_cast<DWORD>(reinterpret_cast<ULONG_PTR>(entry.modBaseAddr));
                break;
            }
        } while (Module32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return base;
}

bool ReadBytes(HANDLE process, DWORD address, void* buffer, SIZE_T size) {
    SIZE_T read = 0;
    return ReadProcessMemory(process, reinterpret_cast<LPCVOID>(static_cast<ULONG_PTR>(address)), buffer, size,
                             &read) != FALSE && read == size;
}

DWORD ReadDword(HANDLE process, DWORD address) {
    DWORD value = 0;
    ReadBytes(process, address, &value, sizeof(value));
    return value;
}

std::string ReadWide(HANDLE process, DWORD address, std::size_t chars) {
    std::wstring text(chars, L'\0');
    if (!ReadBytes(process, address, text.data(), chars * sizeof(wchar_t))) {
        return {};
    }
    const std::size_t end = text.find(L'\0');
    if (end != std::wstring::npos) {
        text.resize(end);
    }
    std::string out;
    out.reserve(text.size());
    for (wchar_t ch : text) {
        out.push_back(ch < 0x80 ? static_cast<char>(ch) : '?');
    }
    return out;
}

ClientProbe ProbeOnce() {
    ClientProbe result;
    result.pid = static_cast<int>(FindProcessId(L"TeacherMain.exe"));
    if (result.pid == 0) {
        result.error = "没有找到运行中的 TeacherMain.exe，请先打开教师端";
        return result;
    }

    const DWORD module = ModuleBase(static_cast<DWORD>(result.pid), L"LibLicenseUI.dll");
    if (module == 0) {
        result.error = "TeacherMain 已运行，但还没加载 LibLicenseUI.dll（授权窗口未初始化）";
        return result;
    }

    HANDLE process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE,
                                 static_cast<DWORD>(result.pid));
    if (process == nullptr) {
        result.error = "打开进程失败，可能需要管理员权限";
        return result;
    }

    const DWORD object = ReadDword(process, module + kGlobalObject);
    if (object != 0) {
        result.product = static_cast<int>(ReadDword(process, object + kOffProduct));
        result.mainver = static_cast<int>(ReadDword(process, object + kOffMainVer));
        result.mask = static_cast<int>(ReadDword(process, object + kOffMask));
        result.machine_code = ReadWide(process, object + kOffMachineCode, 64);
        result.cpu = ReadWide(process, object + kOffCpu, 128);
        result.macs = ReadWide(process, object + kOffMacs, 128);
        const DWORD count = ReadDword(process, object + kOffCount);
        if (result.product > 0 && count == 0) {
            result.error = "产品表为空（count=0），请等授权窗口完全打开后重试";
            result.product = 0;
        }
    } else {
        result.error = "CLicenseUI 尚未创建，请在教师端打开「授权」窗口后重试";
    }

    CloseHandle(process);
    // 不同版本的 DLL 布局可能不同，读到明显不合理的值就明确报错，不要给假数据
    if (result.error.empty()) {
        const bool digits_only = !result.machine_code.empty() &&
                                 result.machine_code.find_first_not_of("0123456789.") == std::string::npos;
        if (!digits_only || result.product <= 0 || result.product > 10000000) {
            result.error = "读到的字段不合理（该版本 DLL 的偏移可能不同），请手动填写产品号";
            result.product = 0;
        }
    }
    result.ok = result.error.empty() && result.product > 0;
    return result;
}

} // namespace

ClientProbe ReadFromTeacher(int timeout_ms) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    ClientProbe probe = ProbeOnce();
    while (!probe.ok && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        probe = ProbeOnce();
    }
    return probe;
}

} // namespace anvil::core
