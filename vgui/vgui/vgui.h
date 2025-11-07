#pragma once
#include "vgui_core.h"
#include "vgui_draw.h"
#include "vgui_streamproof.h"

// Convenience namespace that exposes everything
namespace VGUI {
    // Core functions
    using Core::Initialize;
    using Core::SetWindowSize;
    using Core::Cleanup;

    // Draw functions
    using Draw::DrawLine;
    using Draw::DrawRect;
    using Draw::DrawFilledRect;
    using Draw::DrawCircle;
    using Draw::Render;
}