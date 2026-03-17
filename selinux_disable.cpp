#include <unistd.h>
#include <fcntl.h>

static int is_enforcing(void) {
    int fd = open("/sys/fs/selinux/enforce", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return -1;

    char buf[2] = {0};
    if (read(fd, buf, 1) != 1) {
        close(fd);
        return -1;
    }
    close(fd);
    return (buf[0] == '1') ? 1 : 0;
}

static int disable_selinux(void) {
    int fd = open("/sys/fs/selinux/enforce", O_WRONLY | O_CLOEXEC);
    if (fd < 0) return -1;

    if (write(fd, "0", 1) != 1) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

int main(void) {
    if (is_enforcing() == 1) {
        disable_selinux();
    }
    while (1) {
        if (is_enforcing() == 1) {
            disable_selinux();
        }
        sleep(15);
    }

    return 0;
}
