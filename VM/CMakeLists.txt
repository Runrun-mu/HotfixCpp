add_library(vm_runtime STATIC
    runtime_api.cpp
    bytecode_vm.cpp
    trampoline.s
    adapter_entry.cpp
)

target_include_directories(vm_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}
)