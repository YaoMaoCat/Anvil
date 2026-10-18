#include "core/license.h"

#include "core/lic_codec.h"

#include <ctime>
#include <cstdio>
#include <fstream>

namespace anvil::core {
namespace {

std::string Int(int value) {
    return std::to_string(value);
}

} // namespace

std::string License::Now() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);

    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
                  local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                  local.tm_hour, local.tm_min, local.tm_sec);
    return buffer;
}

std::string License::BuildXml(const LicenseSpec& spec) {
    const int value = spec.wks_value != 0 ? spec.wks_value : spec.product;
    const std::string gettime = spec.gettime.empty() ? Now() : spec.gettime;

    std::string xml;
    xml.reserve(420);
    xml += "<?xml version='1.0'?><wks value='";
    xml += Int(value);
    xml += "' mainver='";
    xml += Int(spec.mainver);
    xml += "' subver='";
    xml += Int(spec.subver);
    xml += "' version='";
    xml += Int(spec.version);
    xml += "'><serialnumber>";
    xml += spec.serial;
    xml += "</serialnumber><signature>";
    xml += spec.machine_code;
    xml += "</signature><software><product number='";
    xml += Int(spec.product);
    xml += "' version='";
    xml += Int(spec.version);
    xml += "'/></software><license period='";
    xml += spec.period;
    xml += "' gettime='";
    xml += gettime;
    xml += "' bindcount='";
    xml += Int(spec.bindcount);
    xml += "'>";
    xml += Int(spec.bindcount);
    xml += "</license><eclass type='";
    xml += Int(spec.eclass_type);
    xml += "' value='";
    xml += Int(spec.eclass_value);
    xml += "' teacher='";
    xml += Int(spec.eclass_teacher);
    xml += "'/></wks>";
    return xml;
}

bool License::Write(const LicenseSpec& spec, const std::string& path, std::string* error) {
    if (spec.machine_code.empty()) {
        if (error != nullptr) {
            *error = "机器特征码为空";
        }
        return false;
    }

    const std::string payload = LicCodec::Encode(BuildXml(spec));

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        if (error != nullptr) {
            *error = "无法写入文件: " + path;
        }
        return false;
    }
    file.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    if (!file) {
        if (error != nullptr) {
            *error = "写入过程中出错: " + path;
        }
        return false;
    }
    return true;
}

} // namespace anvil::core
