#include "vgui_core.h"
#include <d3dcompiler.h>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

namespace VGUI {
    namespace Core {
        static ID3D11Device* g_Device = nullptr;
        static ID3D11DeviceContext* g_Context = nullptr;
        static ID3D11VertexShader* g_VertexShader = nullptr;
        static ID3D11PixelShader* g_PixelShader = nullptr;
        static ID3D11InputLayout* g_InputLayout = nullptr;
        static ID3D11BlendState* g_BlendState = nullptr;
        static ID3D11RasterizerState* g_RasterizerState = nullptr;
        static int g_WindowWidth = 0;
        static int g_WindowHeight = 0;

        const char* vertexShaderSource = R"(
struct VS_INPUT {
    float3 pos : POSITION;
    float4 col : COLOR;
};
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};
PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos.xy, 0.0f, 1.0f);
    output.col = input.col;
    return output;
}
)";

        const char* pixelShaderSource = R"(
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};
float4 main(PS_INPUT input) : SV_Target {
    return input.col;
}
)";

        void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height) {
            g_Device = device;
            g_Context = context;
            g_WindowWidth = width;
            g_WindowHeight = height;

            ID3DBlob* vsBlob = nullptr;
            ID3DBlob* psBlob = nullptr;
            ID3DBlob* errorBlob = nullptr;

            D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr,
                "main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
            D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr,
                "main", "ps_5_0", 0, 0, &psBlob, &errorBlob);

            g_Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_VertexShader);
            g_Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_PixelShader);

            D3D11_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            g_Device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_InputLayout);

            vsBlob->Release();
            psBlob->Release();

            D3D11_BLEND_DESC blendDesc = {};
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            g_Device->CreateBlendState(&blendDesc, &g_BlendState);

            D3D11_RASTERIZER_DESC rastDesc = {};
            rastDesc.FillMode = D3D11_FILL_SOLID;
            rastDesc.CullMode = D3D11_CULL_NONE;
            g_Device->CreateRasterizerState(&rastDesc, &g_RasterizerState);
        }

        void SetWindowSize(int width, int height) {
            g_WindowWidth = width;
            g_WindowHeight = height;
        }

        void Cleanup() {
            if (g_InputLayout) { g_InputLayout->Release(); g_InputLayout = nullptr; }
            if (g_VertexShader) { g_VertexShader->Release(); g_VertexShader = nullptr; }
            if (g_PixelShader) { g_PixelShader->Release(); g_PixelShader = nullptr; }
            if (g_BlendState) { g_BlendState->Release(); g_BlendState = nullptr; }
            if (g_RasterizerState) { g_RasterizerState->Release(); g_RasterizerState = nullptr; }
        }

        ID3D11Device* GetDevice() {
            return g_Device;
        }

        ID3D11DeviceContext* GetContext() {
            return g_Context;
        }

        void GetWindowSize(int& width, int& height) {
            width = g_WindowWidth;
            height = g_WindowHeight;
        }

        ID3D11InputLayout* GetInputLayout() {
            return g_InputLayout;
        }

        ID3D11VertexShader* GetVertexShader() {
            return g_VertexShader;
        }

        ID3D11PixelShader* GetPixelShader() {
            return g_PixelShader;
        }

        ID3D11BlendState* GetBlendState() {
            return g_BlendState;
        }

        ID3D11RasterizerState* GetRasterizerState() {
            return g_RasterizerState;
        }
    }
}