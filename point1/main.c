#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

static void die_errno(const char *msg) {
    int e = errno;
    fprintf(stderr, "%s: %s (errno=%d)\n", msg, strerror(e), e);
    exit(1);
}

static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "malloc(%zu) failed\n", n);
        exit(1);
    }
    return p;
}

static char *reverse_dup(const char *s) {
    size_t n = strlen(s);
    char *r = (char *)xmalloc(n + 1);
    for (size_t i = 0; i < n; i++) r[i] = s[n - 1 - i];
    r[n] = '\0';
    return r;
}

static int is_dot_or_dotdot(const char *name) {
    return (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0);
}

static void reverse_buffer(unsigned char *buf, size_t n) {
    size_t i = 0, j = (n == 0 ? 0 : n - 1);
    while (i < j) {
        unsigned char tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
        i++;
        j--;
    }
}

static void copy_file_reverse(int src_fd, int dst_fd, off_t size) {
    const size_t block_size = 64 * 1024;

    unsigned char *buf = (unsigned char *)xmalloc(block_size);

    off_t blocks = (size + (off_t)block_size - 1) / (off_t)block_size;

    for (off_t bi = blocks - 1; bi >= 0; bi--) {
        off_t offset = bi * (off_t)block_size;
        size_t want = (size_t)((size - offset) < (off_t)block_size ? (size - offset) : (off_t)block_size);

        if (lseek(src_fd, offset, SEEK_SET) == (off_t)-1) {
            free(buf);
            die_errno("lseek(src)");
        }

        ssize_t got = read(src_fd, buf, want);
        if (got < 0) {
            free(buf);
            die_errno("read(src)");
        }
        if ((size_t)got != want) {
            size_t total = (size_t)got;
            while (total < want) {
                ssize_t g2 = read(src_fd, buf + total, want - total);
                if (g2 < 0) {
                    free(buf);
                    die_errno("read(src) (retry)");
                }
                if (g2 == 0) break;
                total += (size_t)g2;
            }
            if (total != want) {
                free(buf);
                fprintf(stderr, "short read: expected %zu, got %zu\n", want, total);
                exit(1);
            }
        }

        reverse_buffer(buf, want);

        size_t written = 0;
        while (written < want) {
            ssize_t w = write(dst_fd, buf + written, want - written);
            if (w < 0) {
                free(buf);
                die_errno("write(dst)");
            }
            written += (size_t)w;
        }

        if (bi == 0) break;
    }

    free(buf);
}

static void join_path(char out[PATH_MAX], const char *dir, const char *name) {
    size_t dl = strlen(dir);
    if (dl == 0) {
        snprintf(out, PATH_MAX, "%s", name);
        return;
    }
    if (dir[dl - 1] == '/') snprintf(out, PATH_MAX, "%s%s", dir, name);
    else snprintf(out, PATH_MAX, "%s/%s", dir, name);
}

static void split_parent_base(const char *path, char parent[PATH_MAX], char base[NAME_MAX]) {
    const char *slash = strrchr(path, '/');
    if (!slash) {
        snprintf(parent, PATH_MAX, "%s", ".");
        snprintf(base, NAME_MAX, "%s", path);
        return;
    }
    if (slash == path) {
        snprintf(parent, PATH_MAX, "%s", "/");
        snprintf(base, NAME_MAX, "%s", slash + 1);
        return;
    }
    size_t plen = (size_t)(slash - path);
    if (plen >= PATH_MAX) plen = PATH_MAX - 1;
    memcpy(parent, path, plen);
    parent[plen] = '\0';
    snprintf(base, NAME_MAX, "%s", slash + 1);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <dir>\n", argv[0]);
        return 1;
    }

    const char *src_dir = argv[1];

    struct stat st_srcdir;
    if (stat(src_dir, &st_srcdir) == -1) die_errno("stat(src_dir)");
    if (!S_ISDIR(st_srcdir.st_mode)) {
        fprintf(stderr, "Not a directory: %s\n", src_dir);
        return 1;
    }

    char parent[PATH_MAX], base[NAME_MAX];
    split_parent_base(src_dir, parent, base);

    if (base[0] == '\0') {
        fprintf(stderr, "Empty directory name in path: %s\n", src_dir);
        return 1;
    }

    char *rev_base = reverse_dup(base);

    char dst_dir[PATH_MAX];
    join_path(dst_dir, parent, rev_base);

    if (mkdir(dst_dir, 0755) == -1) {
        if (errno != EEXIST) die_errno("mkdir(dst_dir)");
    }

    struct stat st_dstdir;
    if (stat(dst_dir, &st_dstdir) == -1) die_errno("stat(dst_dir)");
    if (!S_ISDIR(st_dstdir.st_mode)) {
        fprintf(stderr, "Destination exists but is not a directory: %s\n", dst_dir);
        return 1;
    }

    DIR *dir = opendir(src_dir);
    if (!dir) die_errno("opendir(src_dir)");

    for (;;) {
        errno = 0;
        struct dirent *de = readdir(dir);
        if (!de) {
            if (errno != 0) die_errno("readdir");
            break;
        }

        if (is_dot_or_dotdot(de->d_name)) continue;

        char src_path[PATH_MAX];
        join_path(src_path, src_dir, de->d_name);

        struct stat st;
        if (lstat(src_path, &st) == -1) {
            fprintf(stderr, "Skipping (stat failed) %s: %s\n", src_path, strerror(errno));
            continue;
        }
        if (!S_ISREG(st.st_mode)) {
            continue;
        }

        char *rev_name = reverse_dup(de->d_name);

        char dst_path[PATH_MAX];
        join_path(dst_path, dst_dir, rev_name);

        free(rev_name);

        int sfd = open(src_path, O_RDONLY);
        if (sfd == -1) {
            fprintf(stderr, "Skipping (open src failed) %s: %s\n", src_path, strerror(errno));
            continue;
        }

        int dfd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dfd == -1) {
            fprintf(stderr, "Skipping (open dst failed) %s: %s\n", dst_path, strerror(errno));
            close(sfd);
            continue;
        }

        copy_file_reverse(sfd, dfd, st.st_size);

        if (close(sfd) == -1) die_errno("close(src)");
        if (close(dfd) == -1) die_errno("close(dst)");
    }

    if (closedir(dir) == -1) die_errno("closedir");

    free(rev_base);
    return 0;
}