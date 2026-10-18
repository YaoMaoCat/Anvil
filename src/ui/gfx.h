#pragma once

#include <windows.h>

namespace anvil::ui {

// 图形后端：Win32 窗口 + D3D11 交换链 + ImGui 帧循环
class Gfx {
public:
    Gfx() = default;
    ~Gfx();
    Gfx(const Gfx&) = delete;
    Gfx& operator=(const Gfx&) = delete;

    // width/height 为逻辑尺寸，内部按屏幕 DPI 换算成物理像素
    bool Init(const wchar_t* title, int width, int height);
    void Shutdown();

    // 处理消息并开始新的一帧，返回 false 表示窗口已关闭
    bool NewFrame();
    void Render();
    void Present();
    void OnResize(int width, int height);
    void RequestExit();

    float Scale() const { return scale_; }

private:
    void CreateRenderTarget();
    void DestroyRenderTarget();

    HWND hwnd_ = nullptr;
    void* device_ = nullptr;
    void* context_ = nullptr;
    void* swap_chain_ = nullptr;
    void* render_target_ = nullptr;
    float scale_ = 1.0f;
    bool running_ = true;
};

} // namespace anvil::ui
