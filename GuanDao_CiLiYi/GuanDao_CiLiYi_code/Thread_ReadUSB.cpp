#include <glog/logging.h>
#include <gpiod.h>
#include <termios.h>
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>  // for close(), usleep()
#include <fcntl.h>   // for open()
#include <cstring>   // for strerror()
#include <errno.h>   // for errno
#include <algorithm>
#include <deque>
#include <termios.h>
#include <fstream>   // for std::ofstream
#include <stdio.h>
#include <dirent.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <string>
#include <sys/select.h>
#include "DataExtractor.h"
#include "Thread_ReadUSB.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>  // for high_resolution_clock
#include <iomanip>
#include <numeric> // For std::accumulate
#include "INIReader.h"
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

#define GPIO_CHIP "/dev/gpiochip0" // GPIO 芯片
#define GPIO_PH6 230 

extern struct _Params params;

// USB初始化
int USB_init(const char* filename)//接受一个字符指针 filename，表示串口设备的文件名
{
    // 假设usb_serial是您已经配置和打开的串口文件描述符
    // 使用open函数以只读模式打开指定的串口文件。O_NOCTTY 表示不将此设备设为控制终端，O_SYNC 表示同步读写。
    int usb_serial = open(filename, O_RDWR | O_NOCTTY | O_SYNC);
    if (usb_serial < 0) 
    {
        cerr << "Error opening usb_serial: " << strerror(errno) << endl;
        LOG(ERROR) << "Error opening usb_serial: " << strerror(errno);
        return -1;
    }

    //定义一个termios结构体tty来保存串口配置，并将其内容初始化为零
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    // 调用 tcgetattr获取当前串口配置
    if (tcgetattr(usb_serial, &tty) != 0) 
    {
        std::cerr << "Error from tcgetattr: " << strerror(errno) << std::endl;
        LOG(ERROR) << "Error from tcgetattr: " << strerror(errno);
        return -1;
    }
    // 设置串口的波特率为115200。cfsetospeed 和 cfsetispeed 分别设置输出和输入波特率。
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;//CS8表示数据位为8位
    tty.c_iflag &= ~IGNBRK;                    //IGNBRK表示忽略Break信号
    tty.c_lflag = 0;                           //c_lflag和c_oflag设置为0，禁用特殊处理    
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;                        //VMIN设置至少接收0个字符
    tty.c_cc[VTIME] = 2;                       //VTIME设置超时时间为x个十分之一秒
    tty.c_iflag &= ~(ICRNL | INLCR);  // 禁用 \r 到 \n 的转换,和tty.c_oflag = 0;功能相同

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);    //禁用软件流控制（IXON, IXOFF）
    tty.c_cflag |= (CLOCAL | CREAD);           //启用接收和本地模式（CREAD, CLOCAL）
    tty.c_cflag &= ~(PARENB | PARODD);         //禁用奇偶校验（PARENB, PARODD）
    tty.c_cflag &= ~CSTOPB;                    //设置为一个停止位（CSTOPB）
    tty.c_cflag &= ~CRTSCTS;                   //禁用 RTS/CTS 硬件流控制（CRTSCTS）

    //使用tcsetattr应用配置。如果失败，记录错误并返回-1
    if (tcsetattr(usb_serial, TCSANOW, &tty) != 0)
    {
        cerr << "Error from tcsetattr: " << strerror(errno) << endl;
        LOG(ERROR) << "Error from tcsetattr: " << strerror(errno);
        
        close(usb_serial);
        google::ShutdownGoogleLogging();  // Close glog
        return -1;
    }

    return usb_serial;
}

//定义一个函数 read_process，接受串口文件描述符和要读取的字节数，存在读取效率低下的问题，每次只读一个文件
vector<uint8_t> read_process(int usb_serial, size_t total_bytes, const vector<uint8_t> frame_header)
{
    // 获取程序启动时间
    //auto start_time = std::chrono::high_resolution_clock::now();
    
    uint8_t byte;//声明一个字节变量byte

    deque<uint8_t> buffer;//一个双端队列buffer用于存储读取的数据
    vector<uint8_t> frame(total_bytes, 0);//一个大小为total_bytes的向量frame
    string name;

    if (total_bytes == 120)
    {
        name = "CiLiYi";
    }

    if (total_bytes == 110)
    {
        name = "GuanDao";
    }

    //进入无限循环
    while (true) 
    {
        //从串口读取1个字节。
        int num_bytes = read(usb_serial, &byte, 1);

        //如果读取成功，则将其添加到 buffer 中。
        if (num_bytes > 0) 
        {
            // 将读取的字节添加到 buffer 中
            buffer.push_back(byte);
        }
        else if(num_bytes == 0)
        {
            std::cout << std::dec;
            cout << name << " is " << total_bytes << " bytes data, but no data receive" << endl;
            LOG(WARNING) << name << " is " << total_bytes << " bytes data, but no data receive";
            break;
        }
        else
        {
            LOG(ERROR) << "Error while reading: " << name << " data "<< strerror(errno);
            std::cerr << "Error while reading: " << name << " data "<< strerror(errno) << std::endl;
            break;
        }
        
        if(buffer.size() >= total_bytes)
        {
            //cout << "first: "<< std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[0]) << std::endl;
            
            //检查 buffer 的开头是否与 frame_header 匹配
            if (equal(frame_header.begin(), frame_header.end(), buffer.begin())) 
            {
                copy(buffer.begin(), buffer.begin() + total_bytes, frame.begin());//复制数据到 frame
                buffer.erase(buffer.begin(), buffer.begin() + total_bytes);//从 buffer 中移除已处理的字节
                cout << "match success" << endl;
                
                // 计算工作时间
                //auto current_time = std::chrono::high_resolution_clock::now();
                //auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
                //cout << name << " " << elapsed_time.count() << " milliseconds" << endl;

                return frame; //然后返回 frame
            }
            else 
            {
                // 匹配失败，移除 buffer 的第一个字节，继续检查
                buffer.pop_front();
                LOG(ERROR) << "data "  << name << " mismatch";
                //std::cerr << "data "  << name << " mismatch" << std::endl;
                
                // 计算工作时间
                //auto current_time = std::chrono::high_resolution_clock::now();
                //auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
                //cout << name << " " << elapsed_time.count() << " milliseconds" << endl;
            }
        }
    }

    // 计算工作时间
    //auto current_time = std::chrono::high_resolution_clock::now();
    //auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
    //cout << name << " " << elapsed_time.count() << " milliseconds" << endl;

    return frame;
}

// UDP 接收命令线程
void udp_listener_thread()
{
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) 
    {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    server_addr.sin_port = htons(8081);  // Port for receiving the start command

    if (bind(udp_socket, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }

    char initial_command_buffer[64];
    char BiaoJiao_buffer[32];
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    ofstream initial_command;
    ofstream BiaoJiao;
    ifstream BiaoJiao_read;

    cout << "start listening initial command" << endl;
    while (true) 
    {
        ssize_t n = recvfrom(udp_socket, initial_command_buffer, sizeof(initial_command_buffer), 0, (struct sockaddr*)&client_addr, &len);
        if (n > 0) 
        {
            initial_command_buffer[n] = '\0'; // Null-terminate received data
            cout << "Received command" << endl;

            if (strlen(initial_command_buffer) == 5)
            {
                if (access("/mnt/data/GuanDao_CiLiYi/GuanDao_CiLiYi_code/BiaoJiao", F_OK) == 0)
                {
                    if (initial_command_buffer[0] == 0x81 && initial_command_buffer[1] == 0x82 
                    && initial_command_buffer[3] == 0x0D && initial_command_buffer[4] == 0x0A
                    )
                    {
                        initial_command.open("/mnt/data/GuanDao_CiLiYi/GuanDao_CiLiYi_code/start_command", ios::binary | ios::trunc);
                        // 将接收到的起始命令写入文件
                        initial_command.write(initial_command_buffer, 5);
                        // 关闭文件
                        initial_command.close();

                        // 打开标校文件进行读取
                        BiaoJiao_read.open("/mnt/data/GuanDao_CiLiYi/GuanDao_CiLiYi_code/BiaoJiao", std::ios::binary);

                        // 检查文件是否成功打开
                        if (!BiaoJiao_read.is_open()) 
                        {
                            std::cerr << "Failed to open file: start_command"<< std::endl;
                            exit(EXIT_FAILURE);
                        }

                        BiaoJiao_read.read(BiaoJiao_buffer, 26);
                        //printHex(BiaoJiao_buffer, 26);
                        // 关闭文件
                        BiaoJiao_read.close();  

                        // 写入标校命令
                        initial_command.open("/mnt/data/GuanDao_CiLiYi/GuanDao_CiLiYi_code/start_command", ios::binary | ios::app);
                        // 将接收到的起始命令写入文件
                        initial_command.write(BiaoJiao_buffer, 26);
                        // 关闭文件
                        initial_command.close();
                        break;
                    }
                    else
                    {
                        // 数据包不符合要求
                        cout << "Error! Received data packet does not match expected format or length(5)" << endl;
                        cout << "please continue to receive right initial command" << endl; 
                    }
                }
                else
                {
                    cout << "missing BiaoJiao parameter" << endl;
                    cout << "please continue to receive right and complete initial command" << endl; 
                }
            }
            else
            {
                if (initial_command_buffer[0] == 0x81 && initial_command_buffer[1] == 0x82 
                && initial_command_buffer[3] == 0x0D && initial_command_buffer[4] == 0x0A && 
                initial_command_buffer[5] == 0x4C && initial_command_buffer[6] == 0x57 && 
                initial_command_buffer[27] == 0x00 && initial_command_buffer[28] == 0x00 && 
                initial_command_buffer[29] == 0x0D && initial_command_buffer[30] == 0x0A  // 检查帧尾
                )  // 数据包大小为31字节
                {
                    initial_command.open("/mnt/data/GuanDao_CiLiYi/GuanDao_CiLiYi_code/start_command", ios::binary | ios::trunc);
                    BiaoJiao.open("/mnt/data/GuanDao_CiLiYi/GuanDao_CiLiYi_code/BiaoJiao", ios::binary | ios::trunc);

                    // 将接收到的起始命令写入文件
                    initial_command.write(initial_command_buffer, 31);
                    BiaoJiao.write(initial_command_buffer + 5, 26);

                    // 关闭文件
                    initial_command.close();
                    BiaoJiao.close();
                    break;
                }
                else
                {
                    // 数据包不符合要求
                    cout << "Error! Received data packet does not match expected format or length(31)" << endl;
                    cout << "please continue to receive right initial command" << endl; 
                }
            }
        }
        usleep(1000);
    }

    close(udp_socket);
}


// 初始化 GPIO 引脚
bool set_gpio_value(int gpio_line, int value) 
{
    gpiod_chip *chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) 
    {
        std::cerr << "Failed to open GPIO chip." << std::endl;
        return false;
    }

    gpiod_line *line = gpiod_chip_get_line(chip, gpio_line);
    if (!line) 
    {
        std::cerr << "Failed to get GPIO line." << std::endl;
        gpiod_chip_close(chip);
        return false;
    }

    if (gpiod_line_request_output(line, "gpio_control", value) < 0) 
    {
        std::cerr << "Failed to request line as output." << std::endl;
        gpiod_chip_close(chip);
        return false;
    }

    if (gpiod_line_set_value(line, value) < 0) 
    {
        std::cerr << "Failed to set GPIO value." << std::endl;
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        return false;
    }

    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return true;
}

int is_directory_empty(const char *dir_path) 
{
    DIR *dir = opendir(dir_path);
    struct dirent *entry;

    if (dir == NULL) 
    {
        perror("opendir failed");
        return -1; // 目录打开失败
    }

    // 遍历目录中的条目
    while ((entry = readdir(dir)) != NULL) 
    {
        // 忽略当前目录（.）和上一级目录（..）
        if (entry->d_name[0] != '.') 
        {
            closedir(dir);
            return 0; // 目录不为空
        }
    }

    closedir(dir);
    return 1; // 目录为空
}

// 打印char类型数组的值（16进制）
void printHex(const char* command, size_t length) 
{
    std::cout << "Initial command in hex:" << std::endl;
    for (size_t i = 0; i < length; ++i) {
        std::cout << "0x" 
                  << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
                  << (static_cast<unsigned int>(command[i]) & 0xFF) 
                  << " ";
    }
    std::cout << std::endl;
}


