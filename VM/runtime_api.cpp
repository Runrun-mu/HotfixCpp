#include "runtime_api.h"
#include "common/diff_format.h"
#include "bytecode_vm.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>

#include <sys/mman.h>
#include <unistd.h>

std::vector<uint8_t> g_bytecode;

// 这些在 adapter_entry.cpp 中声明为 extern
extern bool IsFunctionPatched(FunctionId id);
extern FunctionPatchInfo g_patch_table[];   // 我们需要在 adapter_entry.cpp 中把它暴露出来

// 但是上面那句会有问题，因为 g_patch_table 是 static。
// 所以我们在 adapter_entry.cpp 里把 g_patch_table 的操作封装成函数，
// 在这里不直接访问 g_patch_table，而是在 ApplyPatchDiff 中调用一个 helper 函数。
// 为了简单，这里我们直接重新声明一个函数：
//
// void MarkFunctionPatched(FunctionId id);
//
// 在 adapter_entry.cpp 里提供实现。

extern void MarkFunctionPatched(FunctionId id);

extern "C" void g_trampoline_template();  // 来自 trampoline.s

bool LoadBaselineBytecode(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        std::cerr << "[Runtime] Failed to open baseline bytecode: " << path << "\n";
        return false;
    }

    g_bytecode.assign(std::istreambuf_iterator<char>(ifs),
                      std::istreambuf_iterator<char>());

    ifs.close();

    std::cout << "[Runtime] Loaded baseline bytecode, size=" << g_bytecode.size() << "\n";
    return true;
}

bool ApplyPatchDiff(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        std::cerr << "[Runtime] Failed to open patch diff: " << path << "\n";
        return false;
    }

    while (true) {
        PatchOp op;
        ifs.read(reinterpret_cast<char*>(&op.offset), sizeof(op.offset));
        if (!ifs) break;
        ifs.read(reinterpret_cast<char*>(&op.new_value), sizeof(op.new_value));
        if (!ifs) {
            std::cerr << "[Runtime] Corrupted patch diff file\n";
            return false;
        }

        if (op.offset >= g_bytecode.size()) {
            std::cerr << "[Runtime] Patch offset out of range: " << op.offset << "\n";
            continue;
        }

        uint8_t old = g_bytecode[op.offset];
        g_bytecode[op.offset] = op.new_value;

        std::cout << "[Runtime] Patched bytecode at offset " << op.offset
                  << " from " << (int)old << " to " << (int)op.new_value << "\n";
    }

    // 当前只有一个函数 Test，直接标记为 patched
    MarkFunctionPatched(FunctionId::Test);
    return true;
}

// PatchFunctionEntry：把目标函数入口改为 trampoline 模板 + wrapper_id
bool PatchFunctionEntry(void* func_addr, int wrapper_id) {
#if defined(__aarch64__)

    if (!func_addr) {
        std::cerr << "[Runtime] PatchFunctionEntry: func_addr is null\n";
        return false;
    }
    if (wrapper_id < 0 || wrapper_id >= 0x10000) {
        std::cerr << "[Runtime] PatchFunctionEntry: wrapper_id out of range: "
                  << wrapper_id << "\n";
        return false;
    }

    const uint8_t* tmpl = reinterpret_cast<const uint8_t*>(&g_trampoline_template);
    constexpr size_t kTrampSize = 4 * 4;  // 4 条指令 * 4 字节

    size_t page_size = static_cast<size_t>(::sysconf(_SC_PAGESIZE));
    uintptr_t addr   = reinterpret_cast<uintptr_t>(func_addr);
    uintptr_t page   = addr & ~(page_size - 1);

    if (::mprotect(reinterpret_cast<void*>(page),
                   page_size,
                   PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        perror("[Runtime] mprotect RWX failed");
        return false;
    }

    // 拷贝模板
    std::memcpy(reinterpret_cast<void*>(addr), tmpl, kTrampSize);

    // 修改 mov x0, #0 指令里的 imm16 为 wrapper_id
    // 模板中的布局：adrp(0) / add(4) / mov(8) / br(12)
    uint32_t* mov_inst_ptr = reinterpret_cast<uint32_t*>(addr + 8);
    uint32_t  orig         = *mov_inst_ptr;

    uint32_t imm16 = static_cast<uint32_t>(wrapper_id) & 0xFFFF;
    uint32_t new_inst = (orig & ~0x1FFFE0u) | (imm16 << 5);  // imm16 在 [20:5]

    *mov_inst_ptr = new_inst;

    __builtin___clear_cache(
        reinterpret_cast<char*>(addr),
        reinterpret_cast<char*>(addr + kTrampSize));

    std::cout << "[Runtime] Patched function " << func_addr
              << " with wrapper_id=" << wrapper_id << "\n";
    return true;

#else
    std::cerr << "[Runtime] PatchFunctionEntry only implemented for AArch64.\n";
    return false;
#endif
}
