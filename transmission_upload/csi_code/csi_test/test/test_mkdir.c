#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>
// 直接在代码中使用mkdir函数去创建nas中的文件夹会导致nas掉，但是用命令行的方式没有问题
int main()
{
    char base_command[64] = "sudo mkdir -p ";
    char nas_folder_path[64] = "/mnt/nas/copy2/";
    char full_command[128];
    snprintf(full_command, sizeof(full_command), "%s%s", base_command, nas_folder_path);

    int ret = system(full_command);
    if (ret == -1) 
    {
        perror("Failed to execute mkdir command");
        return -1;
    }
    return 0;
}