#include "Thread_ReadUSB.h"
#include <glog/logging.h>
#include <termios.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <unistd.h>  // for close(), usleep()
#include <fcntl.h>   // for open()
#include <cstring>   // for strerror()
#include <errno.h>   // for errno
#include <algorithm>
#include <deque>
#include <termios.h>
#include <fstream>   // for std::ofstream
#include "DataExtractor.h"

using namespace std;

// USB初始化
int USB_init(const char* filename)//接受一个字符指针 filename，表示串口设备的文件名
{
    // 假设usb_serial是您已经配置和打开的串口文件描述符
    // 使用open函数以只读模式打开指定的串口文件。O_NOCTTY 表示不将此设备设为控制终端，O_SYNC 表示同步读写。
    int usb_serial = open(filename, O_RDONLY | O_NOCTTY | O_SYNC);
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
    // 设置串口的波特率为9600。cfsetospeed 和 cfsetispeed 分别设置输出和输入波特率。
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;//CS8表示数据位为8位
    tty.c_iflag &= ~IGNBRK;                    //IGNBRK表示忽略Break信号
    tty.c_lflag = 0;                           //c_lflag和c_oflag设置为0，禁用特殊处理    
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1;                        //VMIN设置至少接收1个字符
    tty.c_cc[VTIME] = 5;                       //VTIME设置超时时间为5个十分之一秒
    //tty.c_iflag &= ~(ICRNL | INLCR);  // 禁用 \r 到 \n 的转换,和tty.c_oflag = 0;功能相同

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
        return -1;
    }

    return usb_serial;
}

//定义一个函数 read_process，接受串口文件描述符和要读取的字节数。
vector<uint8_t> read_process(int usb_serial, int total_bytes, const vector<uint8_t> frame_header)
{
    uint8_t byte;//声明一个字节变量byte
    deque<uint8_t> buffer;//一个双端队列buffer用于存储读取的数据
    vector<uint8_t> frame(total_bytes);//一个大小为total_bytes的向量frame
    time_t start_time;//记录开始时间
    time(&start_time);
    
    //进入无限循环，检查是否超过3秒，如果是则记录错误并退出
    while (true) 
    {
        time_t end_time;
        time(&end_time);
        
        if((end_time - start_time) > 3) //超过3秒则退出
        {
        
            LOG(ERROR) << "read_process:[overtime 3s]read " << total_bytes <<" bytes usb data error";
            break;
        }
        //从串口读取一个字节。若串口没有数据输入，则程序阻塞，会卡在read函数，直到有数据进入
        int num_bytes = read(usb_serial, &byte, 1);

        //如果读取成功，则将其添加到 buffer 中。
        if (num_bytes > 0) 
        {
            buffer.push_back(byte);
        }

        bool found = false;  
        if(buffer.size() >= 110)
        {
            //检查 buffer 的开头是否与 frame_header 匹配
            if (equal(frame_header.begin(), frame_header.end(), buffer.begin())) 
            {
                found = true;
                copy(buffer.begin(), buffer.begin() + total_bytes, frame.begin());//复制数据到 frame
                buffer.erase(buffer.begin(), buffer.begin() + total_bytes);//从 buffer 中移除已处理的字节     
                return frame; //然后返回 frame
            }
            else 
            {
                buffer.pop_front();//如果不匹配，则移除buffer中的第一个字节
            }
        } 
    }

    return frame;
}


