# HotfixCpp

## Interpreter usage
- Configure build files out-of-source: `cmake -S . -B build`
- Build the interpreter executable: `cmake --build build --target interpreter`
- Run the tool from the build output dir (e.g. `build/Interpreter/Debug` on MSVC or `build/Interpreter` for single-config generators).

### Expected paths
- Project root: the directory containing `CMakeLists.txt`.
- Source to inspect: `Test/Test.cpp` under the project root.
- Generated code output: `GeneratedCode/` under the project root.
- Baseline/patch artifacts: `baseline_bc.bin` and `patch_diff.bin` under the project root.

### Workflow
- First pass (generate baseline and native glue): `interpreter --mode=native`
  - Reads `Test/Test.cpp`, extracts `int a = ...` and the body of `Test()`.
  - Writes `baseline_bc.bin` in the project root.
  - Emits `GeneratedCode/Test_native.cpp` and `GeneratedCode/main_hotfix.cpp`.
- After you edit `Test/Test.cpp`, produce a patch diff: `interpreter --mode=patch`
  - Writes `patch_diff.bin` in the project root using the new constant value.

## Interpreter 使用说明（中文）
- 生成构建文件（源外构建）：`cmake -S . -B build`
- 编译 interpreter 可执行程序：`cmake --build build --target interpreter`
- 从生成目录运行（例如 MSVC 下 `build/Interpreter/Debug`，单配置生成器则在 `build/Interpreter`）。

### 路径约定
- 工程根目录：包含顶层 `CMakeLists.txt` 的目录。
- 被分析的源文件：工程根目录下的 `Test/Test.cpp`。
- 生成代码输出目录：工程根目录下的 `GeneratedCode/`。
- 基线与补丁文件：工程根目录下的 `baseline_bc.bin` 与 `patch_diff.bin`。

### 工作流程
- 第一阶段（生成基线与 native 适配层）：`interpreter --mode=native`
  - 读取 `Test/Test.cpp`，提取 `int a = ...` 的值和 `Test()` 的函数体。
  - 在工程根目录写出 `baseline_bc.bin`。
  - 在 `GeneratedCode/` 写出 `Test_native.cpp` 和 `main_hotfix.cpp`。
- 修改 `Test/Test.cpp` 后生成补丁：`interpreter --mode=patch`
  - 根据新的常量值在工程根目录写出 `patch_diff.bin`。
