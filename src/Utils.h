// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

// General purpose utilities...

#pragma once

#include <format>
#include <iomanip>
#include <sstream>
#include <string>

namespace noise {

// Trims leading/trailing whitespace by default. keep_left/keep_right opt out
// of trimming that respective side (see BindingAttr::KEEP_LEFT_WS/RIGHT_WS).
inline std::string trim(const std::string& s, bool keep_left = false,
                         bool keep_right = false) {
    std::string::size_type begin = 0;
    std::string::size_type end = s.size();
    if (!keep_right) {
        std::string::size_type last = s.find_last_not_of(" \t\n");
        end = (last == std::string::npos) ? 0 : last + 1;
    }
    if (!keep_left) {
        std::string::size_type first = s.find_first_not_of(" \t\n");
        if (first == std::string::npos) {
            return "";
        }
        begin = first;
    }
    if (begin >= end) {
        return "";
    }
    return s.substr(begin, end - begin);
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

// for now this code is unused...
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
