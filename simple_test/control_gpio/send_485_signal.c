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

//串口初始化函数
int USB_init_ciliyi(const char* serial_device) 
{
    int serial_fd = open(serial_device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd == -1) 
    {
        perror("Failed to open serial uart6");
        return -1;
    }

    struct termios options;
    tcgetattr(serial_fd, &options);
    
    // 设置串口参数
    cfsetispeed(&options, B115200);        // 输入波特率
    cfsetospeed(&options, B115200);        // 输出波特率
    options.c_cflag &= ~PARENB;           // 无校验位
    options.c_cflag &= ~CSTOPB;           // 1 个停止位
    options.c_cflag &= ~CSIZE;            // 清除数据位
    options.c_cflag |= CS8;               // 8 个数据位
    options.c_cflag &= ~CRTSCTS;          // 无硬件流控制
    options.c_cflag |= CREAD | CLOCAL;    // 启用接收器，忽略控制线状态
    options.c_iflag &= ~(IXON | IXOFF | IXANY); // 关闭软件流控制
    options.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 设置为原始输入模式
    options.c_oflag &= ~OPOST;            // 设置为原始输出模式
    options.c_cc[VMIN] = 1;               // 至少读取 1 个字节
    options.c_cc[VTIME] = 0;              // 超时设置为 0
    options.c_iflag &= ~(ICRNL | INLCR);  // 禁用 \r 到 \n 的转换,和tty.c_oflag = 0;功能相同
    tcsetattr(serial_fd, TCSANOW, &options); // 应用设置

    return serial_fd;
}


int main()
{
    const char* serial_device_ciliyi = "/dev/ttyS3";
    uint8_t initial_command_buffer[5] = {0x08, 0x01, 0x02, 0x03, 0x09};     // 起始命令数据接收缓冲区

    // 配置串口
    int serial_fd_ciliyi = USB_init_ciliyi(serial_device_ciliyi);
    if (serial_fd_ciliyi == -1) 
    {
        return -1;
    }

    // 向串口发送数据包
    ssize_t bytes_written_start = write(serial_fd_ciliyi, initial_command_buffer, sizeof(initial_command_buffer));
    if (bytes_written_start < 0) 
    {
        perror("start data failed to write to serial uart4");
        close(serial_fd_ciliyi);
        return -1;
    }
    else
    {
        printf("serial_fd_ciliyi:%d\n",bytes_written_start);
    }
}





