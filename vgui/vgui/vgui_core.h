#pragma once
#include <d3d11.h>

namespace VGUI {
    namespace Core {
        void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);
        void SetWindowSize(int width, int height);
        void Cleanup();

        // Internal access
        ID3D11Device* GetDevice();
        ID3D11DeviceContext* GetContext();
        void GetWindowSize(int& width, int& height);

        // Shader pipeline access
        ID3D11InputLayout* GetInputLayout();
        ID3D11VertexShader* GetVertexShader();
        ID3D11PixelShader* GetPixelShader();
        ID3D11BlendState* GetBlendState();
        ID3D11RasterizerState* GetRasterizerState();
    }
}