#include "ui/app.h"

#include "core/license.h"
#include "core/machine_id.h"
#include "core/serial.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi_Backend.h>

#include <windows.h>

#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace anvil::ui {
namespace {

constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 640;

std::string ExeDirectory() {
    char path[MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) == 0) {
        return ".";
    }
    std::string full(path);
    const std::size_t pos = full.find_last_of("\\/");
    return pos == std::string::npos ? std::string(".") : full.substr(0, pos);
}

std::string EscapeRml(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 16);
    for (char c : text) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out.push_back(c); break;
        }
    }
    return out;
}

class ActionListener final : public Rml::EventListener {
public:
    explicit ActionListener(std::function<void()> action) : action_(std::move(action)) {}
    void ProcessEvent(Rml::Event& /*event*/) override {
        if (action_) {
            action_();
        }
    }

private:
    std::function<void()> action_;
};

class App {
public:
    bool Initialize() {
        if (!Backend::Initialize("Anvil", kWindowWidth, kWindowHeight, false)) {
            return false;
        }

        Rml::SetSystemInterface(Backend::GetSystemInterface());
        Rml::SetRenderInterface(Backend::GetRenderInterface());
        if (!Rml::Initialise()) {
            return false;
        }
        initialised_ = true;

        Rml::LoadFontFace("C:/Windows/Fonts/msyh.ttc", true);
        Rml::LoadFontFace("C:/Windows/Fonts/simhei.ttf", true);

        context_ = Rml::CreateContext("main", Rml::Vector2i(kWindowWidth, kWindowHeight));
        if (context_ == nullptr) {
            return false;
        }

        const std::string document_path = ExeDirectory() + "/assets/main.rml";
        document_ = context_->LoadDocument(document_path);
        if (document_ == nullptr) {
            return false;
        }
        document_->Show();

        Bind();
        RefreshMachine();
        NewSerial();
        return true;
    }

    void Run() {
        while (Backend::ProcessEvents(context_, nullptr)) {
            Backend::BeginFrame();
            context_->Update();
            context_->Render();
            Backend::PresentFrame();
        }
    }

    void Shutdown() {
        if (document_ != nullptr) {
            document_->Close();
            document_ = nullptr;
        }
        if (initialised_) {
            Rml::Shutdown();
            initialised_ = false;
        }
        Backend::Shutdown();
    }

private:
    Rml::Element* Find(const char* id) const {
        return document_ != nullptr ? document_->GetElementById(id) : nullptr;
    }

    std::string ReadInput(const char* id) const {
        Rml::Element* element = Find(id);
        if (element == nullptr) {
            return {};
        }
        auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(element);
        return input != nullptr ? input->GetValue() : std::string{};
    }

    void SetText(const char* id, const std::string& text) const {
        if (Rml::Element* element = Find(id); element != nullptr) {
            element->SetInnerRML(EscapeRml(text));
        }
    }

    void SetInput(const char* id, const std::string& text) const {
        Rml::Element* element = Find(id);
        if (element == nullptr) {
            return;
        }
        if (auto* input = rmlui_dynamic_cast<Rml::ElementFormControlInput*>(element); input != nullptr) {
            input->SetValue(text);
        }
    }

    void OnClick(const char* id, std::function<void()> action) {
        if (Rml::Element* element = Find(id); element != nullptr) {
            auto listener = std::make_unique<ActionListener>(std::move(action));
            element->AddEventListener("click", listener.get());
            listeners_.push_back(std::move(listener));
        }
    }

    void Bind() {
        OnClick("btn-serial", [this] { NewSerial(); });
        OnClick("btn-generate", [this] { Generate(); });
    }

    void RefreshMachine() {
        machine_code_ = core::MachineId::Local(&cpu_);
        macs_ = core::MachineId::MacAddresses();

        std::string mac_text;
        for (const auto& mac : macs_) {
            if (!mac_text.empty()) {
                mac_text += "  ";
            }
            mac_text += mac;
        }
        if (mac_text.empty()) {
            mac_text = "未检测到物理网卡";
        }

        SetText("mc", machine_code_.empty() ? "读取失败" : machine_code_);
        SetText("cpu", cpu_.empty() ? "—" : cpu_);
        SetText("macs", mac_text);

        if (machine_code_.empty()) {
            SetText("status", "无法读取机器特征码，请检查网卡状态");
        }
    }

    void NewSerial() {
        const std::string serial = core::Serial::Random();
        SetInput("in-serial", serial);
        SetText("status", "已生成序列号 " + core::Serial::Pretty(serial));
    }

    void Generate() {
        if (machine_code_.empty()) {
            SetText("status", "机器特征码为空，无法生成");
            return;
        }

        core::LicenseSpec spec;
        spec.machine_code = machine_code_;
        spec.serial = core::Serial::Normalize(ReadInput("in-serial"));
        if (spec.serial.empty()) {
            spec.serial = core::Serial::Random();
        }

        const std::string product_text = ReadInput("in-product");
        if (!product_text.empty()) {
            spec.product = std::atoi(product_text.c_str());
            spec.subver = spec.product;
        }

        const std::string students_text = ReadInput("in-students");
        if (!students_text.empty()) {
            spec.eclass_value = std::atoi(students_text.c_str());
        }

        const std::string period_text = ReadInput("in-period");
        if (!period_text.empty()) {
            spec.period = period_text;
        }

        if (!core::Serial::IsValid(spec.serial)) {
            SetText("status", "序列号长度必须为 20 位（不带横杠）");
            return;
        }

        const std::string path = ExeDirectory() + "/" + spec.serial + ".lic";
        std::string error;
        if (!core::License::Write(spec, path, &error)) {
            SetText("status", "生成失败：" + error);
            return;
        }

        if (Rml::Element* preview = Find("preview"); preview != nullptr) {
            preview->SetInnerRML(EscapeRml(core::License::BuildXml(spec)));
        }
        SetInput("in-serial", spec.serial);
        SetText("status", "已生成 " + path + "（" + std::to_string(spec.eclass_value) + " 学生席位）");
    }

    Rml::Context* context_ = nullptr;
    Rml::ElementDocument* document_ = nullptr;
    std::vector<std::unique_ptr<ActionListener>> listeners_;
    std::string cpu_;
    std::vector<std::string> macs_;
    std::string machine_code_;
    bool initialised_ = false;
};

} // namespace

int Run() {
    App app;
    if (!app.Initialize()) {
        return 1;
    }
    app.Run();
    app.Shutdown();
    return 0;
}

} // namespace anvil::ui
