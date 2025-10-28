#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ZRAM_DEVICE     "/dev/block/zram0"
#define SYS_RESET       "/sys/block/zram0/reset"
#define SYS_DISKSIZE    "/sys/block/zram0/disksize"
#define SYS_ALGO        "/sys/block/zram0/comp_algorithm"
#define SYS_STREAMS     "/sys/block/zram0/max_comp_streams"

#define ZRAM_SIZE       (512LL * 1024 * 1024)  // 512MB in bytes

int write_sysfs(const char *path, const char *value) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    int ret = fprintf(f, "%s", value);
    fclose(f);
    return (ret > 0) ? 0 : -1;
}

int main() {
    if (write_sysfs(SYS_RESET, "1") != 0) {
        printf("Failed: reset zram0\n");
        return 1;
    }

    char size_str[32];
    snprintf(size_str, sizeof(size_str), "%lld", ZRAM_SIZE);
    if (write_sysfs(SYS_DISKSIZE, size_str) != 0) {
        printf("Failed: set disksize\n");
        return 1;
    }

    FILE *algo_f = fopen(SYS_ALGO, "r+");
    if (algo_f) {
        char buf[256] = {0};
        fread(buf, 1, sizeof(buf)-1, algo_f);

        if (strstr(buf, "lz4")) {
            fseek(algo_f, 0, SEEK_SET);
            fprintf(algo_f, "lz4");
            printf("Algorithm: lz4\n");
        } else {
            char *start = strchr(buf, '[');
            char *end = start ? strchr(start, ']') : NULL;
            if (start && end) {
                *end = '\0';
                char *algo = start + 1;
                fseek(algo_f, 0, SEEK_SET);
                fprintf(algo_f, "%s", algo);
                printf("Algorithm: %s (fallback)\n", algo);
            } else {
                printf("Algorithm: unknown, using default\n");
            }
        }
        fclose(algo_f);
    } else {
        printf("Warning: cannot set algorithm\n");
    }

    write_sysfs(SYS_STREAMS, "4");

    if (system("mkswap " ZRAM_DEVICE " > /dev/null 2>&1") != 0) {
        printf("Failed: mkswap\n");
        return 1;
    }

    if (system("swapon " ZRAM_DEVICE " > /dev/null 2>&1") != 0) {
        printf("Failed: swapon\n");
        return 1;
    }

    printf("zRAM 512MB enabled successfully!\n");
    return 0;
}
