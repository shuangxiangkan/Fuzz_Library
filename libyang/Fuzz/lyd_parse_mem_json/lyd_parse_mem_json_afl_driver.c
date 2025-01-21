#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "libyang.h"

// Helper function to extract options from input data
uint32_t extract_options(const uint8_t *data, size_t size, size_t offset, uint32_t valid_options_mask) {
    if (offset + sizeof(uint32_t) > size) {
        return 0; // Default to no options if out of bounds
    }
    uint32_t extracted_options = *(uint32_t *)(data + offset);
    return extracted_options & valid_options_mask; // Apply a mask to ensure only valid bits are used
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Read the input file
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    if (fseeko(file, 0, SEEK_END) != 0) {
        perror("fseeko error");
        fclose(file);
        return EXIT_FAILURE;
    }

    size_t size = ftello(file);
    if (fseeko(file, 0, SEEK_SET) != 0) {
        perror("fseeko error");
        fclose(file);
        return EXIT_FAILURE;
    }

    uint8_t *data = (uint8_t *)malloc(size);
    if (!data) {
        perror("Memory allocation error");
        fclose(file);
        return EXIT_FAILURE;
    }

    if (fread(data, 1, size, file) != size) {
        perror("Failed to read input file");
        free(data);
        fclose(file);
        return EXIT_FAILURE;
    }
    fclose(file);

    // Initialize libyang context
    struct ly_ctx *ctx = NULL;
    static bool log = false;
    LY_ERR err;

    if (!log) {
        ly_log_options(0);
        log = true;
    }

    // Extract options for `ly_ctx_new`
    uint32_t ctx_options = extract_options(data, size, 0, LY_CTX_NO_YANGLIBRARY | LY_CTX_DISABLE_SEARCHDIRS);
    err = ly_ctx_new(NULL, ctx_options, &ctx);
    if (err != LY_SUCCESS) {
        fprintf(stderr, "Failed to create context\n");
        free(data);
        return EXIT_FAILURE;
    }

    // Prepare schemas
    const char *schema_a =
        "module defs {namespace urn:tests:defs;prefix d;yang-version 1.1;"
        "identity crypto-alg; identity interface-type; identity ethernet {base interface-type;}"
        "identity fast-ethernet {base ethernet;}}";
    const char *schema_b =
        "module types {namespace urn:tests:types;prefix t;yang-version 1.1; import defs {prefix defs;}"
        "feature f; identity gigabit-ethernet { base defs:ethernet;}"
        "container cont {leaf leaftarget {type empty;}"
        "list listtarget {key id; max-elements 5;leaf id {type uint8;} leaf value {type string;}}"
        "leaf-list leaflisttarget {type uint8; max-elements 5;}}"
        "list list {key id; leaf id {type string;} leaf value {type string;} leaf-list targets {type string;}}"
        "list list2 {key \"id value\"; leaf id {type string;} leaf value {type string;}}"
        "list list_inst {key id; leaf id {type instance-identifier {require-instance true;}} leaf value {type string;}}"
        "list list_ident {key id; leaf id {type identityref {base defs:interface-type;}} leaf value {type string;}}"
        "leaf-list leaflisttarget {type string;}"
        "leaf binary {type binary {length 5 {error-message \"This base64 value must be of length 5.\";}}}"
        "leaf binary-norestr {type binary;}"
        "leaf int8 {type int8 {range 10..20;}}"
        "leaf uint8 {type uint8 {range 150..200;}}"
        "leaf int16 {type int16 {range -20..-10;}}"
        "leaf uint16 {type uint16 {range 150..200;}}"
        "leaf int32 {type int32;}"
        "leaf uint32 {type uint32;}"
        "leaf int64 {type int64;}"
        "leaf uint64 {type uint64;}"
        "leaf bits {type bits {bit zero; bit one {if-feature f;} bit two;}}"
        "leaf enums {type enumeration {enum white; enum yellow {if-feature f;}}}"
        "leaf dec64 {type decimal64 {fraction-digits 1; range 1.5..10;}}"
        "leaf dec64-norestr {type decimal64 {fraction-digits 18;}}"
        "leaf str {type string {length 8..10; pattern '[a-z ]*';}}"
        "leaf str-norestr {type string;}"
        "leaf str-utf8 {type string{length 2..5; pattern '€*';}}"
        "leaf bool {type boolean;}"
        "leaf empty {type empty;}"
        "leaf ident {type identityref {base defs:interface-type;}}"
        "leaf inst {type instance-identifier {require-instance true;}}"
        "leaf inst-noreq {type instance-identifier {require-instance false;}}"
        "leaf lref {type leafref {path /leaflisttarget; require-instance true;}}"
        "leaf lref2 {type leafref {path \"../list[id = current()/../str-norestr]/targets\"; require-instance true;}}"
        "leaf un1 {type union {"
        "type leafref {path /int8; require-instance true;}"
        "type union { type identityref {base defs:interface-type;} type instance-identifier {require-instance true;} }"
        "type string {length 1..20;}}}}";

    // Parse schemas
    struct lys_module *module_a = NULL;
    struct lys_module *module_b = NULL;
    lys_parse_mem(ctx, schema_a, LYS_IN_YANG, &module_a);
    lys_parse_mem(ctx, schema_b, LYS_IN_YANG, &module_b);

    // Extract options for `lyd_parse_data_mem`
    uint32_t data_options = extract_options(data, size, 4, LYD_PARSE_ONLY | LYD_VALIDATE_PRESENT);
    struct lyd_node *tree = NULL;

    char *data_copy = (char *)malloc(size + 1);
    if (data_copy == NULL) {
        ly_ctx_destroy(ctx);
        free(data);
        return EXIT_FAILURE;
    }
    memcpy(data_copy, data, size);
    data_copy[size] = '\0';

    lyd_parse_data_mem(ctx, data_copy, LYD_JSON, data_options, LYD_VALIDATE_PRESENT, &tree);

    // Cleanup
    lyd_free_all(tree);
    ly_ctx_destroy(ctx);
    free(data_copy);
    free(data);

    return EXIT_SUCCESS;
}
