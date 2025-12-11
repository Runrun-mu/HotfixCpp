#pragma once
#include <cstdint>

// 最简单的 diff：把 bytecode[offset] 改成 new_value
struct PatchOp {
    uint32_t offset;
    uint8_t  new_value;
};