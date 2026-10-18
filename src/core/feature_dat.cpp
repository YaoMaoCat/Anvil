#include "core/feature_dat.h"

#include <windows.h>
#include <wincrypt.h>

#include <cstring>

namespace anvil::core {
namespace {

// EXE 里 0x4E4149F5 / 0x494E4E51 / 0x7BE / 0x962 四个常量异或后拼出的 8 字节 DES 密钥
constexpr char kDesKey[] = "3GNIKNAN";
constexpr std::size_t kBlock = 8;
constexpr std::size_t kHeader = 12;
constexpr std::size_t kRecord = 260;
constexpr std::size_t kKeyField = 256;

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                         nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(size), ' ');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        out.data(), size, nullptr, nullptr);
    return out;
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring out(static_cast<std::size_t>(size), L' ');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), size);
    return out;
}

bool DesCbcDecrypt(const std::string& input, std::string* output, std::string* error) {
    HCRYPTPROV provider = 0;
    if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        *error = "CryptAcquireContext 失败";
        return false;
    }

    // PLAINTEXTKEYBLOB: bType=8, bVersion=2, aiKeyAlg=CALG_DES，后跟密钥长度与密钥
    constexpr DWORD kDesKeyLen = 8; // DES 只接受 8 字节，字符串末尾的 '\0' 不能算进去
    BYTE blob[sizeof(BLOBHEADER) + sizeof(DWORD) + kDesKeyLen] = {};
    auto* header = reinterpret_cast<BLOBHEADER*>(blob);
    header->bType = PLAINTEXTKEYBLOB;
    header->bVersion = CUR_BLOB_VERSION;
    header->aiKeyAlg = CALG_DES;
    DWORD key_size = kDesKeyLen;
    std::memcpy(blob + sizeof(BLOBHEADER), &key_size, sizeof(key_size));
    std::memcpy(blob + sizeof(BLOBHEADER) + sizeof(DWORD), kDesKey, kDesKeyLen);

    HCRYPTKEY key = 0;
    if (!CryptImportKey(provider, blob, sizeof(blob), 0, 0, &key)) {
        CryptReleaseContext(provider, 0);
        *error = "CryptImportKey 失败（DES 密钥被拒绝）";
        return false;
    }

    output->clear();
    output->reserve(input.size());
    BYTE previous[kBlock] = {}; // CBC，IV 全 0
    for (std::size_t offset = 0; offset + kBlock <= input.size(); offset += kBlock) {
        BYTE block[kBlock];
        std::memcpy(block, input.data() + offset, kBlock);
        DWORD size = kBlock;
        if (!CryptDecrypt(key, 0, FALSE, 0, block, &size)) {
            CryptDestroyKey(key);
            CryptReleaseContext(provider, 0);
            *error = "CryptDecrypt 失败";
            return false;
        }
        for (std::size_t i = 0; i < kBlock; ++i) {
            output->push_back(static_cast<char>(block[i] ^ previous[i]));
        }
        std::memcpy(previous, input.data() + offset, kBlock);
    }

    CryptDestroyKey(key);
    CryptReleaseContext(provider, 0);
    return true;
}

} // namespace

std::string FeatureData::DefaultPath() {
    HKEY key = nullptr;
    const wchar_t* sub = L"SOFTWARE\\Topdomain\\e-Learning Class V6.0";
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, sub, 0, KEY_READ | KEY_WOW64_32KEY, &key) != ERROR_SUCCESS &&
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, sub, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return {};
    }

    wchar_t buffer[MAX_PATH * 2] = {};
    DWORD size = sizeof(buffer) - sizeof(wchar_t);
    DWORD type = 0;
    const LONG status = RegQueryValueExW(key, L"TargetDirectory", nullptr, &type,
                                        reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || type != REG_SZ) {
        return {};
    }

    std::wstring directory(buffer);
    if (!directory.empty() && directory.back() != L'\\' && directory.back() != L'/') {
        directory.push_back(L'\\');
    }
    directory += L"Feature.dat";
    return WideToUtf8(directory);
}

bool FeatureData::Load(const std::string& path) {
    values_.clear();
    ok_ = false;
    error_.clear();

    if (path.empty()) {
        error_ = "没找到安装目录（注册表 Topdomain\\e-Learning Class V6.0 无 TargetDirectory）";
        return false;
    }

    const std::wstring wide_path = Utf8ToWide(path);
    HANDLE file = CreateFileW(wide_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        error_ = "打不开 " + path;
        return false;
    }
    LARGE_INTEGER file_size = {};
    GetFileSizeEx(file, &file_size);
    std::string cipher(static_cast<std::size_t>(file_size.QuadPart), ' ');
    DWORD read = 0;
    const BOOL read_ok = ReadFile(file, cipher.data(), static_cast<DWORD>(cipher.size()), &read, nullptr);
    CloseHandle(file);
    if (read_ok == FALSE || read != cipher.size()) {
        error_ = "读取不完整 " + path;
        return false;
    }

    std::string plain;
    if (!DesCbcDecrypt(cipher, &plain, &error_)) {
        return false;
    }
    if (plain.size() < kHeader || plain.compare(0, 4, "PCLE") != 0) {
        error_ = "不是 PCLE 结构（密钥或算法与该版本不符）";
        return false;
    }

    DWORD version = 0;
    DWORD count = 0;
    std::memcpy(&version, plain.data() + 4, sizeof(version));
    std::memcpy(&count, plain.data() + 8, sizeof(count));
    if (version != 0x00020004u || count == 0 || count > 4096) {
        error_ = "头部异常（version=" + std::to_string(version) + "，count=" + std::to_string(count) + "）";
        return false;
    }

    for (DWORD i = 0; i < count; ++i) {
        const std::size_t base = kHeader + i * kRecord;
        if (base + kRecord > plain.size()) {
            break;
        }
        std::string name(plain.data() + base, kKeyField);
        const std::size_t nul = name.find('\0');
        if (nul != std::string::npos) {
            name.resize(nul);
        }
        DWORD value = 0;
        std::memcpy(&value, plain.data() + base + kKeyField, sizeof(value));
        values_[name] = value;
    }

    ok_ = !values_.empty();
    if (!ok_) {
        error_ = "解析后没有任何记录";
    }
    return ok_;
}

std::uint32_t FeatureData::Get(const std::string& key) const {
    const auto it = values_.find(key);
    return it == values_.end() ? 0 : it->second;
}

int FeatureData::ProductIdCount() const {
    return static_cast<int>(Get("UninstIdCount"));
}

std::vector<int> FeatureData::ProductIds() const {
    std::vector<int> ids;
    const int count = ProductIdCount();
    for (int i = 0; i < count && i < 64; ++i) {
        const int id = static_cast<int>(Get("UninstId" + std::to_string(i)));
        if (id > 0) {
            ids.push_back(id);
        }
    }
    return ids;
}

int FeatureData::TrailDays() const {
    return static_cast<int>(Get("TrailDays"));
}

} // namespace anvil::core
