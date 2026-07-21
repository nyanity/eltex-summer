#include "elchmod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>

elchmod_status_t parse_octal(const char *str, mode_t *mode) {
    if (!str || !mode) return ELCHMOD_ERR_INVALID_FORMAT;
    char *endptr;
    long val = strtol(str, &endptr, 8);
    if (endptr == str || *endptr != '\0' || val < 0 || val > 0777) {
        return ELCHMOD_ERR_INVALID_FORMAT;
    }
    *mode = (mode_t)val;
    return ELCHMOD_OK;
}

elchmod_status_t parse_symbolic(const char *str, mode_t *mode) {
    if (!str || !mode || strlen(str) != 9) return ELCHMOD_ERR_INVALID_FORMAT;
    mode_t m = 0;
    
    if (str[0] == 'r') m |= S_IRUSR; else if (str[0] != '-') return ELCHMOD_ERR_INVALID_FORMAT;
    if (str[1] == 'w') m |= S_IWUSR; else if (str[1] != '-') return ELCHMOD_ERR_INVALID_FORMAT;
    if (str[2] == 'x') m |= S_IXUSR; else if (str[2] != '-') return ELCHMOD_ERR_INVALID_FORMAT;
    
    if (str[3] == 'r') m |= S_IRGRP; else if (str[3] != '-') return ELCHMOD_ERR_INVALID_FORMAT;
    if (str[4] == 'w') m |= S_IWGRP; else if (str[4] != '-') return ELCHMOD_ERR_INVALID_FORMAT;
    if (str[5] == 'x') m |= S_IXGRP; else if (str[5] != '-') return ELCHMOD_ERR_INVALID_FORMAT;
    
    if (str[6] == 'r') m |= S_IROTH; else if (str[6] != '-') return ELCHMOD_ERR_INVALID_FORMAT;
    if (str[7] == 'w') m |= S_IWOTH; else if (str[7] != '-') return ELCHMOD_ERR_INVALID_FORMAT;
    if (str[8] == 'x') m |= S_IXOTH; else if (str[8] != '-') return ELCHMOD_ERR_INVALID_FORMAT;
    
    *mode = m;
    return ELCHMOD_OK;
}

elchmod_status_t apply_modification(mode_t base_mode, const char *mod_str, mode_t *out_mode) {
    if (!mod_str || !out_mode) return ELCHMOD_ERR_INVALID_MODIFIER;
    
    if (parse_octal(mod_str, out_mode) == ELCHMOD_OK) {
        return ELCHMOD_OK;
    }

    const char *p = mod_str;
    int modify_u = 0, modify_g = 0, modify_o = 0;
    int has_targets = 0;
    
    while (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a') {
        has_targets = 1;
        if (*p == 'u') modify_u = 1;
        if (*p == 'g') modify_g = 1;
        if (*p == 'o') modify_o = 1;
        if (*p == 'a') {
            modify_u = 1;
            modify_g = 1;
            modify_o = 1;
        }
        p++;
    }
    if (!has_targets) {
        modify_u = 1;
        modify_g = 1;
        modify_o = 1;
    }

    char op = *p;
    if (op != '+' && op != '-' && op != '=') {
        return ELCHMOD_ERR_INVALID_MODIFIER;
    }
    p++;

    mode_t u_mask = 0, g_mask = 0, o_mask = 0;
    while (*p) {
        if (*p == 'r') {
            if (modify_u) u_mask |= S_IRUSR;
            if (modify_g) g_mask |= S_IRGRP;
            if (modify_o) o_mask |= S_IROTH;
        } else if (*p == 'w') {
            if (modify_u) u_mask |= S_IWUSR;
            if (modify_g) g_mask |= S_IWGRP;
            if (modify_o) o_mask |= S_IWOTH;
        } else if (*p == 'x') {
            if (modify_u) u_mask |= S_IXUSR;
            if (modify_g) g_mask |= S_IXGRP;
            if (modify_o) o_mask |= S_IXOTH;
        } else {
            return ELCHMOD_ERR_INVALID_MODIFIER;
        }
        p++;
    }

    mode_t result = base_mode;
    if (op == '+') {
        result |= (u_mask | g_mask | o_mask);
    } else if (op == '-') {
        result &= ~(u_mask | g_mask | o_mask);
    } else if (op == '=') {
        mode_t clear_mask = 0;
        if (modify_u) clear_mask |= (S_IRUSR | S_IWUSR | S_IXUSR);
        if (modify_g) clear_mask |= (S_IRGRP | S_IWGRP | S_IXGRP);
        if (modify_o) clear_mask |= (S_IROTH | S_IWOTH | S_IXOTH);
        
        result &= ~clear_mask;
        result |= (u_mask | g_mask | o_mask);
    }

    *out_mode = result & 0777;
    return ELCHMOD_OK;
}

void mode_to_symbolic(mode_t mode, char *out) {
    out[0] = (mode & S_IRUSR) ? 'r' : '-';
    out[1] = (mode & S_IWUSR) ? 'w' : '-';
    out[2] = (mode & S_IXUSR) ? 'x' : '-';
    out[3] = (mode & S_IRGRP) ? 'r' : '-';
    out[4] = (mode & S_IWGRP) ? 'w' : '-';
    out[5] = (mode & S_IXGRP) ? 'x' : '-';
    out[6] = (mode & S_IROTH) ? 'r' : '-';
    out[7] = (mode & S_IWOTH) ? 'w' : '-';
    out[8] = (mode & S_IXOTH) ? 'x' : '-';
    out[9] = '\0';
}

void mode_to_octal(mode_t mode, char *out) {
    sprintf(out, "%03o", (unsigned int)(mode & 0777));
}

void mode_to_binary(mode_t mode, char *out) {
    int idx = 0;
    for (int i = 8; i >= 0; i--) {
        out[idx++] = (mode & (1 << i)) ? '1' : '0';
        if (i == 6 || i == 3) {
            out[idx++] = ' ';
        }
    }
    out[idx] = '\0';
}

elchmod_status_t db_get_mode(const char *path, mode_t *mode) {
    if (!path || !mode) return ELCHMOD_ERR_INVALID_FORMAT;
    char abs_path[PATH_MAX];
    if (!realpath(path, abs_path)) {
        return ELCHMOD_ERR_FILE_NOT_FOUND;
    }

    char db_path[PATH_MAX];
    const char *home = getenv("HOME");
    if (!home) return ELCHMOD_ERR_DB_OPEN;
    snprintf(db_path, sizeof(db_path), "%s/.elchmod_db", home);

    FILE *f = fopen(db_path, "r");
    if (!f) return ELCHMOD_ERR_DB_NOT_FOUND;

    char line[PATH_MAX + 16];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char *colon = strrchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        if (strcmp(line, abs_path) == 0) {
            unsigned int m;
            if (sscanf(colon + 1, "%o", &m) == 1) {
                *mode = (mode_t)m;
                found = 1;
                break;
            }
        }
    }
    fclose(f);
    return found ? ELCHMOD_OK : ELCHMOD_ERR_DB_NOT_FOUND;
}

elchmod_status_t db_set_mode(const char *path, mode_t mode) {
    if (!path) return ELCHMOD_ERR_INVALID_FORMAT;
    char abs_path[PATH_MAX];
    if (!realpath(path, abs_path)) {
        return ELCHMOD_ERR_FILE_NOT_FOUND;
    }

    char db_path[PATH_MAX];
    char temp_path[PATH_MAX];
    const char *home = getenv("HOME");
    if (!home) return ELCHMOD_ERR_DB_OPEN;
    snprintf(db_path, sizeof(db_path), "%s/.elchmod_db", home);
    snprintf(temp_path, sizeof(temp_path), "%s/.elchmod_db.tmp", home);

    FILE *f_in = fopen(db_path, "r");
    FILE *f_out = fopen(temp_path, "w");
    if (!f_out) {
        if (f_in) fclose(f_in);
        return ELCHMOD_ERR_DB_WRITE;
    }

    char line[PATH_MAX + 16];
    int updated = 0;

    if (f_in) {
        while (fgets(line, sizeof(line), f_in)) {
            char line_copy[PATH_MAX + 16];
            strcpy(line_copy, line);
            char *colon = strrchr(line_copy, ':');
            if (colon) {
                *colon = '\0';
                if (strcmp(line_copy, abs_path) == 0) {
                    fprintf(f_out, "%s:%03o\n", abs_path, (unsigned int)mode);
                    updated = 1;
                    continue;
                }
            }
            fputs(line, f_out);
        }
        fclose(f_in);
    }

    if (!updated) {
        fprintf(f_out, "%s:%03o\n", abs_path, (unsigned int)mode);
    }

    fclose(f_out);
    if (rename(temp_path, db_path) != 0) {
        return ELCHMOD_ERR_DB_WRITE;
    }
    return ELCHMOD_OK;
}