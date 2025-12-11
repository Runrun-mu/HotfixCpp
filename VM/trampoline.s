// vm_runtime/trampoline.s
//
// 模板跳板：
//   adrp x11, adapter_entry@PAGE
//   add  x11, x11, adapter_entry@PAGEOFF
//   mov  x0, #0      // x0 = wrapper_id，占位，C++ 会把 #0 改为实际值
//   br   x11

    .text
    .align 2
    .globl g_trampoline_template
g_trampoline_template:
    adrp    x11, adapter_entry@PAGE
    add     x11, x11, adapter_entry@PAGEOFF
    mov     x0, #0
    br      x11