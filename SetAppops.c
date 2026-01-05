#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_BUFFER_SIZE 4096

char *execute_command(const char *cmd) {
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        return NULL;
    }

    char *output = NULL;
    size_t output_size = 0;
    char buffer[MAX_BUFFER_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
        char *new_output = realloc(output, output_size + bytes_read + 1);
        if (!new_output) {
            free(output);
            pclose(pipe);
            return NULL;
        }
        output = new_output;
        memcpy(output + output_size, buffer, bytes_read);
        output_size += bytes_read;
    }

    if (ferror(pipe)) {
        free(output);
        pclose(pipe);
        return NULL;
    }

    int status = pclose(pipe);
    if (status == -1 || (WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
        free(output);
        return NULL;
    }

    if (output) {
        output[output_size] = '\0';
    }
    return output;
}

int contains_substring(const char *str, const char *substr) {
    return strstr(str, substr) != NULL;
}

void to_lowercase(char *str) {
    for (char *p = str; *p; p++) {
        *p = tolower((unsigned char)*p);
    }
}

int execute_command_no_output(const char *cmd) {
    int status = system(cmd);
    if (status == -1 || (WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
        return -1;
    }
    return 0;
}

int main() {
    const char *packages[] = {
        "com.aefyr.sai",
        "com.android.chrome",
        "com.topjohnwu.magisk",
        "com.alphainventor.filemanager"
    };
    int num_packages = sizeof(packages) / sizeof(packages[0]);

    for (int i = 0; i < num_packages; i++) {
        const char *pkg = packages[i];

        char cmd[512];
        snprintf(cmd, sizeof(cmd), "dumpsys package \"%s\" 2>/dev/null", pkg);
        char *dumpsys_out = execute_command(cmd);

        int has_permission = 0;
        if (dumpsys_out && contains_substring(dumpsys_out, "REQUEST_INSTALL_PACKAGES")) {
            has_permission = 1;
        }
        free(dumpsys_out);

        if (!has_permission) {
            snprintf(cmd, sizeof(cmd), "pm dump \"%s\" 2>/dev/null", pkg);
            char *pm_out = execute_command(cmd);
            if (pm_out && contains_substring(pm_out, "REQUEST_INSTALL_PACKAGES")) {
                has_permission = 1;
            }
            free(pm_out);
        }

        if (!has_permission) {
            continue;
        }

        snprintf(cmd, sizeof(cmd), "appops get \"%s\" REQUEST_INSTALL_PACKAGES 2>/dev/null", pkg);
        char *appops_out = execute_command(cmd);
        if (!appops_out) {
            continue;
        }

        char *appops_lower = strdup(appops_out);
        if (!appops_lower) {
            free(appops_out);
            continue;
        }
        to_lowercase(appops_lower);

        if (strstr(appops_lower, "allow")) {
            free(appops_lower);
            free(appops_out);
            continue;
        }
        free(appops_lower);
        free(appops_out);

        snprintf(cmd, sizeof(cmd), "appops set \"%s\" REQUEST_INSTALL_PACKAGES allow 2>/dev/null", pkg);
        int set_result = execute_command_no_output(cmd);
        if (set_result != 0) {
            continue;
        }

        snprintf(cmd, sizeof(cmd), "appops get \"%s\" REQUEST_INSTALL_PACKAGES 2>/dev/null", pkg);
        char *appops_out2 = execute_command(cmd);
        if (!appops_out2) {
            continue;
        }

        char *appops_lower2 = strdup(appops_out2);
        if (!appops_lower2) {
            free(appops_out2);
            continue;
        }
        to_lowercase(appops_lower2);

        if (!strstr(appops_lower2, "allow")) {
            // No action, just free
        }
        free(appops_lower2);
        free(appops_out2);
    }

    return 0;
}
