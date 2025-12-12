#include <iostream>
#include "vm_runtime/runtime_api.h"
#include "common/func_ids.h"

static void Test_native_impl() {

    int a = 0;              // 初始为 0，后面你改成 1
    std::cout << a << std::endl;

}

static void __register_Test_native() {
    RegisterNativeImpl((0, &Test_native_impl);
}

struct TestNativeRegistrator {
    TestNativeRegistrator() { __register_Test_native(); }
};
static TestNativeRegistrator s_test_native_reg;
