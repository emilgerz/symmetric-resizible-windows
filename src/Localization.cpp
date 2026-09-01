#include "Localization.h"

namespace resize_symmetrically {

std::wstring_view Text(TextId id, Language language) {
    const bool ru = IsRussianLanguage(language);
    switch (id) {
    case TextId::AppTitle: return L"Resize Symmetrically";
    case TextId::Settings: return ru ? L"Настройки" : L"Settings";
    case TextId::Exit: return ru ? L"Выход" : L"Exit";
    case TextId::Modifier: return ru ? L"Клавиша-модификатор:" : L"Modifier key:";
    case TextId::Autostart: return ru ? L"Запускать вместе с Windows" : L"Start with Windows";
    case TextId::Language: return ru ? L"Язык:" : L"Language:";
    case TextId::AutoLanguage: return ru ? L"Автоматически" : L"Automatic";
    case TextId::Russian: return L"Русский";
    case TextId::English: return L"English";
    case TextId::Close: return ru ? L"Закрыть" : L"Close";
    case TextId::GestureHelp:
        return ru
            ? L"Зажмите выбранную клавишу и потяните границу или угол окна."
            : L"Hold the selected key and drag a window edge or corner.";
    case TextId::SaveError:
        return ru ? L"Не удалось сохранить настройку." : L"Could not save the setting.";
    case TextId::Alt: return L"Alt";
    case TextId::Ctrl: return L"Ctrl";
    case TextId::Shift: return L"Shift";
    case TextId::Win: return L"Win";
    }
    return L"";
}

}  // namespace resize_symmetrically
