#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

static void klog(const char *msg) {
    int fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    if (fd >= 0) {
        dprintf(fd, "selinux_disable: %s\n", msg);
        close(fd);
    }
}
static int is_enforcing(void) {
    int fd = open("/sys/fs/selinux/enforce", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        klog("enforce3");
        return -1;
    }

    char buf[2] = {0};
    if (read(fd, buf, 1) != 1) {
        close(fd);
        klog("enforce2");
        return -1;
    }
    close(fd);

    return (buf[0] == '1') ? 1 : 0;
}
static int disable_selinux(void) {
    const char *path = "/sys/fs/selinux/enforce";
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        klog("enforce");
        return -1;
    }
    if (write(fd, "0", 1) != 1) {
        close(fd);
        klog("0);
        return -1;
    }
    close(fd);

    klog("SELinux Permissive1");
    return 0;
}

int main(void) {
    klog("=== SELinux Permissive2");
    if (is_enforcing() == 1) {
        disable_selinux();
    } else {
        klog("Permissive ok");
    }

    while (1) {
        if (is_enforcing() == 1) {
            klog("Enforcing");
            disable_selinux();
        } else {
            static int once = 0;
            if (once == 0) {
                klog("Permissive");
                once = 1;
            }
        }
        sleep(15);
    }

    return 0;
}
