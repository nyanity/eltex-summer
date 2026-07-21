#ifndef ELCHMOD_H
#define ELCHMOD_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ELCHMOD_OK = 0,
    ELCHMOD_ERR_INVALID_FORMAT,   
    ELCHMOD_ERR_INVALID_MODIFIER, 
    ELCHMOD_ERR_FILE_NOT_FOUND,   
    ELCHMOD_ERR_DB_OPEN,          
    ELCHMOD_ERR_DB_WRITE,         
    ELCHMOD_ERR_DB_NOT_FOUND      
} elchmod_status_t;

elchmod_status_t parse_octal(const char *str, mode_t *mode);
elchmod_status_t parse_symbolic(const char *str, mode_t *mode);
elchmod_status_t apply_modification(mode_t base_mode, const char *mod_str, mode_t *out_mode);

void mode_to_symbolic(mode_t mode, char *out);
void mode_to_octal(mode_t mode, char *out);
void mode_to_binary(mode_t mode, char *out);

elchmod_status_t db_get_mode(const char *path, mode_t *mode);
elchmod_status_t db_set_mode(const char *path, mode_t mode);

#ifdef __cplusplus
}
#endif

#endif // ELCHMOD_H