#include <cstdio>
#include <cstring>
#include <sys/mount.h>
#include <sys/stat.h>

void mount_part(const char* part) {
    char dev[128];
    char mnt[128];
    snprintf(dev, sizeof(dev), "/dev/block/by-name/%s", part);
    snprintf(mnt, sizeof(mnt), "/%s", part);

    mkdir(mnt, 0755);

    if (mount(dev, mnt, "ext4", 0, NULL) == 0) {
        printf("Mounted %s successfully.\n", part);
    } else {
        perror("mount failed");
    }
}

void unmount_part(const char* part) {
    char mnt[128];
    snprintf(mnt, sizeof(mnt), "/%s", part);

    if (umount(mnt) == 0) {
        printf("Unmounted %s successfully.\n", part);
    } else {
        perror("umount failed");
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s [-v | -c | -f | -all | -um]\n", argv[0]);
        return 1;
    }

    const char* opt = argv[1];

    if (strcmp(opt, "-v") == 0) {
        mount_part("vendor");
    } else if (strcmp(opt, "-c") == 0) {
        mount_part("cache");
    } else if (strcmp(opt, "-f") == 0) {
        mount_part("factory");
    } else if (strcmp(opt, "-all") == 0) {
        mount_part("cache");
        mount_part("vendor");
        mount_part("factory");
    } else if (strcmp(opt, "-um") == 0) {
        unmount_part("factory");
        unmount_part("vendor");
    } else {
        printf("Unknown option: %s\n", opt);
        printf("Usage: %s [-v | -c | -f | -all | -um]\n", argv[0]);
        return 1;
    }
    return 0;
}
