#include "runtime_api.h"
#include "bytecode_vm.h"
#include <iostream>
#include <vector>

// g_bytecode 在 runtime_api.cpp 中
extern std::vector<uint8_t> g_bytecode;

// patch 状态表
struct FunctionPatchInfo {
    bool patched = false;
};

static FunctionPatchInfo g_patch_table[(int)FunctionId::Count];

void InitPatchTable() {
    for (int i = 0; i < (int)FunctionId::Count; ++i) {
        g_patch_table[i].patched = false;
    }
}

bool IsFunctionPatched(FunctionId id) {
    return g_patch_table[(int)id].patched;
}

void MarkFunctionPatched(FunctionId id) {
    g_patch_table[(int)id].patched = true;
}

// native 实现表
static void (*g_native_impl_table[256])() = {nullptr};

void RegisterNativeImpl(int wrapper_id, void(*fn)()) {
    if (wrapper_id < 0 || wrapper_id >= 256) {
        std::cerr << "[Runtime] RegisterNativeImpl: invalid wrapper_id="
                  << wrapper_id << "\n";
        return;
    }
    g_native_impl_table[wrapper_id] = fn;
}

// C++ adapter
extern "C" void adapter_entry(uint64_t wrapper_id) {
    int id = static_cast<int>(wrapper_id);

    if (id < 0 || id >= 256) {
        std::cerr << "[adapter] invalid wrapper_id=" << id << "\n";
        return;
    }

    auto func_id = static_cast<FunctionId>(id);

    if (!IsFunctionPatched(func_id)) {
        auto fn = g_native_impl_table[id];
        if (!fn) {
            std::cerr << "[adapter] native_impl not registered for id=" << id << "\n";
            return;
        }
        fn();
    } else {
        VmInvoke(func_id);
    }
}

int VmInvoke(FunctionId /*id*/) {
    return ExecBytecode();
}
