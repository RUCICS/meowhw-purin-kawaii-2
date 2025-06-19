#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

// 辅助函数：健壮地写入所有数据
ssize_t full_write(const void *buf, size_t count) {
    size_t total_written = 0;
    const char *ptr = buf;
    while (total_written < count) {
        ssize_t written = write(STDOUT_FILENO, ptr + total_written, count - total_written);
        if (written == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_written += written;
    }
    return total_written;
}
// 要求2: 实现 align_alloc 函数
// 返回一个对齐到内存页起始的指针
void* align_alloc(size_t size) {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        page_size = 4096;
    }
    
    void *ptr;
    int ret = posix_memalign(&ptr, page_size, size);
    if (ret != 0) {
        // posix_memalign 失败不设置 errno，而是返回错误码
        fprintf(stderr, "错误(posix_memalign): 错误码 %d\n", ret);
        return NULL;
    }
    return ptr;
}

// 要求2: 实现 align_free 函数
// 释放由 align_alloc 分配的内存
void align_free(void* ptr) {
    free(ptr);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法: %s <文件名>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd_in = open(argv[1], O_RDONLY);
    if (fd_in == -1) {
        perror("错误(open)");
        exit(EXIT_FAILURE);
    }

    // 要求3: 缓冲区大小仍然设置成一个内存页的大小
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        page_size = 4096;
    }
    size_t buffer_size = (size_t)page_size;

    // 要求3: 利用这两个函数修改代码
    char *buffer = align_alloc(buffer_size);
    if (buffer == NULL) {
        // align_alloc 内部已打印错误信息
        close(fd_in);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd_in, buffer, buffer_size)) > 0) {
        if (full_write(buffer, bytes_read) != (ssize_t)bytes_read) {
            perror("错误(write)");
            align_free(buffer);
            close(fd_in);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_read == -1) {
        perror("错误(read)");
    }

    align_free(buffer);
    close(fd_in);
    return EXIT_SUCCESS;
}