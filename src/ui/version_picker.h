#pragma once

#include <string>
#include <vector>

namespace anvil::ui {

// 一个已安装的极域版本：目录里的 Feature.dat 决定它的产品号
struct InstalledVersion {
    std::string directory;   // 安装目录（UTF-8）
    std::string label;       // 下拉框显示名，取目录名
    std::vector<int> products;
    int trail_days = 0;
};

// 版本选择：扫描所有已装版本，让用户在下拉框里挑一个
class VersionPicker {
public:
    void Scan();

    bool empty() const { return versions_.empty(); }

    // 画下拉框；返回本帧新选中的产品号（没变化或没选中返回 0）
    int DrawCombo(float scale);

    // 手动加入一个 Feature.dat（浏览选择）；成功返回它的产品号
    int AddCustom(const std::string& path);

    // 初始化时用：第一个版本的主产品号
    int InitialProduct() const;

    // 当前选中版本的产品号（无则为 0）
    int SelectedProduct() const;

private:
    std::vector<InstalledVersion> versions_;
    int selected_ = 0;
};

} // namespace anvil::ui
