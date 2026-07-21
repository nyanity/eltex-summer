#include "calculator.h"

calc_status_t calc_add(double a, double b, double *result) {
    if (!result) {
        return CALC_ERR_INVALID_OP;
    }
    *result = a + b;
    return CALC_OK;
}

calc_status_t calc_sub(double a, double b, double *result) {
    if (!result) {
        return CALC_ERR_INVALID_OP;
    }
    *result = a - b;
    return CALC_OK;
}

calc_status_t calc_mul(double a, double b, double *result) {
    if (!result) {
        return CALC_ERR_INVALID_OP;
    }
    *result = a * b;
    return CALC_OK;
}

calc_status_t calc_div(double a, double b, double *result) {
    if (!result) {
        return CALC_ERR_INVALID_OP;
    }
    if (b == 0.0) {
        return CALC_ERR_DIV_BY_ZERO;
    }
    *result = a / b;
    return CALC_OK;
}

calc_status_t calc_max(double a, double b, double *result) {
    if (!result) {
        return CALC_ERR_INVALID_OP;
    }
    *result = (a > b) ? a : b;
    return CALC_OK;
}

static const calc_command_t commands[] = {
    {"Addition", "+", calc_add},
    {"Subtraction", "-", calc_sub},
    {"Multiplication", "*", calc_mul},
    {"Division", "/", calc_div},
    {"Maximum", "max", calc_max}
};

const calc_command_t* calc_get_commands(size_t *count) {
    if (count) {
        *count = sizeof(commands) / sizeof(commands[0]);
    }
    return commands;
}