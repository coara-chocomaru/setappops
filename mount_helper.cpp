#include <cstdio>
#include <cstring>
#include <sys/mount.h>
#include <sys/stat.h>

void operate_part(const char* part, bool do_mount) {
    char mnt[128];
    snprintf(mnt, sizeof(mnt), "/%s", part);

    if (do_mount) {
        char dev[128];
        snprintf(dev, sizeof(dev), "/dev/block/by-name/%s", part);

        mkdir(mnt, 0755); 

        if (mount(dev, mnt, "ext4", 0, NULL) == 0) {
            printf("マウント成功: %s\n", part);
        } else {
            printf("マウント失敗: %s\n", part);
            perror("mount");
        }
    } else {
        if (umount(mnt) == 0) {
            printf("アンマウント成功: %s\n", part);
        } else {
            printf("アンマウント失敗: %s\n", part);
            perror("umount");
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s [-v | -c | -f | -um -v | -um -c | -um -f] ...\n", argv[0]);
        printf("  （-um は複数指定可：例 -um -v -c -f）\n");
        return 1;
    }

    const char* opt = argv[1];

    if (strcmp(opt, "-v") == 0) {
        operate_part("vendor", true);
    } else if (strcmp(opt, "-c") == 0) {
        operate_part("cache", true);
    } else if (strcmp(opt, "-f") == 0) {
        operate_part("factory", true);
    } else if (strcmp(opt, "-um") == 0) {
        if (argc < 3) {
            printf("Usage: %s -um [-v | -c | -f] ...\n", argv[0]);
            return 1;
        }
        for (int i = 2; i < argc; ++i) {
            const char* sub = argv[i];
            if (strcmp(sub, "-v") == 0) {
                operate_part("vendor", false);
            } else if (strcmp(sub, "-c") == 0) {
                operate_part("cache", false);
            } else if (strcmp(sub, "-f") == 0) {
                operate_part("factory", false);
            } else {
                printf("Unknown unmount option: %s\n", sub);
            }
        }
    } else {
        printf("Unknown option: %s\n", opt);
        printf("Usage: %s [-v | -c | -f | -um -v | -um -c | -um -f] ...\n", argv[0]);
        return 1;
    }
    return 0;
}
