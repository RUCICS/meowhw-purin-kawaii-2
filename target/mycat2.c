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

// 要求2: 实现 io_blocksize 函数
// 在这个任务中，它返回系统内存页的大小
size_t io_blocksize() {
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("警告(sysconf)");
        // 如果获取失败，允许使用一个固定值
        return 4096;
    }
    return (size_t)page_size;
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

    // 使用 io_blocksize 确定缓冲区大小
    size_t buffer_size = io_blocksize();

    // 要求3: 使用标准库函数动态分配内存
    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        perror("错误(malloc)");
        close(fd_in);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd_in, buffer, buffer_size)) > 0) {
        if (full_write(buffer, bytes_read) != (ssize_t)bytes_read) {
            perror("错误(write)");
            free(buffer);
            close(fd_in);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_read == -1) {
        perror("错误(read)");
    }
    
    // 要求5: 保持系统编程规范
    free(buffer);
    close(fd_in);
    return EXIT_SUCCESS;
}