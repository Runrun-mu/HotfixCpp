#include "bytecode_vm.h"
#include "common/opcodes.h"
#include <vector>
#include <iostream>

// 在 runtime_api.cpp 中定义
extern std::vector<uint8_t> g_bytecode;

int ExecBytecode() {
    std::vector<int> stack;
    size_t ip = 0;

    while (ip < g_bytecode.size()) {
        OpCode op = static_cast<OpCode>(g_bytecode[ip++]);
        switch (op) {
        case OP_ICONST: {
            if (ip >= g_bytecode.size()) {
                std::cerr << "[VM] ICONST missing imm\n";
                return 0;
            }
            uint8_t imm = g_bytecode[ip++];
            stack.push_back(static_cast<int>(imm));
            break;
        }
        case OP_PRINT: {
            if (stack.empty()) {
                std::cerr << "[VM] PRINT on empty stack\n";
                break;
            }
            int v = stack.back();
            std::cout << v << std::endl;
            break;
        }
        case OP_RET: {
            int v = stack.empty() ? 0 : stack.back();
            return v;
        }
        default:
            std::cerr << "[VM] Unknown opcode: " << (int)op << "\n";
            return 0;
        }
    }

    return 0;
}
