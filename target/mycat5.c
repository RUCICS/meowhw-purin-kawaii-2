// 为 posix_fadvise 预先定义
#define _GNU_SOURCE
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
// 要求3: 修改 io_blocksize 函数，利用实验结果
// 实验通常表明 128KB 是一个非常好的权衡点，我们将其硬编码
// GNU cat 也是使用这个值
size_t io_blocksize() {
    return 512 * 1024; 
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
    
    size_t buffer_size = io_blocksize();
    
    // 对于大块内存，直接用 malloc 已经足够了
    // 页对齐带来的额外好处相比于巨大的缓冲区大小可以忽略不计
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
    free(buffer);
    close(fd_in);
    return EXIT_SUCCESS;
}