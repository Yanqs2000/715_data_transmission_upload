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

//const vector<uint8_t> frame_header = {0x4C, 0x57, 0x3C, 0x00};
const vector<uint8_t> frame_header = {0xAA, 0x55, 0x5A, 0xA5};

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
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;//CS8表示数据位为8位
    tty.c_iflag &= ~IGNBRK;                    //IGNBRK表示忽略Break信号
    tty.c_lflag = 0;                           //c_lflag和c_oflag设置为0，禁用特殊处理
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1;                        //VMIN设置至少接收1个字符
    tty.c_cc[VTIME] = 5;                       //VTIME设置超时时间为5个十分之一秒

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
vector<uint8_t> read_process(int usb_serial, int GuanDao_total_bytes)
{
    // 获取程序启动时间
    auto start_time = std::chrono::high_resolution_clock::now();
    
    //const vector<uint8_t> frame_header;
    uint8_t byte;//声明一个字节变量byte，一个双端队列buffer用于存储读取的数据
    deque<uint8_t> buffer;
    vector<uint8_t> frame(GuanDao_total_bytes);//一个大小为GuanDao_total_bytes的向量frame
    string name;

    if (GuanDao_total_bytes == 60)
    {
        name = "CiLiYi";
    }

    if (GuanDao_total_bytes == 110)
    {
        name = "GuanDao";
    }
    
    //进入无限循环
    while (true) 
    {
        int num_bytes = read(usb_serial, &byte, 1);//从串口读取一个字节。
        
        //如果读取成功，则将其添加到 buffer 中。
        if (num_bytes > 0) 
        {
            buffer.push_back(byte);
        }
        bool found = false;  
        if(buffer.size() >= GuanDao_total_bytes)
        {
            //检查 buffer 的开头是否与 frame_header 匹配
            if (equal(frame_header.begin(), frame_header.end(), buffer.begin())) 
            {
                found = true;
                copy(buffer.begin(), buffer.begin() + GuanDao_total_bytes, frame.begin());//复制数据到 frame
                buffer.erase(buffer.begin(), buffer.begin() + GuanDao_total_bytes);//从 buffer 中移除已处理的字节     
                
                // 计算工作时间
                auto current_time = std::chrono::high_resolution_clock::now();
                auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
                cout << name << " " << elapsed_time.count() << " milliseconds" << endl;
                
                return frame; //然后返回 frame
            }
            else 
            {
                buffer.pop_front();//如果不匹配，则移除buffer中的第一个字节
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


// 串口读取线程函数
// void readSerial(int usb_serial, SafeQueue<uint8_t>& data) {
//     uint8_t byte;
//     while (true) {
//     int num_bytes = read(usb_serial, &byte, 1);
//     if (num_bytes > 0) {
//         // std::lock_guard<std::mutex> lock(mutex);
//         data.push(byte);
//         // cv.notify_one();
//     } 
//         // else if (num_bytes < 0) {
//         //     std::cerr << "Error reading: " << strerror(errno) << std::endl;
//         //     break;
//         // }
        
//     }
// }

// // 数据处理线程函数 团结就是力量，团结就是力量，这力量是铁，这力量是钢
// void processDataFrame(std::vector<uint8_t> frame) {
//     // 在这里处理帧数据
//     // ...
//     std::cout << "Processing a frame of data..." << std::endl;
//     DataExtractor(frame);

// }

// 数据处理线程函数
// void processData(SafeQueue<uint8_t>& buffer, SafeVector<uint8_t>& data) {
//     // std::unique_lock<std::mutex> lock(mutex);
//     std::vector<uint8_t> frame(GuanDao_total_bytes);
//     // frame.reserve(GuanDao_total_bytes);
//     while (true) {
//         // cv.wait(lock, [] { return buffer.size() >= GuanDao_total_bytes; });

//         // 寻找帧头
//         bool found = false;    
//         while (buffer.size() >= GuanDao_total_bytes && !found) 
//         {
//             //  std::cout<<"buffer size0: "<< std::dec<<buffer.size() <<std::endl;
//             // auto start = std::chrono::high_resolution_clock::now();
//             if (std::equal(frame_header.begin(), frame_header.end(), buffer.begin())) {
//                 found = true;
//                 std::copy(buffer.begin(), buffer.begin() + GuanDao_total_bytes, frame.begin());
//                 buffer.erase(GuanDao_total_bytes);

//             } else {
//                 buffer.pop();
//             }
//         }
//         // lock.unlock();
//         if (found) {
//             if (frame.capacity() == GuanDao_total_bytes) {
//                 data.clear();
//                 data.copyfrom(frame);
//             }

//             // for(int i = 0; i < GuanDao_total_bytes; ++i)
//             // {
//             //     std::cout<< std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
//             // }
//             // std::cout<<std::endl;
//         }
//         // lock.lock();
//     }
// }
