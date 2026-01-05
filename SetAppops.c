#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

static const char *pkgs[] = {
    "com.aefyr.sai",
    "com.android.chrome",
    "com.topjohnwu.magisk",
    "com.alphainventor.filemanager"
};

static int contains_ci(const char *h, const char *n){
    if(!h||!n) return 0;
    size_t hl=strlen(h), nl=strlen(n);
    if(nl==0) return 1;
    for(size_t i=0;i+nl<=hl;i++){
        int ok=1;
        for(size_t j=0;j<nl;j++){
            unsigned char a=h[i+j], b=n[j];
            if(tolower(a)!=tolower(b)){ ok=0; break; }
        }
        if(ok) return 1;
    }
    return 0;
}

static int is_installed(const char *pkg){
    char cmd[256];
    int status;
    snprintf(cmd,sizeof(cmd),"pm path \"%s\" >/dev/null 2>&1",pkg);
    status = system(cmd);
    if(status == -1) return 0;
    if(WIFEXITED(status) && WEXITSTATUS(status)==0) return 1;
    return 0;
}

static int is_allowed(const char *pkg){
    char cmd[256];
    snprintf(cmd,sizeof(cmd),"appops get \"%s\" REQUEST_INSTALL_PACKAGES 2>/dev/null",pkg);
    FILE *p = popen(cmd,"r");
    if(!p) return 0;
    char buf[512];
    size_t n = fread(buf,1,sizeof(buf)-1,p);
    buf[n]='\0';
    pclose(p);
    if(n==0) return 0;
    if(contains_ci(buf,"allow")) return 1;
    return 0;
}

static int set_allow(const char *pkg){
    char cmd[256];
    int status;
    snprintf(cmd,sizeof(cmd),"appops set \"%s\" REQUEST_INSTALL_PACKAGES allow >/dev/null 2>&1",pkg);
    status = system(cmd);
    if(status == -1) return 0;
    if(WIFEXITED(status) && WEXITSTATUS(status)==0) return 1;
    return 0;
}

int main(void){
    size_t n = sizeof(pkgs)/sizeof(pkgs[0]);
    for(size_t i=0;i<n;i++){
        const char *pkg = pkgs[i];
        if(!is_installed(pkg)) continue;
        if(is_allowed(pkg)) continue;
        set_allow(pkg);
    }
    return 0;
}
