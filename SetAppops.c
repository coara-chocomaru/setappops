#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>


#define MAX_BUFFER_SIZE 4096
#define MAX_PACKAGES 1024
#define MAX_PKG_NAME 256


typedef struct {
    char **pkgs;
    int count;
    int capacity;
} PackageList;


void init_package_list(PackageList *list) {
    list->pkgs = malloc(sizeof(char *) * 10); 
    if (!list->pkgs) {
        perror("malloc failed for PackageList");
        exit(EXIT_FAILURE);
    }
    list->capacity = 10;
    list->count = 0;
}


void add_package(PackageList *list, const char *pkg) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        char **new_pkgs = realloc(list->pkgs, sizeof(char *) * list->capacity);
        if (!new_pkgs) {
            perror("realloc failed for PackageList");
            exit(EXIT_FAILURE);
        }
        list->pkgs = new_pkgs;
    }
    list->pkgs[list->count] = strdup(pkg);
    if (!list->pkgs[list->count]) {
        perror("strdup failed");
        exit(EXIT_FAILURE);
    }
    list->count++;
}


void free_package_list(PackageList *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->pkgs[i]);
    }
    free(list->pkgs);
}


char *execute_command(const char *cmd) {
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "popen failed for command: %s (%s)\n", cmd, strerror(errno));
        return NULL;
    }

    char *output = NULL;
    size_t output_size = 0;
    char buffer[MAX_BUFFER_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
        char *new_output = realloc(output, output_size + bytes_read + 1);
        if (!new_output) {
            perror("realloc failed in execute_command");
            free(output);
            pclose(pipe);
            return NULL;
        }
        output = new_output;
        memcpy(output + output_size, buffer, bytes_read);
        output_size += bytes_read;
    }

    if (ferror(pipe)) {
        fprintf(stderr, "fread error in execute_command for: %s\n", cmd);
        free(output);
        pclose(pipe);
        return NULL;
    }

    int status = pclose(pipe);
    if (status == -1) {
        fprintf(stderr, "pclose failed for command: %s (%s)\n", cmd, strerror(errno));
        free(output);
        return NULL;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Command failed with exit code %d: %s\n", WEXITSTATUS(status), cmd);
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


char **split_string(const char *str, char delim, int *count) {
    if (!str) {
        *count = 0;
        return NULL;
    }

    char *copy = strdup(str);
    if (!copy) {
        perror("strdup failed in split_string");
        exit(EXIT_FAILURE);
    }

    int capacity = 10;
    char **tokens = malloc(sizeof(char *) * capacity);
    if (!tokens) {
        perror("malloc failed in split_string");
        free(copy);
        exit(EXIT_FAILURE);
    }

    *count = 0;
    char *token = strtok(copy, &delim);
    while (token) {
        
        while (*token == ' ' || *token == '\n' || *token == '\r') token++;

        if (*token == '\0') {
            token = strtok(NULL, &delim);
            continue;
        }

        if (*count >= capacity) {
            capacity *= 2;
            char **new_tokens = realloc(tokens, sizeof(char *) * capacity);
            if (!new_tokens) {
                perror("realloc failed in split_string");
                for (int i = 0; i < *count; i++) free(tokens[i]);
                free(tokens);
                free(copy);
                exit(EXIT_FAILURE);
            }
            tokens = new_tokens;
        }

        tokens[*count] = strdup(token);
        if (!tokens[*count]) {
            perror("strdup failed in split_string");
            for (int i = 0; i < *count; i++) free(tokens[i]);
            free(tokens);
            free(copy);
            exit(EXIT_FAILURE);
        }
        (*count)++;
        token = strtok(NULL, &delim);
    }

    free(copy);
    return tokens;
}


int execute_command_no_output(const char *cmd) {
    int status = system(cmd);
    if (status == -1) {
        fprintf(stderr, "system failed for command: %s (%s)\n", cmd, strerror(errno));
        return -1;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Command failed with exit code %d: %s\n", WEXITSTATUS(status), cmd);
        return WEXITSTATUS(status);
    }
    return 0;
}

int main() {
    int total = 0;
    int checked = 0;
    int skipped_no_permission = 0;
    int already_allowed = 0;
    int set_success = 0;
    int set_failed = 0;
    int failed_checks = 0;

    PackageList already_allowed_pkgs;
    init_package_list(&already_allowed_pkgs);
    PackageList set_success_pkgs;
    init_package_list(&set_success_pkgs);
    PackageList skipped_no_permission_pkgs; 
    init_package_list(&skipped_no_permission_pkgs);

    
    char *pkg_output = execute_command("pm list packages -3 2>/dev/null | cut -d: -f2");
    int pkg_count;
    char **pkg_list = split_string(pkg_output ? pkg_output : "", '\n', &pkg_count);
    free(pkg_output);

    if (pkg_count == 0) {
        printf("警告: 'pm list packages -3' ユーザーアプリ無し）。\n");
        pkg_output = execute_command("pm list packages | cut -d: -f2");
        free(pkg_list); 
        pkg_list = split_string(pkg_output ? pkg_output : "", '\n', &pkg_count);
        free(pkg_output);
    }

    
    int filtered_count = 0;
    char **filtered_pkg_list = malloc(sizeof(char *) * pkg_count);
    if (!filtered_pkg_list) {
        perror("malloc failed for filtered_pkg_list");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < pkg_count; i++) {
        if (pkg_list[i] && strlen(pkg_list[i]) > 0) {
            filtered_pkg_list[filtered_count++] = pkg_list[i];
        } else {
            free(pkg_list[i]);
        }
    }
    free(pkg_list);
    pkg_list = filtered_pkg_list;
    pkg_count = filtered_count;

    total = pkg_count;

    for (int i = 0; i < pkg_count; i++) {
        const char *pkg = pkg_list[i];

    
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "dumpsys package \"%s\" 2>/dev/null", pkg);
        char *dumpsys_out = execute_command(cmd);

        int requests_found = 1; 
        if (dumpsys_out && contains_substring(dumpsys_out, "REQUEST_INSTALL_PACKAGES")) {
            requests_found = 0;
        }
        free(dumpsys_out);

        if (requests_found != 0) {
        
            snprintf(cmd, sizeof(cmd), "pm dump \"%s\" 2>/dev/null", pkg);
            char *pm_out = execute_command(cmd);
            if (pm_out && contains_substring(pm_out, "REQUEST_INSTALL_PACKAGES")) {
                requests_found = 0;
            }
            free(pm_out);
        }

        if (requests_found != 0) {
            skipped_no_permission++;
            add_package(&skipped_no_permission_pkgs, pkg); // Add to skipped list
            printf("SKIP (no manifest permission) : %s\n", pkg);
            continue;
        }

        checked++;

        
        snprintf(cmd, sizeof(cmd), "appops get \"%s\" REQUEST_INSTALL_PACKAGES 2>/dev/null", pkg);
        char *appops_out = execute_command(cmd);
        if (!appops_out) {
            failed_checks++;
            printf("WARN: appops の取得に失敗 : %s\n", pkg);
            continue;
        }

        char *appops_lower = strdup(appops_out);
        if (!appops_lower) {
            perror("strdup failed for appops_lower");
            free(appops_out);
            exit(EXIT_FAILURE);
        }
        to_lowercase(appops_lower);

        if (strstr(appops_lower, "allow")) {
            already_allowed++;
            add_package(&already_allowed_pkgs, pkg);
            printf("ALREADY allowed : %s\n", pkg);
            free(appops_lower);
            free(appops_out);
            continue;
        }
        free(appops_lower);

        printf("SET allow : %s\n", pkg);

        
        snprintf(cmd, sizeof(cmd), "appops set \"%s\" REQUEST_INSTALL_PACKAGES allow 2>/dev/null", pkg);
        int set_result = execute_command_no_output(cmd);
        if (set_result != 0) {
            set_failed++;
            printf("FAIL: appops set failed : %s\n", pkg);
            free(appops_out);
            continue;
        }

        
        snprintf(cmd, sizeof(cmd), "appops get \"%s\" REQUEST_INSTALL_PACKAGES 2>/dev/null", pkg);
        char *appops_out2 = execute_command(cmd);
        if (!appops_out2) {
            set_failed++;
            printf("FAIL: set succeeded but verification failed : %s\n", pkg); // Assuming failure if can't get
            free(appops_out);
            continue;
        }

        char *appops_lower2 = strdup(appops_out2);
        if (!appops_lower2) {
            perror("strdup failed for appops_lower2");
            free(appops_out2);
            free(appops_out);
            exit(EXIT_FAILURE);
        }
        to_lowercase(appops_lower2);

        if (strstr(appops_lower2, "allow")) {
            set_success++;
            add_package(&set_success_pkgs, pkg);
            printf("OK : %s is now allowed\n", pkg);
        } else {
            set_failed++;
            printf("FAIL: set succeeded but verification failed : %s\n", pkg);
        }
        free(appops_lower2);
        free(appops_out2);
        free(appops_out);
    }

    
    for (int i = 0; i < pkg_count; i++) {
        free(pkg_list[i]);
    }
    free(pkg_list);

    
    printf("-------------------------\n");
    printf("総パッケージ数: %d\n", total);
    printf("ManifestにREQUEST_INSTALL_PACKAGES存在するか確認: %d\n", checked);
    printf(" - 設定済み: %d\n", already_allowed);
    if (already_allowed_pkgs.count > 0) {
        printf("   設定済みパッケージ:\n");
        for (int i = 0; i < already_allowed_pkgs.count; i++) {
            printf("     - %s\n", already_allowed_pkgs.pkgs[i]);
        }
    }
    printf(" - 設定成功: %d\n", set_success);
    if (set_success_pkgs.count > 0) {
        printf("   設定成功パッケージ:\n");
        for (int i = 0; i < set_success_pkgs.count; i++) {
            printf("     - %s\n", set_success_pkgs.pkgs[i]);
        }
    }
    printf(" - 設定失敗: %d\n", set_failed);
    printf("スキップ: Manifestに権限無し: %d\n", skipped_no_permission);
    if (skipped_no_permission_pkgs.count > 0) {
        printf("   権限無しパッケージ:\n");
        for (int i = 0; i < skipped_no_permission_pkgs.count; i++) {
            printf("     - %s\n", skipped_no_permission_pkgs.pkgs[i]);
        }
    }
    printf("警告/確認できない: %d\n", failed_checks);
    printf("処理が完了しましたアプリを終了してください。\n");

    free_package_list(&already_allowed_pkgs);
    free_package_list(&set_success_pkgs);
    free_package_list(&skipped_no_permission_pkgs);

    return 0;
}
