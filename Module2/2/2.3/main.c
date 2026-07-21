#include "calculator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void safe_get_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, size, stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        } else {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
    }
}

static int get_int_option(const char *prompt) {
    char buffer[32];
    safe_get_line(prompt, buffer, sizeof(buffer));
    char *endptr;
    long val = strtol(buffer, &endptr, 10);
    if (endptr == buffer || *endptr != '\0') {
        return -1;
    }
    return (int)val;
}

static double get_double_input(const char *prompt, int *success) {
    char buffer[64];
    safe_get_line(prompt, buffer, sizeof(buffer));
    char *endptr;
    double val = strtod(buffer, &endptr);
    if (endptr == buffer || *endptr != '\0') {
        *success = 0;
        return 0.0;
    }
    *success = 1;
    return val;
}

int main(void) {
    size_t cmd_count = 0;
    const calc_command_t *cmds = calc_get_commands(&cmd_count);

    while (1) {
        printf("\n========================================\n");
        printf("         DYNAMIC CALCULATOR (2.3)        \n");
        printf("========================================\n");
        
        for (size_t i = 0; i < cmd_count; i++) {
            printf("%zu. %s (%s)\n", i + 1, cmds[i].name, cmds[i].symbol);
        }
        
        printf("%zu. Exit\n", cmd_count + 1);
        printf("========================================\n");

        int choice = get_int_option("Choose an option: ");
        if (choice == (int)(cmd_count + 1)) {
            printf("\nExiting.\n");
            break;
        }

        if (choice < 1 || (size_t)choice > cmd_count) {
            printf("Error: Invalid option.\n");
            continue;
        }

        int success_a = 0;
        double arg_a = get_double_input("Enter first argument (A): ", &success_a);
        if (!success_a) {
            printf("Error: Invalid numeric input for argument A.\n");
            continue;
        }

        int success_b = 0;
        double arg_b = get_double_input("Enter second argument (B): ", &success_b);
        if (!success_b) {
            printf("Error: Invalid numeric input for argument B.\n");
            continue;
        }

        double result = 0.0;
        calc_op_t operation = cmds[choice - 1].op;
        calc_status_t status = operation(arg_a, arg_b, &result);

        if (status == CALC_OK) {
            printf("\nResult: %g\n", result);
        } else if (status == CALC_ERR_DIV_BY_ZERO) {
            printf("\nError: Division by zero is undefined.\n");
        } else {
            printf("\nError: Operation failed.\n");
        }
    }

    return 0;
}