#pragma once

namespace VGUI {
    namespace Draw {
        // Global settings
        void SetGlobalAlpha(float alpha);
        void EnableAntiAliasing(bool enable);

        // Basic shapes
        void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a = 1.0f);
        void DrawThickLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a = 1.0f);

        void DrawRect(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f);
        void DrawRectThick(float x, float y, float w, float h, float thickness, float r, float g, float b, float a = 1.0f);
        void DrawFilledRect(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f);

        void DrawRoundedRect(float x, float y, float w, float h, float radius, float r, float g, float b, float a = 1.0f);
        void DrawFilledRoundedRect(float x, float y, float w, float h, float radius, float r, float g, float b, float a = 1.0f);

        void DrawCircle(float cx, float cy, float radius, int segments, float r, float g, float b, float a = 1.0f);
        void DrawFilledCircle(float cx, float cy, float radius, int segments, float r, float g, float b, float a = 1.0f);

        void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a = 1.0f);
        void DrawFilledTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a = 1.0f);

        // Advanced shapes
        void DrawGradientRect(float x, float y, float w, float h,
            float r1, float g1, float b1, float a1,
            float r2, float g2, float b2, float a2,
            bool horizontal = true);

        void DrawPolygon(const float* points, int pointCount, float r, float g, float b, float a = 1.0f);
        void DrawFilledPolygon(const float* points, int pointCount, float r, float g, float b, float a = 1.0f);

        void DrawBezierCurve(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
            int segments, float r, float g, float b, float a = 1.0f);

        // Rendering
        void Render();
    }
}