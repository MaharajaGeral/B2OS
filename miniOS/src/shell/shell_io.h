#pragma once

#include "shell/shell.h"

namespace shell_io {
    void init();

    void write(const char* text);
    int read_line(char* buffer, unsigned long capacity);
    void clear();

    const ShellIO* get();
}