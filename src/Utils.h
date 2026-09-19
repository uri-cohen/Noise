// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

// General purpose utilities...

#pragma once

#include <format>
#include <iomanip>
#include <sstream>
#include <string>

namespace noise {

inline std::string trim(const std::string& s) {
    std::string::size_type last = s.find_last_not_of(" \t\n");
    if (last == std::string::npos) {
        return "";
    }
    std::string::size_type first = s.find_first_not_of(" \t\n");
    return s.substr(first, last - first + 1);
};

// '\' is an escape char: the following character is taken literally and
// the backslash itself is discarded.
inline std::string unescape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (std::string::size_type i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            out += s[++i];
        } else {
            out += s[i];
        }
    }
    return out;
}

inline std::string align_string(const std::string& s, int col) {
    if (s.empty()) {
        return "";
    }

    std::stringstream out_ss;
    for (int i = 0; i < s.size() - 1; ++i) {
        out_ss << s[i];
        if (s[i] == '\n') {
            out_ss << std::setw(col) << ' ';
        }
    }
    out_ss << s[s.size() - 1];
    return out_ss.str();
}

};  // namespace noise
