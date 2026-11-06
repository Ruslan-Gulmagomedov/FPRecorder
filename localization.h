#pragma once

// Two-language UI string table.
//
// Every user-visible string goes through tr("English", "Русский"). The
// language is resolved once at startup from fp.ini [General] language =
// auto|en|ru ("auto" follows the Windows display language) and can be
// changed in Settings; a change applies after a restart.

#include <string>
#include <windows.h>

enum class UiLanguage {
	English,
	Russian
};

namespace ui_language {

	inline UiLanguage& current() {
		static UiLanguage language = UiLanguage::English;
		return language;
	}

	inline bool is_russian() {
		return current() == UiLanguage::Russian;
	}

	inline bool system_prefers_russian() {
		return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_RUSSIAN;
	}

	// Accepts "auto", "en", "ru" (also "english" / "russian"); anything
	// else falls back to auto.
	inline void select(const std::string& value) {
		if (value == "ru" || value == "russian") current() = UiLanguage::Russian;
		else if (value == "en" || value == "english") current() = UiLanguage::English;
		else current() = system_prefers_russian() ? UiLanguage::Russian : UiLanguage::English;
	}

	// Maps the Settings list selection to an fp.ini value.
	inline std::string setting_from_list_index(int index) {
		if (index == 1) return "en";
		if (index == 2) return "ru";
		return "auto";
	}

}

inline const wchar_t* tr(const wchar_t* english, const wchar_t* russian) {
	return ui_language::is_russian() ? russian : english;
}

inline std::wstring trs(const wchar_t* english, const wchar_t* russian) {
	return std::wstring(tr(english, russian));
}
