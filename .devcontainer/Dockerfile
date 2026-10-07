FROM ubuntu:26.04

ARG USERNAME=noise
ARG USER_UID=1234
ARG USER_GID=1234

ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

ENV TERM=xterm-256color
ENV COLORTERM=truecolor


# Base toolchain — adjust for your noise build (cmake, bison, flex, swig, etc.)
RUN apt-get update && apt-get install -y \
    build-essential cmake git gdb \
    clang clang-format clangd \
    bison flex swig python3-dev python3-pip python3-pytest \
    z3 libz3-dev python3-z3 \
    sudo curl ca-certificates \
    git-delta ripgrep fd-find fzf \
    readline-common emacs vim \
    locales \
    && locale-gen en_US.UTF-8 \
    && rm -rf /var/lib/apt/lists/*

# Performance and memory analysis tools. perf (linux-perf) and attaching to a
# running process also need host settings and container options
#   linux-perf         perf: CPU profiles, perf stat event counts (page faults...)
#   strace, ltrace     system/library call tracing and counting (-c)
#   time               /usr/bin/time -v: max RSS, page faults, context switches
#   valgrind           memcheck (leaks, invalid/uninitialized memory),
#                      massif (heap over time), callgrind/cachegrind
#   heaptrack          heap allocation profiling (who allocates, how much)
#   hyperfine          repeatable benchmarks (mean +- stddev, warmup)
#   clang-tidy,        static analysis (clang-analyzer-*, bugprone-*,
#   cppcheck           performance-*)
#   jq                 JSON (session logs, tool output)
RUN apt-get update && apt-get install -y \
    linux-perf \
    strace ltrace time \
    valgrind heaptrack hyperfine \
    clang-tidy cppcheck \
    jq \
    && rm -rf /var/lib/apt/lists/*


# Remove any existing user/group with the same UID/GID to avoid collisions
RUN if getent group $USER_GID >/dev/null; then groupdel $(getent group $USER_GID | cut -d: -f1); fi \
    && if getent passwd $USER_UID >/dev/null; then userdel -f $(getent passwd $USER_UID | cut -d: -f1); fi

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m -s /bin/bash $USERNAME \
    && echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME
