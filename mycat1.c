#include <stdio.h>    // 用于 perror, fprintf
#include <stdlib.h>   // 用于 exit, EXIT_SUCCESS, EXIT_FAILURE
#include <fcntl.h>    // 用于 open
#include <unistd.h>   // 用于 read, write, close

int main(int argc, char *argv[]) {
    // 要求2: 接受且只接受一个命令行参数
    if (argc != 2) {
        fprintf(stderr, "用法: %s <文件名>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd_in = open(argv[1], O_RDONLY);
    if (fd_in == -1) {
        perror("错误(open)"); // 要求5: 保持系统编程规范，进行错误处理
        exit(EXIT_FAILURE);
    }

    // 要求3: 每次读取并输出一个字符
    char buffer[1];
    ssize_t bytes_read;

    // 循环直到文件结束或出错
    while ((bytes_read = read(fd_in, buffer, 1)) > 0) {
        // 将读取到的字符写入标准输出 (STDOUT_FILENO = 1)
        if (write(STDOUT_FILENO, buffer, 1) != 1) {
            perror("错误(write)");
            close(fd_in);
            exit(EXIT_FAILURE);
        }
    }

    // 检查 read 是否因为错误而终止循环
    if (bytes_read == -1) {
        perror("错误(read)");
        close(fd_in);
        exit(EXIT_FAILURE);
    }

    // 关闭文件描述符
    if (close(fd_in) == -1) {
        perror("错误(close)");
        exit(EXIT_FAILURE);
    }

    // 要求1: 源代码为 mycat1.c, 可执行文件为 mycat1
    return EXIT_SUCCESS;
}