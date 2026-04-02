#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_BUF_SIZE 4096
#define STACK_DEPTH 40
#define HEAP_BLOCKS 8
#define HEAP_BLOCK_SIZE (64 * 1024)

static void pause_step(const char *msg, unsigned sec) {
    printf("\n=== %s ===\n", msg);
    printf("pid = %d, sleep(%u)\n", getpid(), sec);
    fflush(stdout);
    sleep(sec);
}

static void stack_grow(int depth, int max_depth) {
    volatile char buf[STACK_BUF_SIZE];
    memset((void *)buf, depth, sizeof(buf));
    printf("stack depth=%2d, buf=%p\n", depth, (void *)buf);
    fflush(stdout);
    sleep(1);
    if (depth + 1 < max_depth) {
        stack_grow(depth + 1, max_depth);
    }
}

static void heap_alloc_demo(void **ptrs, int count, size_t size) {
    for (int i = 0; i < count; ++i) {
        ptrs[i] = malloc(size);
        if (!ptrs[i]) {
            perror("malloc");
            exit(1);
        }
        memset(ptrs[i], 0x41 + i, size);
        printf("heap alloc[%d] = %p, size = %zu\n", i, ptrs[i], size);
        fflush(stdout);
        sleep(1);
    }
}

static void heap_free_demo(void **ptrs, int count) {
    for (int i = 0; i < count; ++i) {
        free(ptrs[i]);
        printf("heap free[%d] = %p\n", i, ptrs[i]);
        fflush(stdout);
        sleep(1);
        ptrs[i] = NULL;
    }
}

static void fault_read_demo(void *region) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        volatile unsigned char value = *((volatile unsigned char *)region);
        printf("unexpected read success: %u\n", value);
        fflush(stdout);
        _exit(0);
    }

    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        printf("child read terminated by signal %d\n", WTERMSIG(status));
    } else {
        printf("child read exited with code %d\n", WEXITSTATUS(status));
    }
    fflush(stdout);
}

static void fault_write_demo(void *region) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        *((volatile unsigned char *)region) = 0xAA;
        printf("unexpected write success\n");
        fflush(stdout);
        _exit(0);
    }

    int status;
    waitpid(pid, &status, 0);
    if (WIFSIGNALED(status)) {
        printf("child write terminated by signal %d\n", WTERMSIG(status));
    } else {
        printf("child write exited with code %d\n", WEXITSTATUS(status));
    }
    fflush(stdout);
}

int main(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        fprintf(stderr, "sysconf(_SC_PAGESIZE) failed\n");
        return 1;
    }

    printf("Process PID: %d\n", getpid());
    printf("Page size: %ld bytes\n", page_size);
    fflush(stdout);

    pause_step("Initial pause", 10);

    pause_step("Stack growth demo starts", 3);
    stack_grow(0, STACK_DEPTH);
    pause_step("Stack growth demo finished", 5);

    void *heap_ptrs[HEAP_BLOCKS] = {0};

    pause_step("Heap allocation demo starts", 3);
    heap_alloc_demo(heap_ptrs, HEAP_BLOCKS, HEAP_BLOCK_SIZE);
    pause_step("Heap allocations completed", 5);

    pause_step("Freeing heap blocks", 3);
    heap_free_demo(heap_ptrs, HEAP_BLOCKS);
    pause_step("Heap free completed", 5);

    size_t region_len = 10 * (size_t)page_size;
    unsigned char *region = mmap(NULL, region_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    for (size_t i = 0; i < region_len; ++i) {
        region[i] = (unsigned char)(i & 0xFF);
    }

    printf("anonymous mmap region: %p .. %p (%zu bytes)\n", region, region + region_len, region_len);
    fflush(stdout);

    pause_step("Anonymous mmap region created", 8);

    if (mprotect(region, region_len, PROT_NONE) != 0) {
        perror("mprotect(PROT_NONE)");
        munmap(region, region_len);
        return 1;
    }

    pause_step("Region set to PROT_NONE", 5);
    fault_read_demo(region);
    pause_step("After forbidden read attempt", 5);

    if (mprotect(region, region_len, PROT_READ) != 0) {
        perror("mprotect(PROT_READ)");
        munmap(region, region_len);
        return 1;
    }

    pause_step("Region set to PROT_READ", 5);
    fault_write_demo(region);
    pause_step("After forbidden write attempt", 5);

    size_t unmap_offset = 3 * (size_t)page_size;
    size_t unmap_len = 3 * (size_t)page_size;

    if (munmap(region + unmap_offset, unmap_len) != 0) {
        perror("munmap");
        munmap(region, region_len);
        return 1;
    }

    printf("unmapped pages 4..6: %p .. %p\n", region + unmap_offset, region + unmap_offset + unmap_len);
    fflush(stdout);

    pause_step("Middle pages unmapped", 10);

    if (munmap(region, 3 * (size_t)page_size) != 0) {
        perror("munmap first part");
    }

    if (munmap(region + 6 * (size_t)page_size, 4 * (size_t)page_size) != 0) {
        perror("munmap second part");
    }

    printf("Done\n");
    fflush(stdout);
    return 0;
}