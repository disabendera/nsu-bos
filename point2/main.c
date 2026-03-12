#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void die_errno(const char *msg) {
    int e = errno;
    fprintf(stderr, "%s: %s (errno=%d)\n", msg, strerror(e), e);
    exit(1);
}

static const char *prog_name(const char *argv0) {
    const char *slash = strrchr(argv0, '/');
    return slash ? slash + 1 : argv0;
}

static void print_mode_bits(mode_t mode) {
    char s[10];

    s[0] = (mode & S_IRUSR) ? 'r' : '-';
    s[1] = (mode & S_IWUSR) ? 'w' : '-';
    s[2] = (mode & S_IXUSR) ? 'x' : '-';

    s[3] = (mode & S_IRGRP) ? 'r' : '-';
    s[4] = (mode & S_IWGRP) ? 'w' : '-';
    s[5] = (mode & S_IXGRP) ? 'x' : '-';

    s[6] = (mode & S_IROTH) ? 'r' : '-';
    s[7] = (mode & S_IWOTH) ? 'w' : '-';
    s[8] = (mode & S_IXOTH) ? 'x' : '-';
    s[9] = '\0';

    printf("%s\n", s);
}

static mode_t parse_octal_mode(const char *s) {
    char *end = NULL;
    long val = strtol(s, &end, 8);

    if (!s[0] || *end != '\0' || val < 0 || val > 07777) {
        fprintf(stderr, "invalid mode: %s\n", s);
        exit(1);
    }

    return (mode_t)val;
}

static void copy_fd_to_stdout(int fd) {
    char buf[4096];

    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n < 0) {
            die_errno("read");
        }
        if (n == 0) {
            break;
        }

        size_t written = 0;
        while (written < (size_t)n) {
            ssize_t w = write(STDOUT_FILENO, buf + written, (size_t)n - written);
            if (w < 0) {
                die_errno("write");
            }
            written += (size_t)w;
        }
    }
}

static void cmd_mkd(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: mkd <dir>\n");
        exit(1);
    }

    if (mkdir(argv[1], 0755) == -1) {
        die_errno("mkdir");
    }
}

static void cmd_lsd(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: lsd <dir>\n");
        exit(1);
    }

    DIR *dir = opendir(argv[1]);
    if (!dir) {
        die_errno("opendir");
    }

    for (;;) {
        errno = 0;
        struct dirent *de = readdir(dir);
        if (!de) {
            if (errno != 0) {
                die_errno("readdir");
            }
            break;
        }

        printf("%s\n", de->d_name);
    }

    if (closedir(dir) == -1) {
        die_errno("closedir");
    }
}

static void cmd_rmd(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: rmd <dir>\n");
        exit(1);
    }

    if (rmdir(argv[1]) == -1) {
        die_errno("rmdir");
    }
}

static void cmd_mkf(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: mkf <file>\n");
        exit(1);
    }

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1) {
        die_errno("open(create file)");
    }

    if (close(fd) == -1) {
        die_errno("close");
    }
}

static void cmd_catf(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: catf <file>\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        die_errno("open");
    }

    copy_fd_to_stdout(fd);

    if (close(fd) == -1) {
        die_errno("close");
    }
}

static void cmd_rmf(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: rmf <file>\n");
        exit(1);
    }

    if (unlink(argv[1]) == -1) {
        die_errno("unlink");
    }
}

static void cmd_mksym(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: mksym <target> <linkpath>\n");
        exit(1);
    }

    if (symlink(argv[1], argv[2]) == -1) {
        die_errno("symlink");
    }
}

static void cmd_catsym(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: catsym <symlink>\n");
        exit(1);
    }

    char buf[PATH_MAX];
    ssize_t n = readlink(argv[1], buf, sizeof(buf) - 1);
    if (n == -1) {
        die_errno("readlink");
    }

    buf[n] = '\0';
    printf("%s\n", buf);
}

static void cmd_catbylink(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: catbylink <symlink>\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        die_errno("open");
    }

    copy_fd_to_stdout(fd);

    if (close(fd) == -1) {
        die_errno("close");
    }
}

static void cmd_rmsym(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: rmsym <symlink>\n");
        exit(1);
    }

    if (unlink(argv[1]) == -1) {
        die_errno("unlink");
    }
}

static void cmd_mkhard(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: mkhard <oldpath> <newpath>\n");
        exit(1);
    }

    if (link(argv[1], argv[2]) == -1) {
        die_errno("link");
    }
}

static void cmd_rmhard(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: rmhard <hardlink>\n");
        exit(1);
    }

    if (unlink(argv[1]) == -1) {
        die_errno("unlink");
    }
}

static void cmd_statf(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: statf <path>\n");
        exit(1);
    }

    struct stat st;
    if (lstat(argv[1], &st) == -1) {
        die_errno("lstat");
    }

    printf("mode: ");
    print_mode_bits(st.st_mode);
    printf("nlink: %lu\n", (unsigned long)st.st_nlink);
}

static void cmd_chmodf(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: chmodf <path> <mode-octal>\n");
        exit(1);
    }

    mode_t mode = parse_octal_mode(argv[2]);

    if (chmod(argv[1], mode) == -1) {
        die_errno("chmod");
    }
}

int main(int argc, char **argv) {
    const char *name = prog_name(argv[0]);

    if (strcmp(name, "mkd") == 0) {
        cmd_mkd(argc, argv);
    } else if (strcmp(name, "lsd") == 0) {
        cmd_lsd(argc, argv);
    } else if (strcmp(name, "rmd") == 0) {
        cmd_rmd(argc, argv);
    } else if (strcmp(name, "mkf") == 0) {
        cmd_mkf(argc, argv);
    } else if (strcmp(name, "catf") == 0) {
        cmd_catf(argc, argv);
    } else if (strcmp(name, "rmf") == 0) {
        cmd_rmf(argc, argv);
    } else if (strcmp(name, "mksym") == 0) {
        cmd_mksym(argc, argv);
    } else if (strcmp(name, "catsym") == 0) {
        cmd_catsym(argc, argv);
    } else if (strcmp(name, "catbylink") == 0) {
        cmd_catbylink(argc, argv);
    } else if (strcmp(name, "rmsym") == 0) {
        cmd_rmsym(argc, argv);
    } else if (strcmp(name, "mkhard") == 0) {
        cmd_mkhard(argc, argv);
    } else if (strcmp(name, "rmhard") == 0) {
        cmd_rmhard(argc, argv);
    } else if (strcmp(name, "statf") == 0) {
        cmd_statf(argc, argv);
    } else if (strcmp(name, "chmodf") == 0) {
        cmd_chmodf(argc, argv);
    } else {
        fprintf(stderr, "Unknown command name: %s\n", name);
        return 1;
    }

    return 0;
}