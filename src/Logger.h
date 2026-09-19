// Copyrights Uri Cohen uri.l.cohen@gmail.com 2026

// Note
// Logger belongs to the global namespace!

#pragma once

#ifndef PROJECT
#define PROJECT
#endif

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#define PROJECT_LOG_LEVELS       \
    X(DEBUG, 60, D, "debug")     \
    X(VERBOSE, 50, V, "verbose") \
    X(INFO, 40, I, "info")       \
    X(WARNING, 30, W, "warning") \
    X(ERROR, 20, E, "error")     \
    X(FATAL, 10, F, "fatal")

class Logger {
  public:
#define X(E, L, C, S) LOG_##E = L,
    enum class Level { PROJECT_LOG_LEVELS N_LEVELS };
#undef X

    Logger(const std::string& log_file_name = "",
           int32_t level = int(Level::LOG_INFO))
        : _file_name(log_file_name),
          _file(log_file_name.empty()
                    ? &std::cerr
                    : new std::ofstream(std::filesystem::path(log_file_name))),
          _level(level) {}
    virtual ~Logger() {
        if (_file && _file != &std::cerr) {
            dynamic_cast<std::ofstream*>(_file)->close();
        }
    }
    void log(uint32_t level, const std::string& msg) {
        if (_file && level <= _level) {
            *_file << msg << std::endl;
        }
    }

    static void init(const std::string& log_file_name, int32_t level) {
        logger = std::make_shared<Logger>(log_file_name, level);
    }
    static int32_t level() { return logger ? logger->_level : -1; }
    static void set_level(uint32_t l) { logger->_level = l; }
    static void set_level(const std::string& s) {
#define X(E, L, C, S)       \
    if (s == S) {           \
        logger->_level = L; \
        return;             \
    }
        PROJECT_LOG_LEVELS;
#undef X
        std::cerr << "unknown log level '" << s << "'" << std::endl;
    }
    static const std::string& file_name() { return logger->_file_name; }

    static std::shared_ptr<Logger> logger;

  private:
    std::string _file_name;
    std::ostream* _file;
    uint32_t _level;
};

#define QQ(A) #A
#define Q(A) QQ(A)
#define X(E, L, C, S)                                                       \
    template <typename... Args>                                             \
    inline void E(uint32_t n, std::string_view fmt, Args&&... args) {       \
        if (!Logger::logger && (L >= int(Logger::Level::LOG_WARNING))) {    \
            return;                                                         \
        }                                                                   \
        std::string id(std::format(Q(PROJECT) "-" #C "-{:3}: ", n));        \
        std::string msg(std::vformat(fmt, std::make_format_args(args...))); \
        if (Logger::logger) {                                               \
            Logger::logger->log(int(Logger::Level::LOG_##E), id + msg);     \
        } else {                                                            \
            std::cerr << id << msg << std::endl;                            \
        }                                                                   \
    };

PROJECT_LOG_LEVELS
#undef X
#undef Q
#undef QQ
