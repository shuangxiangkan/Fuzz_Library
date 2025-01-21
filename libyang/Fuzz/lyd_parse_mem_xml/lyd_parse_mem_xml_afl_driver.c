#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "libyang.h"

// Helper function to read options from input data
uint32_t get_options_from_data(const uint8_t* data, size_t* offset, size_t max_size) {
    uint32_t options = 0;
    if (*offset + sizeof(uint32_t) <= max_size) {
        memcpy(&options, data + *offset, sizeof(uint32_t));
        *offset += sizeof(uint32_t);
    }
    return options;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 0;
    }

    FILE* file = fopen(argv[1], "r");
    if (!file) return 0;

    if (fseeko(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "fseeko error!\n");
        return 0;
    }
    size_t size = ftello(file);

    if (fseeko(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "fseeko error!\n");
        return 0;
    }

    uint8_t* input_data = (uint8_t*)malloc(size);
    if (!input_data) {
        fclose(file);
        return 0;
    }

    if (fread(input_data, 1, size, file) != size) {
        free(input_data);
        fclose(file);
        return 0;
    }
    fclose(file);

    // Keep track of where we are in the input data
    size_t offset = 0;

    struct ly_ctx *ctx = NULL;
    static bool log = false;

    // Get options for ly_log_options from input
    uint32_t log_opts = get_options_from_data(input_data, &offset, size);
    if (!log) {
        ly_log_options(log_opts);
        log = true;
    }

    // Get options for ly_ctx_new from input
    uint32_t ctx_opts = get_options_from_data(input_data, &offset, size);
    LY_ERR err = ly_ctx_new(NULL, ctx_opts, &ctx);
    if (err != LY_SUCCESS) {
        free(input_data);
        return 0;
    }

    const char *schema_a =
            "module defs {namespace urn:tests:defs;prefix d;yang-version 1.1;"
            "identity crypto-alg; identity interface-type; identity ethernet {base interface-type;}"
            "identity fast-ethernet {base ethernet;}}";
    const char *schema_b =
            "module types {namespace urn:tests:types;prefix t;yang-version 1.1; import defs {prefix defs;}"
            "feature f; identity gigabit-ethernet { base defs:ethernet;}"
            // ... [rest of schema_b definition remains the same]
            "type string {length 1..20;}}}}";

    // Parse schemas - note that we don't fuzz the module parameter as it's for output
    struct lys_module *module_a = NULL;
    if (lys_parse_mem(ctx, schema_a, LYS_IN_YANG, &module_a) != LY_SUCCESS) {
        ly_ctx_destroy(ctx);
        free(input_data);
        return 0;
    }

    struct lys_module *module_b = NULL;
    if (lys_parse_mem(ctx, schema_b, LYS_IN_YANG, &module_b) != LY_SUCCESS) {
        ly_ctx_destroy(ctx);
        free(input_data);
        return 0;
    }

    // The remaining data is our YANG data to parse
    if (offset >= size) {
        ly_ctx_destroy(ctx);
        free(input_data);
        return 0;
    }

    size_t data_size = size - offset;
    char* yang_data = malloc(data_size + 1);
    if (!yang_data) {
        ly_ctx_destroy(ctx);
        free(input_data);
        return 0;
    }
    memcpy(yang_data, input_data + offset, data_size);
    yang_data[data_size] = 0;

    // Get options for lyd_parse_data_mem from input
    uint32_t format_opts = get_options_from_data(input_data, &offset, size);
    uint32_t parse_data_opts = get_options_from_data(input_data, &offset, size);
    uint32_t validate_opts = get_options_from_data(input_data, &offset, size);

    struct lyd_node *tree = NULL;
    lyd_parse_data_mem(ctx, yang_data, format_opts, parse_data_opts, validate_opts, &tree);

    // Cleanup
    lyd_free_all(tree);
    ly_ctx_destroy(ctx);
    free(yang_data);
    free(input_data);

    return 0;
}