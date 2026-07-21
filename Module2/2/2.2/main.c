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
    while (1) {
        printf("\n========================================\n");
        printf("               CALCULATOR               \n");
        printf("========================================\n");
        printf("1. Addition (+)\n");
        printf("2. Subtraction (-)\n");
        printf("3. Multiplication (*)\n");
        printf("4. Division (/)\n");
        printf("5. Exit\n");
        printf("========================================\n");

        int choice = get_int_option("Choose an option (1-5): ");
        if (choice == 5) {
            printf("\nExiting.\n");
            break;
        }

        if (choice < 1 || choice > 4) {
            printf("Error: Invalid option. Please select 1-5.\n");
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
        calc_status_t status = CALC_OK;

        switch (choice) {
            case 1:
                status = calc_add(arg_a, arg_b, &result);
                break;
            case 2:
                status = calc_sub(arg_a, arg_b, &result);
                break;
            case 3:
                status = calc_mul(arg_a, arg_b, &result);
                break;
            case 4:
                status = calc_div(arg_a, arg_b, &result);
                break;
            default:
                status = CALC_ERR_INVALID_OP;
                break;
        }

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