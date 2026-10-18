#include "core/machine_id.h"

#include "core/md5.h"

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <intrin.h>

#include <cstdio>
#include <cstring>
#include <cwctype>
#include <string>

namespace anvil::core {
namespace {

std::string FormatMac(const BYTE* address, ULONG length) {
    if (address == nullptr || length < 6) {
        return {};
    }
    char buffer[13] = {};
    std::snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X%02X%02X",
                  address[0], address[1], address[2], address[3], address[4], address[5]);
    return buffer;
}

bool ContainsVirtualKeyword(const wchar_t* description) {
    if (description == nullptr) {
        return false;
    }
    // 与客户端一致：虚拟/隧道/蓝牙等网卡跨机器会变，不能进特征码
    static const wchar_t* const kKeywords[] = {
        L"virtual", L"vmware", L"virtualbox", L"hyper-v", L"tap-", L"tap ",
        L"tun", L"radmin", L"loopback", L"teredo", L"isatap", L"npcap",
        L"wintun", L"bluetooth", L"pseudo", L"teaming", L"wan miniport",
        L"wi-fi direct", L"packet scheduler", L"miniport"};

    std::wstring text(description);
    for (wchar_t& ch : text) {
        ch = static_cast<wchar_t>(std::towlower(ch));
    }
    for (const wchar_t* keyword : kKeywords) {
        if (text.find(keyword) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

bool IsUsableMac(const std::string& mac) {
    if (mac.size() != 12) {
        return false;
    }
    if (mac == "000000000000" || mac == "FFFFFFFFFFFF") {
        return false;
    }
    return true;
}

} // namespace

std::string MachineId::CpuBrand() {
    int regs[4] = {};
    __cpuid(regs, static_cast<int>(0x80000000));
    const unsigned max_leaf = static_cast<unsigned>(regs[0]);
    if (max_leaf < 0x80000004u) {
        return {};
    }

    char brand[49] = {};
    for (unsigned leaf = 0; leaf < 3; ++leaf) {
        __cpuid(regs, static_cast<int>(0x80000002u + leaf));
        std::memcpy(brand + leaf * 16, regs, 16);
    }

    std::string out(brand);
    while (!out.empty() && (out.back() == ' ' || out.back() == '\0')) {
        out.pop_back();
    }
    std::size_t begin = 0;
    while (begin < out.size() && out[begin] == ' ') {
        ++begin;
    }
    return out.substr(begin);
}

std::vector<std::string> MachineId::MacAddresses() {
    ULONG size = 16 * 1024;
    std::vector<BYTE> buffer(size);

    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                        GAA_FLAG_SKIP_DNS_SERVER;

    ULONG result = ERROR_BUFFER_OVERFLOW;
    for (int attempt = 0; attempt < 4; ++attempt) {
        result = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr,
                                      reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()),
                                      &size);
        if (result == ERROR_SUCCESS) {
            break;
        }
        if (result != ERROR_BUFFER_OVERFLOW) {
            return {};
        }
        buffer.resize(size);
    }
    if (result != ERROR_SUCCESS) {
        return {};
    }

    std::vector<std::string> macs;
    for (auto* item = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()); item != nullptr;
         item = item->Next) {
        if (item->IfType != IF_TYPE_ETHERNET_CSMACD && item->IfType != IF_TYPE_IEEE80211) {
            continue;
        }
        if (ContainsVirtualKeyword(item->Description)) {
            continue;
        }
        std::string mac = FormatMac(item->PhysicalAddress, item->PhysicalAddressLength);
        if (!IsUsableMac(mac)) {
            continue;
        }
        bool duplicate = false;
        for (const auto& existing : macs) {
            if (existing == mac) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            macs.push_back(std::move(mac));
        }
    }
    return macs;
}

std::string MachineId::Segment(const std::string& cpu, const std::string& mac) {
    const std::string digest = md5_hex(cpu + mac);
    std::string digits;
    digits.reserve(digest.size() / 2);
    for (std::size_t i = 0; i < digest.size(); i += 2) {
        digits.push_back(static_cast<char>('0' + (digest[i] % 10)));
    }
    return digits;
}

std::string MachineId::Code(const std::string& cpu, const std::vector<std::string>& macs) {
    std::string code;
    const std::size_t limit = macs.size() < 3 ? macs.size() : 3;
    for (std::size_t i = 0; i < limit; ++i) {
        if (!code.empty()) {
            code.push_back('.');
        }
        code += Segment(cpu, macs[i]);
    }
    return code;
}

std::string MachineId::Local(std::string* cpu_out) {
    const std::string cpu = CpuBrand();
    if (cpu_out != nullptr) {
        *cpu_out = cpu;
    }
    return Code(cpu, MacAddresses());
}

} // namespace anvil::core
