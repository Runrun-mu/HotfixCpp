#pragma once

#include <string>
#include "common/func_ids.h"

// 初始化 Patch 表
void InitPatchTable();

// baseline 字节码加载 & diff patch
bool LoadBaselineBytecode(const std::string& path);
bool ApplyPatchDiff(const std::string& path);

// 查询某函数是否已经被 patch
bool IsFunctionPatched(FunctionId id);

// VM 调用（目前不带参数）
int VmInvoke(FunctionId id);

// 注册 native 实现（由 interpreter 生成的代码调用）
void RegisterNativeImpl(int wrapper_id, void(*fn)());

// 打补丁：把目标函数入口改为 trampoline（AArch64）
bool PatchFunctionEntry(void* func_addr, int wrapper_id);