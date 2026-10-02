// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <numeric>
//
#include <Binding.h>
#include <Context.h>
#include <Exception.h>
#include <NoiseFlow.h>
#include <TextMacro.h>
#include <Utils.h>
#include <Vars.h>

using std::format;
using std::string;

namespace noise {

string TextMacro::expand(ContextManager* context_manager) {
    std::ostringstream out;
    owner()->stream_expand(_text, format("macro:{}", name()), out);
    return out.str();
};

RepeatMacro::RepeatMacro(NoiseFlow* owner) : Macro("repeat") {
    set_owner(owner);
    add_arg(Binding("text", "", BindingAttr::REQUIRED));
    add_arg(Binding("count", "1"));
}

string RepeatMacro::expand(ContextManager* context_manager) {
    string text = context_manager->get("text").value_or("");
    long count = std::stol(context_manager->get("count").value_or("1"));
    std::ostringstream out;
    for (long i = 0; i < count; ++i) {
        out << text;
    }
    return out.str();
}

namespace {

// word: split on whitespace; line: split on '\n'; paragraph: split on runs
// of blank lines.
std::vector<string> split_by_unit(const string& text, const string& unit) {
    std::vector<string> items;
    if (unit == "line") {
        std::istringstream iss(text);
        string line;
        while (std::getline(iss, line)) {
            items.push_back(line);
        }
    } else if (unit == "paragraph") {
        std::regex re("\n[ \t]*\n+");
        std::sregex_token_iterator it(text.begin(), text.end(), re, -1), end;
        for (; it != end; ++it) {
            string p = trim(*it);
            if (!p.empty()) {
                items.push_back(p);
            }
        }
    } else {
        std::istringstream iss(text);
        string word;
        while (iss >> word) {
            items.push_back(word);
        }
    }
    return items;
}

}  // namespace

SelectMacro::SelectMacro(NoiseFlow* owner) : Macro("select") {
    set_owner(owner);
    add_arg(Binding("text", "", BindingAttr::REQUIRED));
    add_param(Binding("unit", "word"));
    add_param(Binding("count", "1"));
    add_param(Binding("dups", "false", BindingAttr::BOOL));
    add_param(Binding("sorted", "false", BindingAttr::BOOL));
}

string SelectMacro::expand(ContextManager* context_manager) {
    string text = context_manager->get("text").value_or("");
    string unit = context_manager->get("unit").value_or("word");
    long count = std::stol(context_manager->get("count").value_or("1"));
    bool dups = context_manager->get("dups").value_or("false") == "true";
    bool sorted = context_manager->get("sorted").value_or("false") == "true";

    std::vector<string> items = split_by_unit(text, unit);
    if (items.empty()) {
        return "";
    }
    // A count above the number of items can only be satisfied with repeats.
    if (count > static_cast<long>(items.size())) {
        dups = true;
    }

    std::vector<size_t> idx;
    if (dups) {
        std::uniform_int_distribution<size_t> d(0, items.size() - 1);
        for (long i = 0; i < count; ++i) {
            idx.push_back(d(gen()));
        }
    } else {
        idx.resize(items.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::shuffle(idx.begin(), idx.end(), gen());
        idx.resize(count);
    }
    if (sorted) {
        std::sort(idx.begin(), idx.end());
    }

    string sep = (unit == "word") ? " " : "\n";
    std::ostringstream out;
    for (long i = 0; i < count; ++i) {
        if (i) {
            out << sep;
        }
        out << items[idx[i]];
    }
    return out.str();
}

ForeachMacro::ForeachMacro(NoiseFlow* owner) : Macro("foreach") {
    set_owner(owner);
    add_arg(Binding("items", "", BindingAttr::REQUIRED));
    add_arg(Binding("text", "", BindingAttr::REQUIRED));
    add_param(Binding("unit", "word"));
    add_param(Binding("var", "item"));
}

string ForeachMacro::expand(ContextManager* context_manager) {
    string items = context_manager->get("items").value_or("");
    string text = context_manager->get("text").value_or("");
    string unit = context_manager->get("unit").value_or("word");
    string var = context_manager->get("var").value_or("item");

    std::ostringstream out;
    for (const string& item : split_by_unit(items, unit)) {
        Context ctx;
        ctx.add_map(var, item);
        context_manager->push(std::move(ctx));
        owner()->stream_expand(text, format("macro:{}:{}", name(), var), out);
        context_manager->pop();
    }
    return out.str();
}

// substr<str, msb[, lsb][, fmt]>: Verilog-style selection of characters of
// str, formatted by fmt first. Positions count from the right - 0 is the last
// character (the least significant digit of a formatted number) - and
// [msb:lsb] includes both ends; lsb defaults to msb (a single character).
// Positions are characters (UTF-8 code points), not bytes; anything but
// 0 <= lsb <= msb < length is an error (no negatives, no clamping).
// fmt is a std::format spec (the part after ':' in "{:...}", as in the
// <dist>_dist macros' fmt) applied to str as an integer if it is one, else as
// a real number if it is one, else as text; empty: str as is.
// str is a param, so it is fully expanded before formatting.
SubstrMacro::SubstrMacro(NoiseFlow* owner) : Macro("substr") {
    set_owner(owner);
    add_param(Binding("str", "", BindingAttr::REQUIRED));
    add_param(Binding("msb", "", BindingAttr::REQUIRED));
    add_param(Binding("lsb", ""));
    add_param(Binding("fmt", ""));
}

namespace {

string format_value(const string& str, const string& fmt) {
    if (fmt.empty()) {
        return str;
    }
    string spec = "{:" + fmt + "}";
    try {
        if (auto i = parse_int64(str)) {
            return std::vformat(spec, std::make_format_args(*i));
        }
        if (auto u = parse_uint64(str)) {
            return std::vformat(spec, std::make_format_args(*u));
        }
        if (auto d = parse_real(str)) {
            return std::vformat(spec, std::make_format_args(*d));
        }
        return std::vformat(spec, std::make_format_args(str));
    } catch (const std::format_error& e) {
        throw NoiseValueError(format(
            "substr fmt '{}' can't format '{}': {}", fmt, str, e.what()));
    }
}

}  // namespace

string SubstrMacro::expand(ContextManager* context_manager) {
    string str = context_manager->get("str").value_or("");
    string msb_text = context_manager->get("msb").value_or("");
    string lsb_text = context_manager->get("lsb").value_or("");
    string fmt = context_manager->get("fmt").value_or("");

    string text = format_value(str, fmt);
    // byte offset of each character's start, plus the end
    std::vector<size_t> starts;
    for (size_t i = 0; i < text.size(); ++i) {
        if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80) {
            starts.push_back(i);
        }
    }
    size_t length = starts.size();
    starts.push_back(text.size());

    auto position = [&](const string& name, const string& value) {
        auto v = parse_uint64(value);
        if (!v) {
            throw NoiseValueError(format(
                "substr {} '{}' is not a non-negative integer", name, value));
        }
        return *v;
    };
    uint64_t msb = position("msb", msb_text);
    uint64_t lsb = trim(lsb_text).empty() ? msb : position("lsb", lsb_text);
    if (lsb > msb || msb >= length) {
        throw NoiseValueError(format(
            "substr [{}:{}] is invalid for '{}' ({} characters; 0 <= lsb <= msb < {} "
            "is required)", msb, lsb, text, length, length));
    }
    // position p (from the right) is character length-1-p from the left
    size_t first = length - 1 - msb;
    size_t last = length - 1 - lsb;
    return text.substr(starts[first], starts[last + 1] - starts[first]);
}

};  // namespace noise
