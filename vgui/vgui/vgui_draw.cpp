#include "vgui_draw.h"
#include "vgui_core.h"
#include <d3d11.h>
#include <vector>
#include <cmath>

namespace VGUI {
    namespace Draw {
        struct Vertex {
            float x, y, z;
            float r, g, b, a;
        };

        enum class DrawCommandType {
            Lines,
            Triangles,
            TriangleStrip,
            LineStrip
        };

        struct DrawCommand {
            DrawCommandType type;
            size_t vertexStart;
            size_t vertexCount;
            bool antiAlias;
        };

        static std::vector<Vertex> g_VertexBuffer;
        static std::vector<DrawCommand> g_CommandBuffer;
        static ID3D11Buffer* g_D3DVertexBuffer = nullptr;
        static bool g_AntiAliasEnabled = true;
        static float g_GlobalAlpha = 1.0f;

        // Advanced vertex manipulation
        void ToNDC(float x, float y, float& ndcX, float& ndcY) {
            int width, height;
            Core::GetWindowSize(width, height);
            ndcX = (x / static_cast<float>(width)) * 2.0f - 1.0f;
            ndcY = 1.0f - (y / static_cast<float>(height)) * 2.0f;
        }

        inline void AddVertex(float x, float y, float r, float g, float b, float a) {
            float ndcX, ndcY;
            ToNDC(x, y, ndcX, ndcY);
            g_VertexBuffer.push_back({ ndcX, ndcY, 0.0f, r, g, b, a * g_GlobalAlpha });
        }

        void SetGlobalAlpha(float alpha) {
            g_GlobalAlpha = (alpha < 0.0f) ? 0.0f : (alpha > 1.0f) ? 1.0f : alpha;
        }

        void EnableAntiAliasing(bool enable) {
            g_AntiAliasEnabled = enable;
        }

        // Basic primitives
        void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a) {
            size_t startIdx = g_VertexBuffer.size();
            AddVertex(x1, y1, r, g, b, a);
            AddVertex(x2, y2, r, g, b, a);
            g_CommandBuffer.push_back({ DrawCommandType::Lines, startIdx, 2, g_AntiAliasEnabled });
        }

        void DrawThickLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a) {
            float dx = x2 - x1;
            float dy = y2 - y1;
            float len = sqrtf(dx * dx + dy * dy);
            if (len < 0.001f) return;

            float nx = -dy / len;
            float ny = dx / len;
            float halfThick = thickness * 0.5f;

            float ox1 = nx * halfThick;
            float oy1 = ny * halfThick;

            size_t startIdx = g_VertexBuffer.size();

            // Create quad as two triangles
            AddVertex(x1 - ox1, y1 - oy1, r, g, b, a);
            AddVertex(x1 + ox1, y1 + oy1, r, g, b, a);
            AddVertex(x2 + ox1, y2 + oy1, r, g, b, a);

            AddVertex(x1 - ox1, y1 - oy1, r, g, b, a);
            AddVertex(x2 + ox1, y2 + oy1, r, g, b, a);
            AddVertex(x2 - ox1, y2 - oy1, r, g, b, a);

            g_CommandBuffer.push_back({ DrawCommandType::Triangles, startIdx, 6, false });
        }

        void DrawRect(float x, float y, float w, float h, float r, float g, float b, float a) {
            DrawLine(x, y, x + w, y, r, g, b, a);
            DrawLine(x + w, y, x + w, y + h, r, g, b, a);
            DrawLine(x + w, y + h, x, y + h, r, g, b, a);
            DrawLine(x, y + h, x, y, r, g, b, a);
        }

        void DrawRectThick(float x, float y, float w, float h, float thickness, float r, float g, float b, float a) {
            DrawThickLine(x, y, x + w, y, thickness, r, g, b, a);
            DrawThickLine(x + w, y, x + w, y + h, thickness, r, g, b, a);
            DrawThickLine(x + w, y + h, x, y + h, thickness, r, g, b, a);
            DrawThickLine(x, y + h, x, y, thickness, r, g, b, a);
        }

        void DrawFilledRect(float x, float y, float w, float h, float r, float g, float b, float a) {
            float ndcX1, ndcY1, ndcX2, ndcY2;
            ToNDC(x, y, ndcX1, ndcY1);
            ToNDC(x + w, y + h, ndcX2, ndcY2);

            size_t startIdx = g_VertexBuffer.size();

            // Triangle 1
            g_VertexBuffer.push_back({ ndcX1, ndcY1, 0.0f, r, g, b, a * g_GlobalAlpha });
            g_VertexBuffer.push_back({ ndcX2, ndcY1, 0.0f, r, g, b, a * g_GlobalAlpha });
            g_VertexBuffer.push_back({ ndcX2, ndcY2, 0.0f, r, g, b, a * g_GlobalAlpha });

            // Triangle 2
            g_VertexBuffer.push_back({ ndcX1, ndcY1, 0.0f, r, g, b, a * g_GlobalAlpha });
            g_VertexBuffer.push_back({ ndcX2, ndcY2, 0.0f, r, g, b, a * g_GlobalAlpha });
            g_VertexBuffer.push_back({ ndcX1, ndcY2, 0.0f, r, g, b, a * g_GlobalAlpha });

            g_CommandBuffer.push_back({ DrawCommandType::Triangles, startIdx, 6, false });
        }

        void DrawRoundedRect(float x, float y, float w, float h, float radius, float r, float g, float b, float a) {
            float minDim = (w < h) ? w : h;
            float maxRadius = minDim * 0.5f;
            if (radius > maxRadius) radius = maxRadius;

            int segments = (int)(radius * 0.5f + 8.0f);
            if (segments < 4) segments = 4;
            if (segments > 32) segments = 32;

            size_t startIdx = g_VertexBuffer.size();
            float angleStep = 1.57079632679f / (float)segments; // PI/2

            // Center point for triangle fan
            float cx = x + w * 0.5f;
            float cy = y + h * 0.5f;

            // We'll build this as a filled polygon using triangle fan from center
            std::vector<std::pair<float, float>> points;

            // Top-right corner
            for (int i = 0; i <= segments; i++) {
                float angle = (float)i * angleStep;
                float px = x + w - radius + radius * cosf(angle);
                float py = y + radius - radius * sinf(angle);
                points.push_back({ px, py });
            }

            // Bottom-right corner
            for (int i = 0; i <= segments; i++) {
                float angle = (float)i * angleStep;
                float px = x + w - radius + radius * sinf(angle);
                float py = y + h - radius + radius * cosf(angle);
                points.push_back({ px, py });
            }

            // Bottom-left corner
            for (int i = 0; i <= segments; i++) {
                float angle = (float)i * angleStep;
                float px = x + radius - radius * cosf(angle);
                float py = y + h - radius + radius * sinf(angle);
                points.push_back({ px, py });
            }

            // Top-left corner
            for (int i = 0; i <= segments; i++) {
                float angle = (float)i * angleStep;
                float px = x + radius - radius * sinf(angle);
                float py = y + radius - radius * cosf(angle);
                points.push_back({ px, py });
            }

            // Create triangles from center
            for (size_t i = 0; i < points.size(); i++) {
                AddVertex(cx, cy, r, g, b, a);
                AddVertex(points[i].first, points[i].second, r, g, b, a);
                AddVertex(points[(i + 1) % points.size()].first, points[(i + 1) % points.size()].second, r, g, b, a);
            }

            g_CommandBuffer.push_back({ DrawCommandType::Triangles, startIdx, g_VertexBuffer.size() - startIdx, false });
        }

        void DrawFilledRoundedRect(float x, float y, float w, float h, float radius, float r, float g, float b, float a) {
            DrawRoundedRect(x, y, w, h, radius, r, g, b, a);
        }

        void DrawCircle(float cx, float cy, float radius, int segments, float r, float g, float b, float a) {
            if (segments < 8) segments = 8;
            if (segments > 128) segments = 128;

            float angleStep = 6.28318530718f / (float)segments;

            size_t startIdx = g_VertexBuffer.size();
            for (int i = 0; i < segments; i++) {
                float angle1 = (float)i * angleStep;
                float angle2 = (float)(i + 1) * angleStep;
                float x1 = cx + radius * cosf(angle1);
                float y1 = cy + radius * sinf(angle1);
                float x2 = cx + radius * cosf(angle2);
                float y2 = cy + radius * sinf(angle2);

                AddVertex(x1, y1, r, g, b, a);
                AddVertex(x2, y2, r, g, b, a);
            }
            g_CommandBuffer.push_back({ DrawCommandType::Lines, startIdx, (size_t)(segments * 2), g_AntiAliasEnabled });
        }

        void DrawFilledCircle(float cx, float cy, float radius, int segments, float r, float g, float b, float a) {
            if (segments < 8) segments = 8;
            if (segments > 128) segments = 128;

            float angleStep = 6.28318530718f / (float)segments;

            size_t startIdx = g_VertexBuffer.size();
            for (int i = 0; i < segments; i++) {
                float angle1 = (float)i * angleStep;
                float angle2 = (float)(i + 1) * angleStep;
                float x1 = cx + radius * cosf(angle1);
                float y1 = cy + radius * sinf(angle1);
                float x2 = cx + radius * cosf(angle2);
                float y2 = cy + radius * sinf(angle2);

                AddVertex(cx, cy, r, g, b, a);
                AddVertex(x1, y1, r, g, b, a);
                AddVertex(x2, y2, r, g, b, a);
            }
            g_CommandBuffer.push_back({ DrawCommandType::Triangles, startIdx, (size_t)(segments * 3), false });
        }

        void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a) {
            DrawLine(x1, y1, x2, y2, r, g, b, a);
            DrawLine(x2, y2, x3, y3, r, g, b, a);
            DrawLine(x3, y3, x1, y1, r, g, b, a);
        }

        void DrawFilledTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a) {
            size_t startIdx = g_VertexBuffer.size();
            AddVertex(x1, y1, r, g, b, a);
            AddVertex(x2, y2, r, g, b, a);
            AddVertex(x3, y3, r, g, b, a);
            g_CommandBuffer.push_back({ DrawCommandType::Triangles, startIdx, 3, false });
        }

        void DrawGradientRect(float x, float y, float w, float h,
            float r1, float g1, float b1, float a1,
            float r2, float g2, float b2, float a2, bool horizontal) {
            float ndcX1, ndcY1, ndcX2, ndcY2;
            ToNDC(x, y, ndcX1, ndcY1);
            ToNDC(x + w, y + h, ndcX2, ndcY2);

            size_t startIdx = g_VertexBuffer.size();

            if (horizontal) {
                // Gradient left to right
                g_VertexBuffer.push_back({ ndcX1, ndcY1, 0.0f, r1, g1, b1, a1 * g_GlobalAlpha });
                g_VertexBuffer.push_back({ ndcX2, ndcY1, 0.0f, r2, g2, b2, a2 * g_GlobalAlpha });
                g_VertexBuffer.push_back({ ndcX2, ndcY2, 0.0f, r2, g2, b2, a2 * g_GlobalAlpha });

                g_VertexBuffer.push_back({ ndcX1, ndcY1, 0.0f, r1, g1, b1, a1 * g_GlobalAlpha });
                g_VertexBuffer.push_back({ ndcX2, ndcY2, 0.0f, r2, g2, b2, a2 * g_GlobalAlpha });
                g_VertexBuffer.push_back({ ndcX1, ndcY2, 0.0f, r1, g1, b1, a1 * g_GlobalAlpha });
            }
            else {
                // Gradient top to bottom
                g_VertexBuffer.push_back({ ndcX1, ndcY1, 0.0f, r1, g1, b1, a1 * g_GlobalAlpha });
                g_VertexBuffer.push_back({ ndcX2, ndcY1, 0.0f, r1, g1, b1, a1 * g_GlobalAlpha });
                g_VertexBuffer.push_back({ ndcX2, ndcY2, 0.0f, r2, g2, b2, a2 * g_GlobalAlpha });

                g_VertexBuffer.push_back({ ndcX1, ndcY1, 0.0f, r1, g1, b1, a1 * g_GlobalAlpha });
                g_VertexBuffer.push_back({ ndcX2, ndcY2, 0.0f, r2, g2, b2, a2 * g_GlobalAlpha });
                g_VertexBuffer.push_back({ ndcX1, ndcY2, 0.0f, r2, g2, b2, a2 * g_GlobalAlpha });
            }

            g_CommandBuffer.push_back({ DrawCommandType::Triangles, startIdx, 6, false });
        }

        void DrawPolygon(const float* points, int pointCount, float r, float g, float b, float a) {
            if (pointCount < 3) return;

            size_t startIdx = g_VertexBuffer.size();
            for (int i = 0; i < pointCount; i++) {
                int next = (i + 1) % pointCount;
                AddVertex(points[i * 2], points[i * 2 + 1], r, g, b, a);
                AddVertex(points[next * 2], points[next * 2 + 1], r, g, b, a);
            }
            g_CommandBuffer.push_back({ DrawCommandType::Lines, startIdx, (size_t)(pointCount * 2), g_AntiAliasEnabled });
        }

        void DrawFilledPolygon(const float* points, int pointCount, float r, float g, float b, float a) {
            if (pointCount < 3) return;

            // Calculate centroid
            float cx = 0, cy = 0;
            for (int i = 0; i < pointCount; i++) {
                cx += points[i * 2];
                cy += points[i * 2 + 1];
            }
            cx /= (float)pointCount;
            cy /= (float)pointCount;

            size_t startIdx = g_VertexBuffer.size();
            for (int i = 0; i < pointCount; i++) {
                int next = (i + 1) % pointCount;
                AddVertex(cx, cy, r, g, b, a);
                AddVertex(points[i * 2], points[i * 2 + 1], r, g, b, a);
                AddVertex(points[next * 2], points[next * 2 + 1], r, g, b, a);
            }
            g_CommandBuffer.push_back({ DrawCommandType::Triangles, startIdx, (size_t)(pointCount * 3), false });
        }

        void DrawBezierCurve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
            int segments, float r, float g, float b, float a) {
            if (segments < 4) segments = 4;
            if (segments > 64) segments = 64;

            size_t startIdx = g_VertexBuffer.size();

            float prevX = x1, prevY = y1;
            for (int i = 1; i <= segments; i++) {
                float t = (float)i / (float)segments;
                float t2 = t * t;
                float t3 = t2 * t;
                float mt = 1.0f - t;
                float mt2 = mt * mt;
                float mt3 = mt2 * mt;

                float x = mt3 * x1 + 3.0f * mt2 * t * x2 + 3.0f * mt * t2 * x3 + t3 * x4;
                float y = mt3 * y1 + 3.0f * mt2 * t * y2 + 3.0f * mt * t2 * y3 + t3 * y4;

                AddVertex(prevX, prevY, r, g, b, a);
                AddVertex(x, y, r, g, b, a);

                prevX = x;
                prevY = y;
            }
            g_CommandBuffer.push_back({ DrawCommandType::Lines, startIdx, (size_t)(segments * 2), g_AntiAliasEnabled });
        }

        void Render() {
            if (g_VertexBuffer.empty() || g_CommandBuffer.empty()) {
                g_VertexBuffer.clear();
                g_CommandBuffer.clear();
                return;
            }

            ID3D11Device* device = Core::GetDevice();
            ID3D11DeviceContext* context = Core::GetContext();

            if (!device || !context) {
                g_VertexBuffer.clear();
                g_CommandBuffer.clear();
                return;
            }

            // Release old buffer
            if (g_D3DVertexBuffer) {
                g_D3DVertexBuffer->Release();
                g_D3DVertexBuffer = nullptr;
            }

            // Create vertex buffer
            D3D11_BUFFER_DESC bd = {};
            bd.Usage = D3D11_USAGE_DEFAULT;
            bd.ByteWidth = sizeof(Vertex) * static_cast<UINT>(g_VertexBuffer.size());
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = 0;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = g_VertexBuffer.data();

            if (FAILED(device->CreateBuffer(&bd, &initData, &g_D3DVertexBuffer))) {
                g_VertexBuffer.clear();
                g_CommandBuffer.clear();
                return;
            }

            // Set vertex buffer
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            context->IASetVertexBuffers(0, 1, &g_D3DVertexBuffer, &stride, &offset);

            // Set shader pipeline
            context->IASetInputLayout(Core::GetInputLayout());
            context->VSSetShader(Core::GetVertexShader(), nullptr, 0);
            context->PSSetShader(Core::GetPixelShader(), nullptr, 0);
            context->OMSetBlendState(Core::GetBlendState(), nullptr, 0xffffffff);
            context->RSSetState(Core::GetRasterizerState());

            // Execute draw commands
            for (const auto& cmd : g_CommandBuffer) {
                switch (cmd.type) {
                case DrawCommandType::Lines:
                    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                    context->Draw(static_cast<UINT>(cmd.vertexCount), static_cast<UINT>(cmd.vertexStart));
                    break;

                case DrawCommandType::Triangles:
                    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    context->Draw(static_cast<UINT>(cmd.vertexCount), static_cast<UINT>(cmd.vertexStart));
                    break;

                case DrawCommandType::TriangleStrip:
                    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                    context->Draw(static_cast<UINT>(cmd.vertexCount), static_cast<UINT>(cmd.vertexStart));
                    break;

                case DrawCommandType::LineStrip:
                    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
                    context->Draw(static_cast<UINT>(cmd.vertexCount), static_cast<UINT>(cmd.vertexStart));
                    break;
                }
            }

            // Clear buffers for next frame
            g_VertexBuffer.clear();
            g_CommandBuffer.clear();
        }
    }
}