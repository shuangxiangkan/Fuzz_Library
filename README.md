# Fuzz_Library

## Introduction

Fuzz_Library is a project designed to perform fuzz testing on C/C++ library APIs, with a particular focus on APIs that include option parameters. For example, consider the `cJSON_ParseWithOpts` function in the cJSON library:

```c
CJSON_PUBLIC(cJSON *) cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);
```

Here, the third parameter `require_null_terminated` controls the behavior of the API by accepting `true` or `false`. The goal of this project is to explore how these option parameters affect the API's behavior and to uncover memory-related bugs (e.g., buffer overflows, use-after-free, memory leaks) by dynamically varying these options during fuzz testing.

Currently, the repository includes three libraries:

- [openjpeg](https://github.com/uclouvain/openjpeg)

More libraries will be added in the future.

---

## Project Structure

Each library in the repository has a `Fuzz` folder, which contains:

- **Fuzz Driver**: A program that invokes APIs with option parameters.
- **Input**: Initial input files for fuzz testing.

---

## Getting Started

### 1. Clone the Libraries

First, clone the libraries you want to test from GitHub. For example:

```bash
git clone https://github.com/uclouvain/openjpeg.git
git clone https://github.com/CESNET/libyang.git
git clone https://github.com/libxls/libxls.git
```

### 2. Instrumented Compilation with AFL++

Use AFL++ to instrument and compile the libraries with `sanitizer=address` enabled. Here’s an example for openjpeg:

```bash
cd openjpeg
mkdir build
cd build
cmake .. -DCMAKE_C_COMPILER=afl-clang-fast -DCMAKE_CXX_COMPILER=afl-clang-fast++ -DCMAKE_C_FLAGS="-fsanitize=address" -DCMAKE_CXX_FLAGS="-fsanitize=address"
make
```

This will generate the library files required for fuzz testing.

### 3. Compile the Fuzz Driver

Place the `Fuzz` folder into the cloned library directory. Then, compile the Fuzz Driver and link it with the generated library files. For example, using `opj_decompress_fuzzer_J2K_afl.cpp` in openjpeg:

```bash
cd openjpeg/Fuzz
afl-clang-fast++ -fsanitize=address -I../src/lib/openjp2 -o opj_decompress_fuzzer_J2K_afl opj_decompress_fuzzer_J2K_afl.cpp -L../../build/bin -lopenjp2
```

### 4. Run AFL++

Use `afl-fuzz` to start the fuzz testing process. For example:

```bash
afl-fuzz -i input -o output ./opj_decompress_fuzzer_J2K_afl @@
```

---

## Writing Fuzz Drivers for New Libraries

If you want to fuzz APIs from other libraries but are unsure how to write a fuzz driver, you can use [oss-fuzz-gen](https://github.com/google/oss-fuzz-gen), a tool developed by Google to automatically generate fuzz drivers for C/C++ libraries. This tool can help you quickly create fuzz drivers for new libraries, which you can then integrate into this project.

---

## Reporting Bugs

If you discover memory-related bugs during fuzz testing, please follow these steps:

1. **Debug and Verify**: Carefully debug the issue to confirm that the bug is caused by the API being fuzzed.
2. **Report the Bug**:
   - Open an issue in the respective library’s repository (e.g., [openjpeg](https://github.com/uclouvain/openjpeg/issues)).
   - Notify us via email:
     - Shuangxiang Kan: [shuangxiang.kan@unsw.edu.au](mailto:shuangxiang.kan@unsw.edu.au)
     - Yuekang Li: [yuekang.li@unsw.edu.au](mailto:yuekang.li@unsw.edu.au)
