#pragma once
#include <windows.h>

namespace VGUI {
    namespace StreamProof {
        void LoadConfig();
        void SaveConfig();
        bool IsEnabled();
        void SetEnabled(bool enabled);
        void ApplyToWindow(HWND hwnd);
    }
}