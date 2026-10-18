#include "core/serial.h"

#include <cctype>
#include <random>

namespace anvil::core {
namespace {

bool IsAlnum(char c) {
    const unsigned char u = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(c)));
    return (u >= '0' && u <= '9') || (u >= 'A' && u <= 'Z');
}

} // namespace

bool Serial::IsValid(const std::string& serial) {
    const std::size_t size = serial.size();
    if (size != 20 && size != 24) {
        return false;
    }

    for (std::size_t i = 0; i < size; ++i) {
        if (IsAlnum(serial[i])) {
            continue;
        }
        // 非字母数字：只有 24 位格式允许横杠，且必须落在 (i+1)%5==0 的位置
        if (size != 24) {
            return false;
        }
        if ((i + 1) % 5 != 0 || serial[i] != '-') {
            return false;
        }
    }
    return true;
}

std::string Serial::Normalize(const std::string& raw) {
    std::string alnum;
    alnum.reserve(raw.size());
    for (char c : raw) {
        if (IsAlnum(c)) {
            alnum.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
    }
    if (alnum.size() == 20 || alnum.size() == 24) {
        return alnum;
    }
    return {};
}

std::string Serial::Random() {
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device device;
    std::mt19937 engine(device());
    std::uniform_int_distribution<int> pick(0, static_cast<int>(sizeof(kAlphabet) - 2));

    std::string serial(20, '0');
    for (char& c : serial) {
        c = kAlphabet[pick(engine)];
    }
    return serial;
}

std::string Serial::Pretty(const std::string& serial) {
    if (serial.size() != 20) {
        return serial;
    }
    std::string out;
    out.reserve(24);
    for (std::size_t i = 0; i < serial.size(); ++i) {
        if (i != 0 && i % 4 == 0) {
            out.push_back('-');
        }
        out.push_back(serial[i]);
    }
    return out;
}

} // namespace anvil::core
