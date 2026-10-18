#include "ui/app.h"

#include "core/license.h"
#include "core/machine_id.h"
#include "core/serial.h"
#include "ui/gfx.h"
#include "ui/theme.h"
#include "ui/xml_pretty.h"

#include <imgui.h>

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace anvil::ui {
namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 620;
constexpr float kLabelOffset = 84.0f;

std::string ExeDirectory() {
    char path[MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) == 0) {
        return ".";
    }
    std::string full(path);
    const std::size_t pos = full.find_last_of("\\/");
    return pos == std::string::npos ? std::string(".") : full.substr(0, pos);
}

void Section(const char* text) {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.00f, 0.41f, 0.72f, 1.0f), "%s", text);
    ImGui::Separator();
}

class App {
public:
    bool Initialize() {
        if (!gfx_.Init(L"Anvil", kWindowWidth, kWindowHeight)) {
            return false;
        }
        fonts_ = theme::LoadFonts(gfx_.Scale());
        RefreshMachine();
        std::snprintf(serial_, sizeof(serial_), "%s", core::Serial::Random().c_str());
        status_ = "就绪";
        return true;
    }

    bool NewFrame() { return gfx_.NewFrame(); }

    void Present() {
        gfx_.Render();
        gfx_.Present();
    }

    void Frame() {
        // 自动化冒烟测试：ANVIL_SELFTEST=1 生成后退出；=run 只生成、界面继续留着
        if (const char* selftest = std::getenv("ANVIL_SELFTEST"); selftest != nullptr && ++frames_ == 30) {
            Generate();
            std::fprintf(stderr, "[selftest] %s\n", status_.c_str());
            if (std::strcmp(selftest, "run") != 0) {
                gfx_.RequestExit();
            }
        }

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("##root", nullptr, flags)) {
            DrawHeader();
            const float left_width = 430.0f * gfx_.Scale();
            ImGui::BeginChild("##left", ImVec2(left_width, 0.0f), ImGuiChildFlags_None);
            DrawMachineInfo();
            DrawParameters();
            DrawActions();
            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("##right", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
            DrawPreview();
            ImGui::EndChild();
        }
        ImGui::End();
    }

private:
    void DrawHeader() {
        if (fonts_.title != nullptr) {
            ImGui::PushFont(fonts_.title);
        }
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.00f, 0.30f, 0.55f, 1.0f));
        ImGui::TextUnformatted("Anvil");
        ImGui::PopStyleColor();
        if (fonts_.title != nullptr) {
            ImGui::PopFont();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 7.0f * gfx_.Scale());
        ImGui::TextDisabled("授权文件生成器");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    void DrawMachineInfo() {
        Section("本机信息");

        ImGui::TextDisabled("特征码");
        ImGui::SameLine(kLabelOffset * gfx_.Scale());
        ImGui::TextUnformatted(machine_code_.empty() ? "读取失败" : machine_code_.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("复制##mc")) {
            ImGui::SetClipboardText(machine_code_.c_str());
        }

        ImGui::TextDisabled("CPU");
        ImGui::SameLine(kLabelOffset * gfx_.Scale());
        ImGui::TextWrapped("%s", cpu_.empty() ? "-" : cpu_.c_str());

        ImGui::TextDisabled("网卡");
        ImGui::SameLine(kLabelOffset * gfx_.Scale());
        ImGui::TextWrapped("%s", macs_.c_str());
    }

    void DrawParameters() {
        Section("授权参数");
        const float input_width = 250.0f * gfx_.Scale();

        ImGui::TextDisabled("产品号");
        ImGui::SameLine(kLabelOffset * gfx_.Scale());
        ImGui::SetNextItemWidth(input_width);
        ImGui::InputText("##product", product_, sizeof(product_), ImGuiInputTextFlags_CharsDecimal);

        ImGui::TextDisabled("序列号");
        ImGui::SameLine(kLabelOffset * gfx_.Scale());
        ImGui::SetNextItemWidth(input_width);
        ImGui::InputText("##serial", serial_, sizeof(serial_));

        ImGui::TextDisabled("学生数");
        ImGui::SameLine(kLabelOffset * gfx_.Scale());
        ImGui::SetNextItemWidth(input_width);
        ImGui::InputText("##students", students_, sizeof(students_), ImGuiInputTextFlags_CharsDecimal);

        ImGui::TextDisabled("到期");
        ImGui::SameLine(kLabelOffset * gfx_.Scale());
        ImGui::SetNextItemWidth(input_width);
        ImGui::InputText("##period", period_, sizeof(period_));
    }

    void DrawActions() {
        ImGui::Spacing();
        if (ImGui::Button("随机序列号")) {
            std::snprintf(serial_, sizeof(serial_), "%s", core::Serial::Random().c_str());
            status_ = "已生成序列号 " + core::Serial::Pretty(serial_);
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.42f, 0.85f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.50f, 0.95f, 1.0f));
        if (ImGui::Button("生成授权文件")) {
            Generate();
        }
        ImGui::PopStyleColor(2);

        ImGui::Spacing();
        ImGui::TextWrapped("%s", status_.c_str());
    }

    void DrawPreview() {
        Section("明文预览");
        if (xml_.empty()) {
            ImGui::TextDisabled("点击「生成授权文件」后在这里预览");
            return;
        }
        if (ImGui::SmallButton("复制 XML")) {
            ImGui::SetClipboardText(xml_.c_str());
        }

        const float pad = 10.0f * gfx_.Scale();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.99f, 0.99f, 0.99f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(pad, pad));
        if (ImGui::BeginChild("##previewbox", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders)) {
            if (fonts_.mono != nullptr) {
                ImGui::PushFont(fonts_.mono);
            }
            // 关键：给 TextUnformatted 一个换行边界，长行才会自动折行（InputTextMultiline 不支持换行）
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(xml_view_.empty() ? xml_.c_str() : xml_view_.c_str());
            ImGui::PopTextWrapPos();
            if (fonts_.mono != nullptr) {
                ImGui::PopFont();
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    void Generate() {
        if (machine_code_.empty()) {
            status_ = "机器特征码为空，无法生成";
            return;
        }
        core::LicenseSpec spec;
        spec.machine_code = machine_code_;
        spec.serial = core::Serial::Normalize(serial_);
        if (!core::Serial::IsValid(spec.serial)) {
            status_ = "序列号必须是 20 位字母数字（不带横杠）";
            return;
        }
        if (const int product = std::atoi(product_); product > 0) {
            spec.product = product;
            spec.subver = product;
        }
        if (const int students = std::atoi(students_); students > 0) {
            spec.eclass_value = students;
        }
        if (period_[0] != '\0') {
            spec.period = period_;
        }

        const std::string path = ExeDirectory() + "/" + spec.serial + ".lic";
        std::string error;
        if (!core::License::Write(spec, path, &error)) {
            status_ = "生成失败：" + error;
            return;
        }
        std::snprintf(serial_, sizeof(serial_), "%s", spec.serial.c_str());
        xml_ = core::License::BuildXml(spec);
        xml_view_ = xml_pretty::Format(xml_);
        status_ = "已生成 " + path;
    }

    void RefreshMachine() {
        machine_code_ = core::MachineId::Local(&cpu_);
        const std::vector<std::string> macs = core::MachineId::MacAddresses();
        macs_.clear();
        for (const std::string& mac : macs) {
            if (!macs_.empty()) {
                macs_ += "  ";
            }
            macs_ += mac;
        }
        if (macs_.empty()) {
            macs_ = "未检测到物理网卡";
        }
    }

    Gfx gfx_;
    theme::Fonts fonts_;
    int frames_ = 0;
    std::string cpu_;
    std::string macs_;
    std::string machine_code_;
    std::string status_;
    std::string xml_;
    std::string xml_view_;
    char product_[16] = "10652";
    char serial_[64] = "";
    char students_[16] = "1";
    char period_[40] = "2099-12-31 23:59:59";
};

} // namespace

int Run() {
    App app;
    if (!app.Initialize()) {
        return 1;
    }
    while (app.NewFrame()) {
        app.Frame();
        app.Present();
    }
    return 0;
}

} // namespace anvil::ui
