#pragma once

namespace serial {
    void init();
    void write(char c);
    bool data_available();
    char read_byte();
}