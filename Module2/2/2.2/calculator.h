#ifndef CALCULATOR_H
#define CALCULATOR_H

typedef enum {
    CALC_OK = 0,
    CALC_ERR_DIV_BY_ZERO,
    CALC_ERR_INVALID_OP
} calc_status_t;

calc_status_t calc_add(double a, double b, double *result);
calc_status_t calc_sub(double a, double b, double *result);
calc_status_t calc_mul(double a, double b, double *result);
calc_status_t calc_div(double a, double b, double *result);

#endif // CALCULATOR_H