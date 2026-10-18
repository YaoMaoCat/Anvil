#include "ui/version_picker.h"

#include "core/feature_dat.h"

#include <imgui.h>

#include <windows.h>
#include <commdlg.h>

namespace anvil::ui {
namespace {

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

// 手动指定 Feature.dat：默认扫描找不到的版本（比如只留了备份的）也能用
std::string BrowseFeatureFile() {
    wchar_t buffer[MAX_PATH * 2] = {};
    OPENFILENAMEW dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFilter = L"Feature.dat\0Feature.dat\0所有文件\0*.*\0";
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = MAX_PATH * 2;
    dialog.lpstrTitle = L"选择 Feature.dat";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&dialog) == FALSE) {
        return {};
    }
    return WideToUtf8(buffer);
}

// ...\极域课堂管理系统软件V6.0 2016 豪华版\Feature.dat -> V6.0 2016 豪华版
std::string DirectoryLabel(const std::string& feature_path) {
    std::string path = feature_path;
    const std::size_t file_slash = path.find_last_of("\\/");
    if (file_slash != std::string::npos) {
        path.resize(file_slash); // 去掉 Feature.dat
    }
    const std::size_t dir_slash = path.find_last_of("\\/");
    std::string label = dir_slash == std::string::npos ? path : path.substr(dir_slash + 1);

    // 每个版本都带这个前缀，下拉框里没必要重复显示
    const std::string prefix = "\xE6\x9E\x81\xE5\x9F\x9F\xE8\xAF\xBE\xE5\xA0\x82"; // "极域课堂"
    const std::string full_prefix = prefix + "\xE7\xAE\xA1\xE7\x90\x86\xE7\xB3\xBB\xE7\xBB\x9F\xE8\xBD\xAF\xE4\xBB\xB6";
    if (label.rfind(full_prefix, 0) == 0) {
        label.erase(0, full_prefix.size());
    }
    return label;
}

} // namespace

void VersionPicker::Scan() {
    versions_.clear();
    selected_ = 0;

    for (const std::string& path : core::FeatureData::Candidates()) {
        core::FeatureData data;
        if (!data.Load(path)) {
            continue;
        }
        InstalledVersion version;
        version.directory = path;
        version.label = DirectoryLabel(path);
        version.products = data.ProductIds();
        version.trail_days = data.TrailDays();
        if (version.products.empty()) {
            continue;
        }
        versions_.push_back(version);
    }
}

int VersionPicker::AddCustom(const std::string& path) {
    core::FeatureData data;
    if (!data.Load(path)) {
        return 0;
    }
    InstalledVersion version;
    version.directory = path;
    version.label = DirectoryLabel(path);
    version.products = data.ProductIds();
    version.trail_days = data.TrailDays();
    if (version.products.empty()) {
        return 0;
    }
    for (const InstalledVersion& existing : versions_) {
        if (existing.directory == path) {
            return version.products.front();
        }
    }
    versions_.push_back(version);
    selected_ = static_cast<int>(versions_.size()) - 1;
    return version.products.front();
}

int VersionPicker::InitialProduct() const {
    return versions_.empty() ? 0 : versions_.front().products.front();
}

int VersionPicker::SelectedProduct() const {
    if (selected_ < 0 || selected_ >= static_cast<int>(versions_.size())) {
        return 0;
    }
    const InstalledVersion& version = versions_[static_cast<std::size_t>(selected_)];
    return version.products.empty() ? 0 : version.products.front();
}

int VersionPicker::DrawCombo(float scale) {
    ImGui::TextDisabled("安装版本");
    ImGui::SameLine(84.0f * scale);

    int picked = 0;
    if (versions_.empty()) {
        ImGui::TextDisabled("未检测到已安装的 Feature.dat");
    } else {
        ImGui::SetNextItemWidth(196.0f * scale);
        if (ImGui::BeginCombo("##version", versions_[static_cast<std::size_t>(selected_)].label.c_str())) {
            for (int i = 0; i < static_cast<int>(versions_.size()); ++i) {
                const InstalledVersion& version = versions_[static_cast<std::size_t>(i)];
                std::string text = version.label + "  (产品号 " + std::to_string(version.products.front()) + ")";
                if (ImGui::Selectable(text.c_str(), i == selected_)) {
                    selected_ = i;
                    picked = version.products.front();
                }
            }
            ImGui::EndCombo();
        }
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("重新扫描")) {
        Scan();
        picked = SelectedProduct();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("浏览")) {
        const std::string path = BrowseFeatureFile();
        if (!path.empty()) {
            picked = AddCustom(path);
        }
    }
    return picked;
}

} // namespace anvil::ui
