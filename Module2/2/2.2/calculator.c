#include "calculator.h"
#include <math.h>

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