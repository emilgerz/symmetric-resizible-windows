#pragma once

#include "Settings.h"

#include <string_view>

namespace resize_symmetrically {

enum class TextId {
    AppTitle,
    Settings,
    Exit,
    Modifier,
    Autostart,
    Language,
    AutoLanguage,
    Russian,
    English,
    Close,
    GestureHelp,
    SaveError,
    Alt,
    Ctrl,
    Shift,
    Win,
};

[[nodiscard]] std::wstring_view Text(TextId id, Language language);

}  // namespace resize_symmetrically
