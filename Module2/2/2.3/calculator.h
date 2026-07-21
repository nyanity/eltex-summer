#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stddef.h>

typedef enum {
    CALC_OK = 0,
    CALC_ERR_DIV_BY_ZERO,
    CALC_ERR_INVALID_OP
} calc_status_t;

typedef calc_status_t (*calc_op_t)(double a, double b, double *result);

typedef struct {
    const char *name;
    const char *symbol;
    calc_op_t op;
} calc_command_t;

calc_status_t calc_add(double a, double b, double *result);
calc_status_t calc_sub(double a, double b, double *result);
calc_status_t calc_mul(double a, double b, double *result);
calc_status_t calc_div(double a, double b, double *result);
calc_status_t calc_max(double a, double b, double *result);

const calc_command_t* calc_get_commands(size_t *count);

#endif // CALCULATOR_H