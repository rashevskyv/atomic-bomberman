#include "resources/lang.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace ab {

namespace {

LanguageTable& table() {
    static LanguageTable t;
    return t;
}

void appendUtf8(std::string& out, unsigned code) {
    if (code < 0x80) {
        out += static_cast<char>(code);
    } else if (code < 0x800) {
        out += static_cast<char>(0xC0 | (code >> 6));
        out += static_cast<char>(0x80 | (code & 0x3F));
    } else if (code < 0x10000) {
        out += static_cast<char>(0xE0 | (code >> 12));
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (code & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (code >> 18));
        out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (code & 0x3F));
    }
}

void skipSpace(const std::string& s, std::size_t& i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
}

// Four hex digits at s[i..i+3]; false if they are not there.
bool hex4(const std::string& s, std::size_t i, unsigned& code) {
    if (i + 4 > s.size()) return false;
    code = 0;
    for (std::size_t k = 0; k < 4; ++k) {
        const int c = std::isxdigit(static_cast<unsigned char>(s[i + k])) ? std::tolower(static_cast<unsigned char>(s[i + k])) : -1;
        if (c < 0) return false;
        code = code * 16 + static_cast<unsigned>(c <= '9' ? c - '0' : c - 'a' + 10);
    }
    return true;
}

// A JSON string starting at s[i] (the quote). Escapes are read as JSON writes them.
bool readString(const std::string& s, std::size_t& i, std::string& out) {
    if (i >= s.size() || s[i] != '"') return false;
    for (++i; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '"') {
            ++i;
            return true;
        }
        if (c != '\\') {
            out += c;
            continue;
        }
        if (++i >= s.size()) return false;
        switch (s[i]) {
            case 'n': out += '\n'; break;
            case 't': out += '\t'; break;
            case 'r': out += '\r'; break;
            case 'b': case 'f': break;
            case 'u': {
                unsigned code = 0;
                if (!hex4(s, i + 1, code)) return false;
                i += 4;
                // A character outside the first 65536 comes as two escapes (a surrogate pair).
                if (code >= 0xD800 && code < 0xDC00 && i + 6 < s.size() && s[i + 1] == '\\' && s[i + 2] == 'u') {
                    unsigned low = 0;
                    if (!hex4(s, i + 3, low) || low < 0xDC00 || low >= 0xE000) return false;
                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                    i += 6;
                }
                appendUtf8(out, code);
                break;
            }
            default: out += s[i];  // \" \\ and \/
        }
    }
    return false;
}

}  // namespace

bool parseLanguageTable(const std::string& json, LanguageTable& out) {
    std::size_t i = json.compare(0, 3, "\xEF\xBB\xBF") == 0 ? 3 : 0;  // a byte-order mark is allowed
    skipSpace(json, i);
    if (i >= json.size() || json[i] != '{') return false;
    ++i;
    skipSpace(json, i);
    if (i < json.size() && json[i] == '}') return true;
    while (true) {
        std::string key, value;
        skipSpace(json, i);
        if (!readString(json, i, key)) return false;
        skipSpace(json, i);
        if (i >= json.size() || json[i] != ':') return false;
        ++i;
        skipSpace(json, i);
        if (!readString(json, i, value)) return false;
        out[key] = value;
        skipSpace(json, i);
        if (i >= json.size()) return false;
        if (json[i] == '}') return true;
        if (json[i] != ',') return false;
        ++i;
    }
}

namespace {

// A language code is a plain name: no paths.
bool plainCode(const std::string& code) {
    for (const char c : code)
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') return false;
    return true;
}

}  // namespace

bool loadLanguage(const std::string& dir, const std::string& code) {
    clearLanguage();
    if (code.empty() || code == "en") return true;
    if (!plainCode(code)) {
        std::fprintf(stderr, "WARN  Language code \"%s\" is not a plain name: English is used.\n", code.c_str());
        return false;
    }
    const std::string path = dir + "/" + code + ".json";
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "WARN  No language file %s: English is used.\n", path.c_str());
        return false;
    }
    std::ostringstream text;
    text << in.rdbuf();
    LanguageTable parsed;
    if (!parseLanguageTable(text.str(), parsed)) {
        std::fprintf(stderr, "WARN  Language file %s is not a JSON object of strings: English is used.\n", path.c_str());
        return false;
    }
    table() = std::move(parsed);
    std::fprintf(stderr, "INFO  Language: %s (%zu texts)\n", code.c_str(), table().size());
    return true;
}

void clearLanguage() { table().clear(); }

std::vector<LanguageChoice> listLanguages(const std::string& dir) {
    namespace fs = std::filesystem;
    std::vector<std::string> codes;
    std::error_code ec;
    fs::directory_iterator it(dir, ec);
    for (; !ec && it != fs::directory_iterator(); it.increment(ec)) {
        // UTF-8 bytes: a name the active code page cannot show must not throw here.
        const auto name = it->path().filename().u8string();
        const std::string file(reinterpret_cast<const char*>(name.data()), name.size());
        if (file.size() > 5 && file.compare(file.size() - 5, 5, ".json") == 0) codes.push_back(file.substr(0, file.size() - 5));
    }
    std::sort(codes.begin(), codes.end());
    std::vector<LanguageChoice> out{{"", "English"}};
    for (const std::string& code : codes) {
        if (code.empty() || code == "en" || !plainCode(code)) continue;  // English is always there, from the program
        std::ifstream in(dir + "/" + code + ".json", std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        LanguageTable parsed;
        if (!in || !parseLanguageTable(text.str(), parsed)) continue;
        const auto named = parsed.find("_language_name");
        out.push_back({code, named != parsed.end() && !named->second.empty() ? named->second : code});
    }
    return out;
}

const LanguageChoice& nextLanguage(const std::vector<LanguageChoice>& choices, const std::string& code, int direction) {
    static const LanguageChoice kEnglish{"", "English"};
    if (choices.empty()) return kEnglish;
    std::size_t at = 0;  // a code that is not in the list counts as English
    for (std::size_t i = 0; i < choices.size(); ++i)
        if (choices[i].code == code) {
            at = i;
            break;
        }
    const std::size_t n = choices.size();
    return choices[direction < 0 ? (at + n - 1) % n : (at + 1) % n];
}

std::string tr(const std::string& english) {
    const auto& t = table();
    if (t.empty()) return english;
    const auto it = t.find(english);
    return it == t.end() || it->second.empty() ? english : it->second;
}

std::string fillPlaceholders(const std::string& text, std::initializer_list<std::string> args) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if ((c == '{' || c == '}') && i + 1 < text.size() && text[i + 1] == c) {
            out += c;  // "{{" and "}}" are one brace
            ++i;
            continue;
        }
        if (c == '{') {
            std::size_t j = i + 1, index = 0;
            while (j < text.size() && std::isdigit(static_cast<unsigned char>(text[j])) && index < 100000) index = index * 10 + static_cast<std::size_t>(text[j++] - '0');
            if (j > i + 1 && j < text.size() && text[j] == '}' && index < args.size()) {
                out += *(args.begin() + static_cast<std::ptrdiff_t>(index));
                i = j;
                continue;
            }
        }
        out += c;
    }
    return out;
}

std::string trf(const std::string& english, std::initializer_list<std::string> args) {
    return fillPlaceholders(tr(english), args);
}

}  // namespace ab
