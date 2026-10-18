#pragma once

struct ImFont;

namespace anvil::ui::theme {

struct Fonts {
    ImFont* body = nullptr;   // 正文字体
    ImFont* title = nullptr;  // 标题
    ImFont* mono = nullptr;   // 等宽，用于 XML 预览
};

// Win10 风格：直角、细边框、浅灰控件
void Apply(float scale);

// 加载微软雅黑 + Consolas；返回的指针全部有效（加载失败时为默认字体）
Fonts LoadFonts(float scale);

} // namespace anvil::ui::theme
