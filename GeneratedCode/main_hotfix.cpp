#include <iostream>
#include <string>
#include "vm_runtime/runtime_api.h"

extern "C" void Test();  // 原始函数（来�?user_src/Test.cpp�?

int main(){ 
    InitPatchTable();

    if (!LoadBaselineBytecode("baseline_bc.bin")) {
        std::cerr << "Failed to load baseline_bc.bin. Please run interpreter --mode=native first.\n";
        return 1;
    }

    // �?Test 的入口打 AArch64 trampoline patch
    if(!PatchFunctionEntry((void*)&Test, 0)) {
        std::cerr << "PatchFunctionEntry failed\n";
        return 1;
    }

    std::cout << "=== First run (native via adapter) ===\n";
    Test();

    std::cout << "\n*** 请在另一个终端运�?interpreter --mode=patch 生成 patch_diff.bin�?"
                 "然后回到这里�?Enter 继续 ***\n";
    std::cin.get();

    if (!ApplyPatchDiff("patch_diff.bin")) {
        std::cerr << "Failed to apply patch_diff.bin\n";
        return 1;
    }

    std::cout << "=== Second run (VM, patched) ===\n";
    Test();

    return 0;
}
