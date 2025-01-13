#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <termios.h>

int main() {
    const char* device = "/dev/ttyS4"; // 串口设备路径
    int serial_fd = open(device, O_RDWR | O_NOCTTY);
    if (serial_fd < 0) {
        std::cerr << "Error opening serial port: " << strerror(errno) << std::endl;
        return -1;
    }

    struct serial_struct ser_info;
    if (ioctl(serial_fd, TIOCGSERIAL, &ser_info) == 0) {
        std::cout << "Transmit FIFO size: " << ser_info.xmit_fifo_size << " bytes" << std::endl;
    } else {
        std::cerr << "Error getting serial info: " << strerror(errno) << std::endl;
    }

    // 获取串口配置
    struct termios term_info;
    if (tcgetattr(serial_fd, &term_info) == 0) {
        std::cout << "Input baud rate: " << cfgetispeed(&term_info) << std::endl;
        std::cout << "Output baud rate: " << cfgetospeed(&term_info) << std::endl;
    } else {
        std::cerr << "Error getting termios info: " << strerror(errno) << std::endl;
    }

    close(serial_fd);
    return 0;
}
