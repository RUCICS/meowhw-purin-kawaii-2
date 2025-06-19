#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h> // 用于 fstat

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
// 辅助函数：求最大公约数 (Greatest Common Divisor)
size_t gcd(size_t a, size_t b) {
    return b == 0 ? a : gcd(b, a % b);
}

// 辅助函数：求最小公倍数 (Least Common Multiple)
size_t lcm(size_t a, size_t b) {
    if (a == 0 || b == 0) return 0;
    return (a / gcd(a, b)) * b;
}

// 要求2: 修改 io_blocksize 函数
// 让缓冲区大小既考虑到内存页大小也考虑到文件系统的块大小
size_t io_blocksize(int fd) {
    long page_size_long = sysconf(_SC_PAGESIZE);
    if (page_size_long == -1) {
        page_size_long = 4096;
    }
    size_t page_size = (size_t)page_size_long;

    // 注意事项1: 每个文件的块大小可能不同，所以要用 fstat
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("警告(fstat)");
        return page_size; // 如果获取失败，安全地回退到页大小
    }
    size_t fs_block_size = (size_t)file_stat.st_blksize;

    // 注意事项2: 处理虚假的块大小
    // 检查它是否为正且是2的整数次幂
    if (fs_block_size <= 0 || (fs_block_size & (fs_block_size - 1)) != 0) {
        fprintf(stderr, "警告: 检测到虚假的文件系统块大小 (%zu)，将回退使用页大小。\n", fs_block_size);
        return page_size;
    }

    // 返回两者的最小公倍数
    return lcm(page_size, fs_block_size);
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
    
    // 使用新的 io_blocksize 函数确定大小
    size_t buffer_size = io_blocksize(fd_in);
    
    // 分配内存。为了保持规范，我们继续使用对齐分配
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) page_size = 4096;
    
    void *buffer;
    if (posix_memalign(&buffer, (size_t)page_size, buffer_size) != 0) {
        fprintf(stderr, "错误: 无法分配对齐内存\n");
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