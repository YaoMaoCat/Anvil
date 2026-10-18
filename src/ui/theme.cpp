#include "ui/theme.h"

#include <imgui.h>

#include <algorithm>

namespace anvil::ui::theme {
namespace {

const char* kCjkFont = "C:/Windows/Fonts/msyh.ttc";
const char* kMonoFont = "C:/Windows/Fonts/consola.ttf";

} // namespace

void Apply(float scale) {
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();

    // Win10 观感的核心：全部直角 + 1px 细边框
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f * scale;
    style.FrameBorderSize = 1.0f * scale;
    style.PopupBorderSize = 1.0f * scale;
    style.WindowPadding = ImVec2(14.0f * scale, 12.0f * scale);
    style.FramePadding = ImVec2(8.0f * scale, 5.0f * scale);
    style.ItemSpacing = ImVec2(8.0f * scale, 7.0f * scale);
    style.ItemInnerSpacing = ImVec2(6.0f * scale, 4.0f * scale);
    style.ScrollbarSize = 14.0f * scale;
    style.GrabMinSize = 10.0f * scale;

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
    c[ImGuiCol_WindowBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    c[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    c[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    c[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    c[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.95f, 0.99f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.80f, 0.90f, 0.98f, 1.00f);
    c[ImGuiCol_Button] = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.90f, 0.94f, 0.98f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.88f, 0.95f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.90f, 0.94f, 0.98f, 1.00f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.80f, 0.88f, 0.95f, 1.00f);
    c[ImGuiCol_Separator] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.97f, 0.97f, 0.97f, 1.00f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    c[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.47f, 0.84f, 0.35f);
    c[ImGuiCol_NavHighlight] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
}

Fonts LoadFonts(float scale) {
    ImGuiIO& io = ImGui::GetIO();
    const ImWchar* ranges = io.Fonts->GetGlyphRangesChineseSimplifiedCommon();

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 2;

    Fonts fonts;
    fonts.body = io.Fonts->AddFontFromFileTTF(kCjkFont, 16.0f * scale, &config, ranges);
    if (fonts.body == nullptr) {
        fonts.body = io.Fonts->AddFontDefault();
    }
    fonts.title = io.Fonts->AddFontFromFileTTF(kCjkFont, 24.0f * scale, &config, ranges);
    if (fonts.title == nullptr) {
        fonts.title = fonts.body;
    }

    fonts.mono = io.Fonts->AddFontFromFileTTF(kMonoFont, 14.0f * scale, &config, io.Fonts->GetGlyphRangesDefault());
    if (fonts.mono != nullptr) {
        // 等宽字体没中文，用雅黑补齐，否则中文注释变方块
        ImFontConfig merge;
        merge.MergeMode = true;
        merge.OversampleH = 2;
        merge.OversampleV = 2;
        io.Fonts->AddFontFromFileTTF(kCjkFont, 14.0f * scale, &merge, ranges);
    }
    return fonts;
}

} // namespace anvil::ui::theme
