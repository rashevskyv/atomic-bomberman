// The language of the text on screen. A language is one UTF-8 JSON object that maps
// English text to its translation, for example data/lang/uk.json. The English text is
// the key in the code and the fallback: a missing key, or a missing file, shows English.
//
// Text with values in it uses positional placeholders: trf("{0} of {1}", {a, b}). A
// translator may move them. A literal brace is written twice: "{{" and "}}".
#pragma once

#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace ab {

using LanguageTable = std::unordered_map<std::string, std::string>;

// Reads <dir>/<code>.json and makes it the language. An empty code, or "en", is English
// and reads nothing. Returns false (and stays in English) when the file is missing or not
// valid. Call once at start-up, before any screen is drawn.
bool loadLanguage(const std::string& dir, const std::string& code);

// Back to English.
void clearLanguage();

// The translation of an English text, or the text itself when there is none.
std::string tr(const std::string& english);

// tr(), then the placeholders {0}, {1}, ... are replaced by the arguments.
// A placeholder with no argument stays as written.
std::string trf(const std::string& english, std::initializer_list<std::string> args);

// Replaces the placeholders of a text (already translated or not) by the arguments.
std::string fillPlaceholders(const std::string& text, std::initializer_list<std::string> args);

// Reads a JSON object whose keys and values are strings. False when it is not one.
bool parseLanguageTable(const std::string& json, LanguageTable& out);

// A language to choose in the Options menu. The English one is always first, with the code "".
struct LanguageChoice {
    std::string code;  // the file name without ".json"; empty: English
    std::string name;  // shown in the menu: the table's "_language_name" entry, else the code
};

// English first, then every <dir>/<code>.json that is a valid table, by code. A file that is not
// a valid table is skipped. The key "_language_name" holds the language's own name for the menu.
std::vector<LanguageChoice> listLanguages(const std::string& dir);

// The choice after (direction 1) or before (-1) the one with this code, wrapping round. A code
// that is not in the list starts from English.
const LanguageChoice& nextLanguage(const std::vector<LanguageChoice>& choices, const std::string& code, int direction);

}  // namespace ab
