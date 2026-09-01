#pragma once

#include <windows.h>

#include <string>

namespace resize_symmetrically {

enum class ModifierKey : DWORD {
    Alt = 0,
    Ctrl = 1,
    Shift = 2,
    Win = 3,
};

enum class Language : DWORD {
    Auto = 0,
    Russian = 1,
    English = 2,
};

struct AppSettings {
    ModifierKey modifier = ModifierKey::Alt;
    Language language = Language::Auto;
};

[[nodiscard]] AppSettings LoadSettings();
bool SaveSettings(const AppSettings& settings);
[[nodiscard]] bool IsAutostartEnabled();
bool SetAutostartEnabled(bool enabled);
bool RemoveAllSettings();
[[nodiscard]] std::wstring ExecutablePath();
[[nodiscard]] bool IsRussianLanguage(Language language);

}  // namespace resize_symmetrically
