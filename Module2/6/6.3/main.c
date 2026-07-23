#include "plugin_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>

typedef struct {
    char path[PATH_MAX]; 
    char *name;          
    char *symbol;        
} loaded_plugin_t;

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
    const char *plugin_dir = "./build/plugins";
    DIR *dir = opendir(plugin_dir);
    if (!dir) {
        plugin_dir = "./plugins";
        dir = opendir(plugin_dir);
    }

    if (!dir) {
        fprintf(stderr, "Error: Plugins directory not found.\n");
        return 1;
    }

    loaded_plugin_t *plugins = NULL;
    size_t capacity = 0;
    size_t count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);
        if (len > 3 && strcmp(entry->d_name + len - 3, ".so") == 0) {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", plugin_dir, entry->d_name);

            void *handle = dlopen(path, RTLD_LAZY);
            if (!handle) {
                fprintf(stderr, "Warning: Failed to load %s: %s\n", path, dlerror());
                continue;
            }

            dlerror();
            calc_plugin_t *info = (calc_plugin_t*)dlsym(handle, "plugin_info");
            char *err = dlerror();
            if (err != NULL || !info) {
                fprintf(stderr, "Warning: Symbol plugin_info not found in %s: %s\n", path, err ? err : "NULL");
                dlclose(handle);
                continue;
            }

            if (count >= capacity) {
                size_t new_cap = capacity == 0 ? 4 : capacity * 2;
                loaded_plugin_t *temp = realloc(plugins, new_cap * sizeof(loaded_plugin_t));
                if (!temp) {
                    fprintf(stderr, "Fatal error: out of memory.\n");
                    dlclose(handle);
                    break;
                }
                plugins = temp;
                capacity = new_cap;
            }

            strncpy(plugins[count].path, path, sizeof(plugins[count].path) - 1);
            plugins[count].path[sizeof(plugins[count].path) - 1] = '\0';
            
            plugins[count].name = strdup(info->name);
            plugins[count].symbol = strdup(info->symbol);
            count++;

            dlclose(handle);
        }
    }
    closedir(dir);

    if (count == 0) {
        fprintf(stderr, "Error: No valid plugins loaded from %s.\n", plugin_dir);
        free(plugins);
        return 1;
    }

    while (1) {
        printf("\n========================================\n");
        printf("         PLUGIN CALCULATOR (6.3)        \n");
        printf("========================================\n");
        for (size_t i = 0; i < count; i++) {
            printf("%zu. %s (%s)\n", i + 1, plugins[i].name, plugins[i].symbol);
        }
        printf("%zu. Exit\n", count + 1);
        printf("========================================\n");

        int choice = get_int_option("Choose an option: ");
        if (choice == (int)(count + 1)) {
            printf("\nExiting.\n");
            break;
        }

        if (choice < 1 || (size_t)choice > count) {
            printf("Error: Invalid option.\n");
            continue;
        }

        size_t idx = (size_t)(choice - 1);

        void *handle = dlopen(plugins[idx].path, RTLD_LAZY);
        if (!handle) {
            printf("\nError: Plugin file was deleted or is unavailable!\n");
            printf("Unloading plugin '%s' from list.\n", plugins[idx].name);

            free(plugins[idx].name);
            free(plugins[idx].symbol);

            for (size_t j = idx; j < count - 1; j++) {
                plugins[j] = plugins[j + 1];
            }
            count--;
            continue;
        }

        calc_plugin_t *info = (calc_plugin_t*)dlsym(handle, "plugin_info");
        if (!info || !info->op) {
            printf("\nError: Failed to find valid plugin info or operation pointer.\n");
            dlclose(handle);
            continue;
        }

        int success_a = 0;
        double arg_a = get_double_input("Enter first argument (A): ", &success_a);
        if (!success_a) {
            printf("Error: Invalid numeric input for argument A.\n");
            dlclose(handle);
            continue;
        }

        int success_b = 0;
        double arg_b = get_double_input("Enter second argument (B): ", &success_b);
        if (!success_b) {
            printf("Error: Invalid numeric input for argument B.\n");
            dlclose(handle);
            continue;
        }

        double result = 0.0;
        calc_op_t operation = info->op;
        calc_status_t status = operation(arg_a, arg_b, &result);

        if (status == CALC_OK) {
            printf("\nResult: %g\n", result);
        } else if (status == CALC_ERR_DIV_BY_ZERO) {
            printf("\nError: Division by zero is undefined.\n");
        } else {
            printf("\nError: Operation failed.\n");
        }

        dlclose(handle);
    }

    for (size_t i = 0; i < count; i++) {
        free(plugins[i].name);
        free(plugins[i].symbol);
    }
    free(plugins);

    return 0;
}