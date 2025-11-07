// hey! for documentation, read *instruction.md*


#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include "vgui/vgui_core.h"
#include "vgui/vgui_draw.h"
#include <math.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

// DirectX globals
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

static int g_WindowWidth = 0;
static int g_WindowHeight = 0;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND FindNotepadWindow() {
    return FindWindowA("Notepad", nullptr); // whatever window youd like
}

bool GetWindowRect(HWND hWnd, int& x, int& y, int& width, int& height) {
    RECT rect;
    if (::GetWindowRect(hWnd, &rect)) {
        x = rect.left;
        y = rect.top;
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
        return true;
    }
    return false;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    HWND hNotepad = FindNotepadWindow();
    if (!hNotepad) {
        MessageBoxA(nullptr, "Notepad.exe not found! Please open Notepad first.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    int x, y, width, height;
    if (!GetWindowRect(hNotepad, x, y, width, height)) {
        MessageBoxA(nullptr, "Failed to get Notepad window position!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    g_WindowWidth = width;
    g_WindowHeight = height;

    WNDCLASSEXA wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, "OverlayClass", nullptr };
    RegisterClassExA(&wc);

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        wc.lpszClassName,
        "Overlay",
        WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA | LWA_COLORKEY);

    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Initialize VGUI
    VGUI::Core::Initialize(g_pd3dDevice, g_pd3dDeviceContext, width, height);

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    bool done = false;
    float animTime = 0.0f;

    while (!done) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        HWND hNotepadCurrent = FindNotepadWindow();
        if (hNotepadCurrent) {
            int nx, ny, nw, nh;
            if (GetWindowRect(hNotepadCurrent, nx, ny, nw, nh)) {
                SetWindowPos(hwnd, HWND_TOPMOST, nx, ny, nw, nh, SWP_NOACTIVATE);
                g_WindowWidth = nw;
                g_WindowHeight = nh;
                VGUI::Core::SetWindowSize(nw, nh);
            }
        }
        else {
            done = true;
        }

        const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.01f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);

        D3D11_VIEWPORT vp;
        vp.Width = (FLOAT)g_WindowWidth;
        vp.Height = (FLOAT)g_WindowHeight;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        g_pd3dDeviceContext->RSSetViewports(1, &vp);

        animTime += 0.016f;

        // Main panel with rounded corners
        float panelX = 50;
        float panelY = 50;
        float panelW = 400;
        float panelH = 300;

        VGUI::Draw::DrawFilledRoundedRect(panelX, panelY, panelW, panelH, 15, 0.15f, 0.15f, 0.2f, 0.95f);
        VGUI::Draw::DrawRectThick(panelX, panelY, panelW, panelH, 2.0f, 0.4f, 0.8f, 1.0f, 1.0f);

        // Title bar gradient
        VGUI::Draw::DrawGradientRect(panelX, panelY, panelW, 40,
            0.3f, 0.6f, 0.9f, 1.0f,
            0.2f, 0.4f, 0.7f, 1.0f,
            true);

        // Animated circle
        float circleX = panelX + panelW / 2;
        float circleY = panelY + 150;
        float circleRadius = 30 + sinf(animTime * 2.0f) * 10;
        VGUI::Draw::DrawFilledCircle(circleX, circleY, circleRadius, 32, 1.0f, 0.3f, 0.3f, 0.9f);
        VGUI::Draw::DrawCircle(circleX, circleY, circleRadius + 5, 32, 1.0f, 1.0f, 0.0f, 1.0f);

        // Bezier curve showcase
        float bezStartX = panelX + 20;
        float bezStartY = panelY + 80;
        float bezEndX = panelX + panelW - 20;
        float bezEndY = panelY + 80;
        float ctrlOffset = sinf(animTime) * 50;

        VGUI::Draw::DrawBezierCurve(
            bezStartX, bezStartY,
            bezStartX + 100, bezStartY - 50 + ctrlOffset,
            bezEndX - 100, bezStartY + 50 - ctrlOffset,
            bezEndX, bezEndY,
            32, 0.0f, 1.0f, 1.0f, 1.0f
        );

        // Thick line test
        VGUI::Draw::DrawThickLine(panelX + 20, panelY + panelH - 40, panelX + panelW - 20, panelY + panelH - 40,
            4.0f, 0.2f, 1.0f, 0.2f, 1.0f);

        // Triangle
        float triX = panelX + panelW - 80;
        float triY = panelY + panelH - 80;
        VGUI::Draw::DrawFilledTriangle(triX, triY, triX + 30, triY + 40, triX - 30, triY + 40,
            1.0f, 0.5f, 0.0f, 0.8f);

        // Polygon (pentagon)
        float polyPoints[10] = {
            panelX + 80, panelY + panelH - 30,
            panelX + 100, panelY + panelH - 50,
            panelX + 90, panelY + panelH - 70,
            panelX + 70, panelY + panelH - 70,
            panelX + 60, panelY + panelH - 50
        };
        VGUI::Draw::DrawFilledPolygon(polyPoints, 5, 0.8f, 0.2f, 0.8f, 0.9f);

        // Status indicators
        for (int i = 0; i < 5; i++) {
            float indicatorX = panelX + 20 + i * 30;
            float indicatorY = panelY + 20;
            float phase = animTime * 3.0f - i * 0.5f;
            float brightness = (sinf(phase) + 1.0f) * 0.5f;
            VGUI::Draw::DrawFilledCircle(indicatorX, indicatorY, 8, 16, 0.2f + brightness * 0.8f, 0.8f, 0.2f, 1.0f);
        }

        // Render everything
        VGUI::Draw::Render();

        g_pSwapChain->Present(1, 0);
    }

    VGUI::Core::Cleanup();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassA(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice,
        &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}