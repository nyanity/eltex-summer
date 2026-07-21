#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

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
} calc_plugin_t;

#endif // PLUGIN_INTERFACE_H