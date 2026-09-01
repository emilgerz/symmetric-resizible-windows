#include "Settings.h"

#include <array>

namespace resize_symmetrically {
namespace {

constexpr wchar_t kSettingsKey[] = L"Software\\ResizeSymmetrically";
constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValue[] = L"ResizeSymmetrically";

DWORD ReadDword(HKEY key, const wchar_t* name, DWORD fallback) {
    DWORD value = fallback;
    DWORD size = sizeof(value);
    if (RegGetValueW(key, nullptr, name, RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS) {
        return fallback;
    }
    return value;
}

}  // namespace

AppSettings LoadSettings() {
    AppSettings settings;
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return settings;
    }

    const DWORD modifier = ReadDword(key, L"Modifier", static_cast<DWORD>(ModifierKey::Alt));
    const DWORD language = ReadDword(key, L"Language", static_cast<DWORD>(Language::Auto));
    RegCloseKey(key);

    if (modifier <= static_cast<DWORD>(ModifierKey::Win)) {
        settings.modifier = static_cast<ModifierKey>(modifier);
    }
    if (language <= static_cast<DWORD>(Language::English)) {
        settings.language = static_cast<Language>(language);
    }
    return settings;
}

bool SaveSettings(const AppSettings& settings) {
    HKEY key = nullptr;
    DWORD disposition = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, nullptr, 0,
            KEY_SET_VALUE, nullptr, &key, &disposition) != ERROR_SUCCESS) {
        return false;
    }

    const DWORD modifier = static_cast<DWORD>(settings.modifier);
    const DWORD language = static_cast<DWORD>(settings.language);
    const bool ok =
        RegSetValueExW(key, L"Modifier", 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&modifier), sizeof(modifier)) == ERROR_SUCCESS &&
        RegSetValueExW(key, L"Language", 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&language), sizeof(language)) == ERROR_SUCCESS;
    RegCloseKey(key);
    return ok;
}

std::wstring ExecutablePath() {
    std::wstring result(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, result.data(), static_cast<DWORD>(result.size()));
    if (length == 0 || length >= result.size()) {
        return {};
    }
    result.resize(length);
    return result;
}

bool IsAutostartEnabled() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }
    const LONG result = RegQueryValueExW(key, kRunValue, nullptr, nullptr, nullptr, nullptr);
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

bool SetAutostartEnabled(bool enabled) {
    HKEY key = nullptr;
    DWORD disposition = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0,
            KEY_SET_VALUE, nullptr, &key, &disposition) != ERROR_SUCCESS) {
        return false;
    }

    LONG result = ERROR_SUCCESS;
    if (enabled) {
        const std::wstring path = ExecutablePath();
        const std::wstring command = L"\"" + path + L"\" --startup";
        result = RegSetValueExW(key, kRunValue, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()),
            static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        result = RegDeleteValueW(key, kRunValue);
        if (result == ERROR_FILE_NOT_FOUND) {
            result = ERROR_SUCCESS;
        }
    }
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

bool RemoveAllSettings() {
    const bool autostartRemoved = SetAutostartEnabled(false);
    LONG result = RegDeleteTreeW(HKEY_CURRENT_USER, kSettingsKey);
    if (result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND) {
        result = ERROR_SUCCESS;
    }
    return autostartRemoved && result == ERROR_SUCCESS;
}

bool IsRussianLanguage(Language language) {
    if (language == Language::Russian) {
        return true;
    }
    if (language == Language::English) {
        return false;
    }
    const LANGID lang = GetUserDefaultUILanguage();
    return PRIMARYLANGID(lang) == LANG_RUSSIAN;
}

}  // namespace resize_symmetrically
