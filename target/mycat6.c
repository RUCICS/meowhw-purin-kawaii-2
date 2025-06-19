#define _GNU_SOURCE // fadvise 需要这个宏
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

    // 要求2: 在代码中使用 fadvise 进行优化
    // 提示内核我们将要顺序读取整个文件
    // 参数: 文件描述符, 偏移量(0=文件头), 长度(0=到文件尾), 提示类型
    if (posix_fadvise(fd_in, 0, 0, POSIX_FADV_SEQUENTIAL) != 0) {
        // fadvise 失败不应是致命错误，只打印警告
        perror("警告(posix_fadvise)");
    }
    
    size_t buffer_size = io_blocksize();
    
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