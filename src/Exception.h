// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

#pragma once

#include <noise_std_include.h>

using std::format;

namespace noise {

// assume no nested exceptions...
// non copieable
class NoiseException : public std::exception {
  public:
    NoiseException() {}
    virtual ~NoiseException() {}

    virtual const char* what() const noexcept {
        _buf[std::min(_idx, BUF_SIZE - 1)] = '\0';
        return _buf;
    }

  protected:
    void init_msg(const std::string& msg) {
        _idx = std::min(msg.size(), BUF_SIZE - 1);
        std::strncpy(_buf, msg.c_str(), _idx);
    }
    void append_msg(const std::string& msg) {
        if (_idx >= BUF_SIZE - 1) {
            return;
        }
        size_t n = std::min(msg.size(), BUF_SIZE - 1 - msg.size());
        std::strncpy(_buf + _idx, msg.c_str(), n);
        _idx += n;
    }

  private:
    static constexpr size_t BUF_SIZE{2048};
    static char _buf[BUF_SIZE];
    static size_t _idx;
};

class NoiseIOError : public NoiseException {
  public:
    NoiseIOError(const std::string& msg) {
        init_msg("Noise IO error: ");
        append_msg(msg);
    }
    ~NoiseIOError() {}
};

class NoiseMacroBuilderError : public NoiseException {
  public:
    NoiseMacroBuilderError(const std::string& msg) {
        init_msg("Noise Macro builder error: ");
        append_msg(msg);
    }
    ~NoiseMacroBuilderError() {}
};

class NoiseMacroCallError : public NoiseException {
  public:
    NoiseMacroCallError(const std::string& msg) {
        init_msg("Noise Macro call error: ");
        append_msg(msg);
    }
    ~NoiseMacroCallError() {}
};

class NoiseValueError : public NoiseException {
  public:
    NoiseValueError(const std::string& msg) {
        init_msg("Noise value error: ");
        append_msg(msg);
    }
    ~NoiseValueError() {}
};

class NoiseParamFormatError : public NoiseException {
  public:
    NoiseParamFormatError(int pos, const std::string& ba,
                          const std::string& msg) {
        init_msg("Noise param error: ");
        append_msg(format("position {} - {}): {}", pos, ba, msg));
    }
    ~NoiseParamFormatError() {}
};

class NoiseDefBuilderError : public NoiseException {
  public:
    NoiseDefBuilderError(const std::string& msg) {
        init_msg("Noise def builder error: ");
        append_msg(msg);
    }
    ~NoiseDefBuilderError() {}
};

class NoiseInternalError : public NoiseException {
  public:
    NoiseInternalError(const std::string& msg) {
        init_msg("Noise internal error: ");
        append_msg(msg);
    }
    ~NoiseInternalError() {}
};

}  // namespace noise
