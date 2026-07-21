#include "../plugin_interface.h"

static calc_status_t calc_add(double a, double b, double *result) {
    if (!result) return CALC_ERR_INVALID_OP;
    *result = a + b;
    return CALC_OK;
}

calc_plugin_t plugin_info = {
    "Addition",
    "+",
    calc_add
};