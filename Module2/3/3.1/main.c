#include "elchmod.h"
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static void print_permissions(mode_t mode) {
    char sym[10];
    char oct[4];
    char bin[12];
    mode_to_symbolic(mode, sym);
    mode_to_octal(mode, oct);
    mode_to_binary(mode, bin);
    printf("Symbolic: %s\n", sym);
    printf("Octal:    %s\n", oct);
    printf("Binary:   %s\n", bin);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  1) %s <mask_or_symbolic>\n", argv[0]);
        fprintf(stderr, "  2) %s <filename>\n", argv[0]);
        fprintf(stderr, "  3) %s <modification> <filename>\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        struct stat sb;
        if (stat(argv[1], &sb) == 0) {
            mode_t mode;
            if (db_get_mode(argv[1], &mode) != ELCHMOD_OK) {
                mode = sb.st_mode & 0777;
            }
            printf("File: %s\n", argv[1]);
            print_permissions(mode);
            return 0;
        }

        mode_t mode;
        if (parse_octal(argv[1], &mode) == ELCHMOD_OK || parse_symbolic(argv[1], &mode) == ELCHMOD_OK) {
            printf("Mask input: %s\n", argv[1]);
            print_permissions(mode);
            return 0;
        }

        fprintf(stderr, "Error: '%s' is neither an existing file nor a valid permission mask.\n", argv[1]);
        return 1;
    }

    if (argc == 3) {
        struct stat sb;
        if (stat(argv[2], &sb) != 0) {
            fprintf(stderr, "Error: File '%s' not found.\n", argv[2]);
            return 1;
        }

        mode_t base_mode;
        if (db_get_mode(argv[2], &base_mode) != ELCHMOD_OK) {
            base_mode = sb.st_mode & 0777;
        }

        mode_t modified_mode;
        elchmod_status_t status = apply_modification(base_mode, argv[1], &modified_mode);
        if (status != ELCHMOD_OK) {
            fprintf(stderr, "Error: Invalid modification expression '%s' (status code: %d).\n", argv[1], status);
            return 1;
        }

        status = db_set_mode(argv[2], modified_mode);
        if (status != ELCHMOD_OK) {
            fprintf(stderr, "Warning: Failed to save simulated permissions to state DB (status code: %d).\n", status);
        }

        printf("File: %s\n", argv[2]);
        printf("--- Original Permissions ---\n");
        print_permissions(base_mode);
        printf("--- Modified Permissions (Saved to State DB) ---\n");
        print_permissions(modified_mode);
        return 0;
    }

    fprintf(stderr, "Error: Too many arguments.\n");
    return 1;
}