// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <numeric>
//
#include <Binding.h>
#include <Context.h>
#include <NoiseFlow.h>
#include <TextMacro.h>
#include <Utils.h>

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
}

string SelectMacro::expand(ContextManager* context_manager) {
    string text = context_manager->get("text").value_or("");
    string unit = context_manager->get("unit").value_or("word");
    long count = std::stol(context_manager->get("count").value_or("1"));

    std::vector<string> items = split_by_unit(text, unit);
    if (items.empty()) {
        return "";
    }
    count = std::min<long>(count, static_cast<long>(items.size()));

    std::vector<size_t> idx(items.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), gen());
    idx.resize(count);
    std::sort(idx.begin(), idx.end());

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

};  // namespace noise
