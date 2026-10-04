// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#pragma once

#include <noise_std_include.h>

namespace noise {

enum class BindingAttr : unsigned {
    NONE = 0,
    REQUIRED = 0x1,
    QUOTED = 0x4,
    // Whitespace trimming is the default; these opt out of it per side.
    // "keep_enclosing_ws" (alias "kws") sets both bits.
    KEEP_LEFT_WS = 0x8,
    KEEP_RIGHT_WS = 0x10,
    // The bound value is interpreted as a boolean string (see
    // DocParser.yy's normalize_bool), not used verbatim.
    BOOL = 0x20,
};

/// How a param/arg is assigned (see bind_list in DocParser.yy):
/// NORMAL "name = value"; COND "name := value" - only if the name isn't
/// defined already (else the defined value is kept), and in a definition it
/// makes every normal assignment by a caller conditional; FORCE "name =!
/// value" - a caller assigning even if the definition said ":=".
enum class AssignMode { NORMAL, COND, FORCE };

inline BindingAttr operator|(BindingAttr a, BindingAttr b) {
    return static_cast<BindingAttr>(static_cast<unsigned>(a) |
                                     static_cast<unsigned>(b));
}

inline bool has(BindingAttr set, BindingAttr bit) {
    return (static_cast<unsigned>(set) & static_cast<unsigned>(bit)) != 0;
}

/// @brief Binding is an immutable relation between a string name, an
/// optional string value, and a set of attrs
/// (REQUIRED/QUOTED/KEEP_LEFT_WS/KEEP_RIGHT_WS).
/// An empty name denotes a positional (unnamed) instantiation-site entry.
class Binding {
  public:
    Binding(const std::string& name, const std::string& value = "",
            BindingAttr attrs = BindingAttr::NONE,
            AssignMode mode = AssignMode::NORMAL)
        : _name(name), _value(value), _attrs(attrs), _mode(mode) {}
    Binding() = default;
    Binding(const Binding& o) = default;

    Binding& operator=(const Binding& o) = default;

    virtual ~Binding() {}

    const std::string& name() const { return _name; }
    const std::string& value() const { return _value; }
    BindingAttr attrs() const { return _attrs; }
    AssignMode mode() const { return _mode; }

    bool required() const { return has(_attrs, BindingAttr::REQUIRED); }
    bool quoted() const { return has(_attrs, BindingAttr::QUOTED); }
    bool keep_left_ws() const { return has(_attrs, BindingAttr::KEEP_LEFT_WS); }
    bool keep_right_ws() const {
        return has(_attrs, BindingAttr::KEEP_RIGHT_WS);
    }
    bool is_bool() const { return has(_attrs, BindingAttr::BOOL); }

  private:
    std::string _name;
    std::string _value;
    BindingAttr _attrs = BindingAttr::NONE;
    AssignMode _mode = AssignMode::NORMAL;
};

/// @brief Throws NoiseDefBuilderError if a non-required entry precedes a
/// required one - required params/args must be a prefix of the list.
void validate_binding_order(const std::vector<Binding>& list);

};  // namespace noise
