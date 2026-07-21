#include "../plugin_interface.h"

static calc_status_t calc_div(double a, double b, double *result) {
    if (!result) return CALC_ERR_INVALID_OP;
    if (b == 0.0) return CALC_ERR_DIV_BY_ZERO;
    *result = a / b;
    return CALC_OK;
}

calc_plugin_t plugin_info = {
    "Division",
    "/",
    calc_div
};