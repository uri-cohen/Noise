// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#include <NoiseFlow.h>
#include <Binding.h>
#include <Macro.h>
#include <Vars.h>

namespace noise {

Macro::Macro(const std::string& name)
    : _owner(nullptr), _name(name), _hasParams(false), _hasArgs(false)
{}

Macro::~Macro()
{

}

void Macro::add_param(const Binding& param)
{
    _params.push_back(param);
    validate_binding_order(_params);
    _hasParams = true;
}

void Macro::add_arg(const Binding& arg)
{
    _args.push_back(arg);
    validate_binding_order(_args);
    _hasArgs = true;
}

std::mt19937_64& Macro::gen() { return _owner->gen(); }

}; // namespace noise
