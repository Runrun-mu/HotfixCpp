#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include "common/opcodes.h"
#include "common/diff_format.h"

namespace fs = std::filesystem;

// 读取文件
std::string read_file(const fs::path& path) {
    std::ifstream ifs(path);
    if (!ifs) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

// 解析 int a = X; 中的 X
int parse_const_from_test(const std::string& src) {
    auto pos = src.find("int a");
    if (pos == std::string::npos) {
        throw std::runtime_error("Cannot find 'int a' in Test.cpp");
    }
    pos = src.find('=', pos);
    if (pos == std::string::npos) {
        throw std::runtime_error("Cannot find '=' after 'int a'");
    }
    ++pos;
    while (pos < src.size() && std::isspace((unsigned char)src[pos])) ++pos;

    int value = 0;
    bool has_digit = false;
    while (pos < src.size() && std::isdigit((unsigned char)src[pos])) {
        has_digit = true;
        value = value * 10 + (src[pos] - '0');
        ++pos;
    }
    if (!has_digit) {
        throw std::runtime_error("No digit found after '=' in 'int a = ...'");
    }
    return value;
}

// 提取 Test 函数体（大括号之间）
std::string extract_test_body(const std::string& src) {
    auto pos = src.find("void Test");
    if (pos == std::string::npos) {
        throw std::runtime_error("Cannot find 'void Test' in Test.cpp");
    }
    pos = src.find('{', pos);
    if (pos == std::string::npos) {
        throw std::runtime_error("Cannot find '{' after 'void Test'");
    }
    ++pos; // 跳过 '{'
    auto end = src.rfind('}');
    if (end == std::string::npos || end <= pos) {
        throw std::runtime_error("Cannot find matching '}' for Test body");
    }
    std::string body = src.substr(pos, end - pos);
    return body;
}

// baseline 字节码生成：ICONST value, PRINT, RET
void generate_baseline_bc(const fs::path& out_path, int value) {
    std::vector<uint8_t> bc;
    bc.push_back(OP_ICONST);
    bc.push_back(static_cast<uint8_t>(value));
    bc.push_back(OP_PRINT);
    bc.push_back(OP_RET);

    std::ofstream ofs(out_path, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Failed to open baseline bytecode for write: " + out_path.string());
    }
    ofs.write(reinterpret_cast<const char*>(bc.data()),
              (std::streamsize)bc.size());
    ofs.close();

    std::cout << "[Interpreter] baseline_bc.bin written (" << bc.size() << " bytes), value=" << value << "\n";
}

// 生成 Test_native.cpp：native_impl + RegisterNativeImpl
void generate_test_native(const fs::path& out_path,
                          const std::string& test_body,
                          int wrapper_id) {
    std::ofstream ofs(out_path);
    if (!ofs) {
        throw std::runtime_error("Failed to open Test_native.cpp for write: " + out_path.string());
    }

    ofs <<
R"(#include <iostream>
#include "vm_runtime/runtime_api.h"
#include "common/func_ids.h"

static void Test_native_impl() {
)";

    ofs << test_body << "\n}\n\n";

    ofs <<
R"(static void __register_Test_native() {
    RegisterNativeImpl()";

    ofs << "(" << wrapper_id << ", &Test_native_impl);\n}\n\n";

    ofs <<
R"(struct TestNativeRegistrator {
    TestNativeRegistrator() { __register_Test_native(); }
};
static TestNativeRegistrator s_test_native_reg;
)";

    ofs.close();
    std::cout << "[Interpreter] Generated " << out_path.string() << "\n";
}

// 生成 main_hotfix.cpp：Patch + 两次调用 Test()
void generate_main_hotfix(const fs::path& out_path, int wrapper_id) {
    std::ofstream ofs(out_path);
    if (!ofs) {
        throw std::runtime_error("Failed to open main_hotfix.cpp for write: " + out_path.string());
    }

    ofs <<
R"(#include <iostream>
#include <string>
#include "vm_runtime/runtime_api.h"

extern "C" void Test();  // 原始函数（来自 user_src/Test.cpp）

int main() {
    InitPatchTable();

    if (!LoadBaselineBytecode("baseline_bc.bin")) {
        std::cerr << "Failed to load baseline_bc.bin. Please run interpreter --mode=native first.\n";
        return 1;
    }

    // 对 Test 的入口打 AArch64 trampoline patch
    if (!PatchFunctionEntry((void*)&Test, )";

    ofs << wrapper_id << R"() {
        std::cerr << "PatchFunctionEntry failed\n";
        return 1;
    }

    std::cout << "=== First run (native via adapter) ===\n";
    Test();

    std::cout << "\n*** 请在另一个终端运行 interpreter --mode=patch 生成 patch_diff.bin，"
                 "然后回到这里按 Enter 继续 ***\n";
    std::cin.get();

    if (!ApplyPatchDiff("patch_diff.bin")) {
        std::cerr << "Failed to apply patch_diff.bin\n";
        return 1;
    }

    std::cout << "=== Second run (VM, patched) ===\n";
    Test();

    return 0;
}
)";

    ofs.close();
    std::cout << "[Interpreter] Generated " << out_path.string() << "\n";
}

// 生成 patch_diff.bin：只把 offset=1 的 imm 改为 new_value
void generate_patch_diff(const fs::path& out_path, int new_value) {
    PatchOp op;
    op.offset    = 1;  // ICONST 的 imm 在 offset=1
    op.new_value = static_cast<uint8_t>(new_value);

    std::ofstream ofs(out_path, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Failed to open patch_diff.bin for write: " + out_path.string());
    }

    ofs.write(reinterpret_cast<const char*>(&op.offset), sizeof(op.offset));
    ofs.write(reinterpret_cast<const char*>(&op.new_value), sizeof(op.new_value));
    ofs.close();

    std::cout << "[Interpreter] patch_diff.bin written (offset=" << op.offset
              << ", new=" << (int)op.new_value << ")\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage:\n"
                  << "  interpreter --mode=native\n"
                  << "  interpreter --mode=patch\n";
        return 0;
    }

    std::string mode = argv[1];

    try {
        // 当前工作目录通常为 build/Interpreter/<config>，向上三级即为工程根目录
        fs::path build_dir = fs::current_path();
        fs::path project_root = fs::weakly_canonical(build_dir / ".." / ".." / "..");
        fs::path user_src_dir = project_root / "Test";
        fs::path game_gen_dir = project_root / "GeneratedCode";

        fs::create_directories(game_gen_dir);

        fs::path test_src_path = user_src_dir / "Test.cpp";

        constexpr int kWrapperIdForTest = 0;

        if (mode == "--mode=native") {
            std::string test_src = read_file(test_src_path);
            int value = parse_const_from_test(test_src);
            std::string body = extract_test_body(test_src);

            generate_baseline_bc(project_root / "baseline_bc.bin", value);
            generate_test_native(game_gen_dir / "Test_native.cpp", body, kWrapperIdForTest);
            generate_main_hotfix(game_gen_dir / "main_hotfix.cpp", kWrapperIdForTest);

        } else if (mode == "--mode=patch") {
            std::string test_src = read_file(test_src_path);
            int new_value = parse_const_from_test(test_src);
            generate_patch_diff(project_root / "patch_diff.bin", new_value);

        } else {
            std::cout << "Unknown mode: " << mode << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "[Interpreter] Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
