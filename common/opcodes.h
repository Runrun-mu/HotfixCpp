#pragma once
#include <cstdint>

enum OpCode : uint8_t {
    OP_ICONST = 1,
    OP_PRINT  = 2,
    OP_RET    = 3
};