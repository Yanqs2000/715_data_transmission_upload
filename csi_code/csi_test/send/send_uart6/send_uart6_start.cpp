#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <cstring>
#include <errno.h>

using namespace std;

// 设置串口的配置
int configure_serial_port(const char* serial_device) 
{
    int serial_fd = open(serial_device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd == -1) 
    {
        cerr << "Failed to open serial port: " << strerror(errno) << endl;
        return -1;
    }

    struct termios options;
    tcgetattr(serial_fd, &options);
    
    // 设置串口参数
    cfsetispeed(&options, B9600);        // 输入波特率 9600
    cfsetospeed(&options, B9600);        // 输出波特率 9600
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
    const char* serial_device = "/dev/ttyS6"; // 串口6设备路径

    // 配置串口
    int serial_fd = configure_serial_port(serial_device);
    if (serial_fd == -1) 
    {
        return -1;
    }

    // 数据包（你可以根据实际需要修改这个数据包）
    uint8_t data_packet[32] = 
    {
        0xF0, 0x00, 0x20, 0x00, 0x0A, 0x01, 0x02, 0x03,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0D, 0x0A,
    };

    // 向串口发送数据包
    ssize_t bytes_written = write(serial_fd, data_packet, sizeof(data_packet));
    if (bytes_written < 0) 
    {
        cerr << "Failed to write to serial port: " << strerror(errno) << endl;
        close(serial_fd);
        return -1;
    }

    cout << "Data successfully sent to " << serial_device << endl;

    // 关闭串口
    close(serial_fd);

    return 0;
}
// 编译命令g++ -o send_uart6_start send_uart6_start.cpp 