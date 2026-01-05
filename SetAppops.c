#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>int main() {
    const char *packages[] = {
        "com.aefyr.sai",
        "com.android.chrome",
        "com.topjohnwu.magisk",
        "com.alphainventor.filemanager",
        NULL
    };for (int i = 0; packages[i] != NULL; i++) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "appops set \"%s\" REQUEST_INSTALL_PACKAGES allow 2>/dev/null", packages[i]);
    system(cmd);
}

return 0;}
