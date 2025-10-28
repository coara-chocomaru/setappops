#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ZRAM_DEVICE     "/dev/block/zram0"
#define SYS_RESET       "/sys/block/zram0/reset"
#define SYS_DISKSIZE    "/sys/block/zram0/disksize"
#define SYS_MEM_LIMIT   "/sys/block/zram0/mem_limit"
#define SYS_ALGO        "/sys/block/zram0/comp_algorithm"
#define SYS_STREAMS     "/sys/block/zram0/max_comp_streams"

#define ZRAM_SIZE       (512LL * 1024 * 1024)

static int write_sysfs(const char *path, const char *value) {
    FILE *f = fopen(path, "we");
    if (!f) return -1;
    int ret = fprintf(f, "%s\n", value);
    fflush(f);
    fclose(f);
    return (ret > 0) ? 0 : -1;
}

int main() {
    if (write_sysfs(SYS_RESET, "1") != 0) {
        fprintf(stderr, "Failed: reset zram0\n");
        return 1;
    }

    char size_str[32];
    snprintf(size_str, sizeof(size_str), "%lld", ZRAM_SIZE);
    if (write_sysfs(SYS_DISKSIZE, size_str) != 0) {
        fprintf(stderr, "Failed: set disksize\n");
        return 1;
    }

    if (write_sysfs(SYS_MEM_LIMIT, size_str) != 0) {
        fprintf(stderr, "Warning: set mem_limit (continue)\n");
    }

    if (write_sysfs(SYS_ALGO, "lzo") != 0) {
        fprintf(stderr, "Failed: set lzo\n");
        return 1;
    }

    if (write_sysfs(SYS_STREAMS, "2") != 0) {
        fprintf(stderr, "Warning: set streams (continue)\n");
    }

    if (system("mkswap " ZRAM_DEVICE " >/dev/null 2>&1") != 0) {
        fprintf(stderr, "Failed: mkswap\n");
        return 1;
    }

    if (system("swapon " ZRAM_DEVICE " >/dev/null 2>&1") != 0) {
        fprintf(stderr, "Failed: swapon\n");
        return 1;
    }

    printf("zRAM 512MB (LZO optimized) enabled!\n");
    fflush(stdout);
    return 0;
}
