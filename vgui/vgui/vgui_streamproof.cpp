#include "vgui_streamproof.h"
#include <fstream>
#include <string>

namespace VGUI {
    namespace StreamProof {
        static bool g_Enabled = false;

        std::string GetConfigPath() {
            char path[MAX_PATH];
            GetModuleFileNameA(nullptr, path, MAX_PATH);
            std::string pathStr(path);
            size_t lastSlash = pathStr.find_last_of("\\/");
            if (lastSlash != std::string::npos) {
                pathStr = pathStr.substr(0, lastSlash + 1);
            }
            return pathStr + "vgui_config.ini";
        }

        void LoadConfig() {
            std::ifstream file(GetConfigPath());
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    if (line.find("streamproof=") == 0) {
                        g_Enabled = (line.substr(12) == "1");
                    }
                }
                file.close();
            }
        }

        void SaveConfig() {
            std::ofstream file(GetConfigPath());
            if (file.is_open()) {
                file << "streamproof=" << (g_Enabled ? "1" : "0") << std::endl;
                file.close();
            }
        }

        bool IsEnabled() {
            return g_Enabled;
        }

        void SetEnabled(bool enabled) {
            g_Enabled = enabled;
            SaveConfig();
        }

        void ApplyToWindow(HWND hwnd) {
            if (!g_Enabled || !hwnd) return;

            // Use SetWindowDisplayAffinity to exclude from screen capture
            typedef BOOL(WINAPI* SetWindowDisplayAffinity_t)(HWND, DWORD);
            HMODULE user32 = GetModuleHandleA("user32.dll");
            if (user32) {
                SetWindowDisplayAffinity_t SetWindowDisplayAffinityFunc =
                    (SetWindowDisplayAffinity_t)GetProcAddress(user32, "SetWindowDisplayAffinity");
                if (SetWindowDisplayAffinityFunc) {
                    SetWindowDisplayAffinityFunc(hwnd, 0x00000011); // WDA_EXCLUDEFROMCAPTURE
                }
            }
        }
    }
}
