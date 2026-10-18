#include "ui/gfx.h"

#include "ui/theme.h"

#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <windows.h>

#include <algorithm>
#include <cstdio>

// ImGui 的 win32 后端自带消息处理，声明在此即可
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace anvil::ui {
namespace {

HWND g_hwnd = nullptr;
bool g_closed = false;

ID3D11Device* Device(void* p) { return static_cast<ID3D11Device*>(p); }
ID3D11DeviceContext* Context(void* p) { return static_cast<ID3D11DeviceContext*>(p); }
IDXGISwapChain* SwapChain(void* p) { return static_cast<IDXGISwapChain*>(p); }
ID3D11RenderTargetView* RenderTarget(void* p) { return static_cast<ID3D11RenderTargetView*>(p); }

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return true;
    }
    switch (msg) {
    case WM_SIZE:
        if (wparam != SIZE_MINIMIZED && g_hwnd != nullptr) {
            auto* gfx = reinterpret_cast<Gfx*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (gfx != nullptr) {
                gfx->OnResize(LOWORD(lparam), HIWORD(lparam));
            }
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wparam & 0xfff0) == SC_KEYMENU) {
            return 0; // 屏蔽 Alt 菜单
        }
        break;
    case WM_CLOSE:
        g_closed = true;
        return 0;
    case WM_DESTROY:
        if (hwnd == g_hwnd) {
            PostQuitMessage(0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

} // namespace

Gfx::~Gfx() {
    Shutdown();
}

bool Gfx::Init(const wchar_t* title, int width, int height) {
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)); // IDC_ARROW
    wc.lpszClassName = L"AnvilWindow";
    RegisterClassExW(&wc);

    // 系统 DPI：不需要临时窗口（临时窗口销毁时会给消息队列塞 WM_QUIT）
    HDC screen_dc = GetDC(nullptr);
    const int dpi = screen_dc != nullptr ? GetDeviceCaps(screen_dc, LOGPIXELSX) : 96;
    if (screen_dc != nullptr) {
        ReleaseDC(nullptr, screen_dc);
    }
    scale_ = static_cast<float>(dpi) / 96.0f;
    if (scale_ <= 0.0f) {
        scale_ = 1.0f;
    }

    const int pixel_w = std::min(static_cast<int>(width * scale_),
                                 static_cast<int>(GetSystemMetrics(SM_CXSCREEN) * 0.92f));
    const int pixel_h = std::min(static_cast<int>(height * scale_),
                                 static_cast<int>(GetSystemMetrics(SM_CYSCREEN) * 0.86f));
    RECT rect = {0, 0, pixel_w, pixel_h};
    AdjustWindowRectExForDpi(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0, static_cast<UINT>(96 * scale_));
    const int win_w = rect.right - rect.left;
    const int win_h = rect.bottom - rect.top;
    const int pos_x = (GetSystemMetrics(SM_CXSCREEN) - win_w) / 2;
    const int pos_y = (GetSystemMetrics(SM_CYSCREEN) - win_h) / 2;

    hwnd_ = CreateWindowExW(0, wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, pos_x, pos_y, win_w, win_h,
                            nullptr, nullptr, wc.hInstance, nullptr);
    if (hwnd_ == nullptr) {
        return false;
    }
    g_hwnd = hwnd_;
    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferCount = 2;
    desc.BufferDesc.Width = pixel_w;
    desc.BufferDesc.Height = pixel_h;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd_;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL obtained = D3D_FEATURE_LEVEL_11_0;
    const HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2,
                                                     D3D11_SDK_VERSION, &desc, &swap_chain, &device,
                                                     &obtained, &context);
    if (FAILED(hr)) {
        return false;
    }
    device_ = device;
    context_ = context;
    swap_chain_ = swap_chain;

    CreateRenderTarget();
    ShowWindow(hwnd_, SW_SHOWDEFAULT);
    UpdateWindow(hwnd_);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    theme::Apply(scale_);

    ImGui_ImplWin32_Init(hwnd_);
    if (!ImGui_ImplDX11_Init(device, context)) {
        return false;
    }
    return true;
}

void Gfx::OnResize(int width, int height) {
    if (swap_chain_ == nullptr || width <= 0 || height <= 0) {
        return;
    }
    DestroyRenderTarget();
    if (FAILED(SwapChain(swap_chain_)->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height),
                                                     DXGI_FORMAT_UNKNOWN, 0))) {
        return;
    }
    CreateRenderTarget();
}

void Gfx::CreateRenderTarget() {
    if (swap_chain_ == nullptr) {
        return;
    }
    ID3D11Texture2D* back_buffer = nullptr;
    if (FAILED(SwapChain(swap_chain_)->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
        return;
    }
    ID3D11RenderTargetView* view = nullptr;
    Device(device_)->CreateRenderTargetView(back_buffer, nullptr, &view);
    back_buffer->Release();
    render_target_ = view;
}

void Gfx::DestroyRenderTarget() {
    if (render_target_ != nullptr) {
        RenderTarget(render_target_)->Release();
        render_target_ = nullptr;
    }
}

bool Gfx::NewFrame() {
    if (g_closed) {
        return false;
    }
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) {
            return false;
        }
    }
    if (g_closed) {
        return false;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    running_ = true;
    return running_;
}

void Gfx::Render() {
    ImGui::Render();
    const ImVec4 clear(0.10f, 0.11f, 0.13f, 1.0f);
    const float color[4] = {clear.x, clear.y, clear.z, clear.w};
    Context(context_)->OMSetRenderTargets(1, reinterpret_cast<ID3D11RenderTargetView* const*>(&render_target_), nullptr);
    Context(context_)->ClearRenderTargetView(RenderTarget(render_target_), color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Gfx::RequestExit() {
    if (hwnd_ != nullptr) {
        PostMessageW(hwnd_, WM_CLOSE, 0, 0);
    }
}

void Gfx::Present() {
    SwapChain(swap_chain_)->Present(1, 0);
}

void Gfx::Shutdown() {
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
    DestroyRenderTarget();
    if (swap_chain_ != nullptr) {
        SwapChain(swap_chain_)->Release();
        swap_chain_ = nullptr;
    }
    if (context_ != nullptr) {
        Context(context_)->Release();
        context_ = nullptr;
    }
    if (device_ != nullptr) {
        Device(device_)->Release();
        device_ = nullptr;
    }
    if (hwnd_ != nullptr) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
        g_hwnd = nullptr;
    }
    UnregisterClassW(L"AnvilWindow", GetModuleHandleW(nullptr));
}

} // namespace anvil::ui
