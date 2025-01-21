#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "libyang.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        perror("Failed to open input file");
        return 1;
    }

    if (fseeko(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "fseeko error!\n");
        fclose(file);
        return 1;
    }

    size_t size = ftello(file);
    if (fseeko(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "fseeko error!\n");
        fclose(file);
        return 1;
    }

    if (size == 0) {
        fprintf(stderr, "Input file is empty\n");
        fclose(file);
        return 1;
    }

    uint8_t* data = (uint8_t*)malloc(size);
    if (!data) {
        perror("Failed to allocate memory");
        fclose(file);
        return 1;
    }

    if (fread(data, 1, size, file) != size) {
        perror("Failed to read input file");
        free(data);
        fclose(file);
        return 1;
    }
    fclose(file);

    // 动态生成选项
    uint32_t ctx_options = size % 0xFFFF; // 根据文件大小生成一个 16 位的选项值
    uint32_t format_option = size % 10;   // 根据文件大小决定格式（0, 1, 2 对应合法的 LYS_IN_* 枚举）
    const char* yang_data = (const char*)data;
    size_t yang_data_len = size;

    struct ly_ctx* ctx = NULL;
    LY_ERR err = ly_ctx_new(NULL, ctx_options, &ctx);
    if (err != LY_SUCCESS) {
        fprintf(stderr, "Failed to create context with options: 0x%X\n", ctx_options);
        free(data);
        return 1;
    }

    ly_log_options(size % 8); // 动态设置日志选项：0-禁用，1-错误，2-警告，3-调试

    // 为 YANG 数据添加终止符
    char* yang_buffer = (char*)malloc(yang_data_len + 1);
    if (!yang_buffer) {
        perror("Failed to allocate memory for YANG data");
        ly_ctx_destroy(ctx);
        free(data);
        return 1;
    }
    memcpy(yang_buffer, yang_data, yang_data_len);
    yang_buffer[yang_data_len] = '\0';

    // 解析 YANG 数据
    lys_parse_mem(ctx, yang_buffer, format_option, NULL);

    // 释放资源
    free(yang_buffer);
    ly_ctx_destroy(ctx);
    free(data);

    return 0;
}
