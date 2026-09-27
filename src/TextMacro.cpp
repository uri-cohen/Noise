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

};  // namespace noise
