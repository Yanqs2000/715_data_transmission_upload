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
#include <fstream>   // for ofstream
#include "DataExtractor.h"
#include "Thread_ReadUSB.h"
#include <glog/logging.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

extern int USB_init(const char* filename);

//定义一个函数 read_process，接受串口文件描述符和要读取的字节数。
vector<uint8_t> read_process(int usb_serial, int total_bytes)
{
    uint8_t byte;//声明一个字节变量byte
    deque<uint8_t> buffer;//一个双端队列buffer用于存储读取的数据
    vector<uint8_t> frame(total_bytes);//一个大小为total_bytes的向量frame

    int num_bytes = read(usb_serial, &byte, 1);//从串口读取一个字节。

    //如果读取成功，则将其添加到 buffer 中。
    if (num_bytes > 0) 
    {
        buffer.push_back(byte);
    }
    // 将 buffer 中的数据复制到 frame 中
    copy(buffer.begin(), buffer.begin() + total_bytes, frame.begin());

    // 清除 buffer 中的数据
    buffer.erase(buffer.begin(), buffer.begin() + total_bytes);
    
    return frame;
}

int main(int argc, char* argv[]) 
{ 
    const char* usb_device_GuanDao = "/dev/ttyS3";  // ttyS3为惯导串口输入
    int GuanDao_total_bytes = 1;  // 惯导需要读取的字节数为110
    const char* output_file = "output_data.dat";  // 输出文件名

    // 初始化惯导USB串口
    int usb_serial_GuanDao = USB_init(usb_device_GuanDao);
    if (usb_serial_GuanDao < 0) 
    {
        LOG(ERROR) << "Failed to initialize GuanDao USB serial." << strerror(errno);  // 使用glog记录错误
        cerr << "Failed to initialize GuanDao USB serial." << endl; // 将错误消息输出到标准错误流,表示USB串口初始化失败
        return -1;
    }

    // 从串口读取数据
    while (true) 
    {
        vector<uint8_t> data_raw_GuanDao = read_process(usb_serial_GuanDao, GuanDao_total_bytes);
        vector<uint8_t> data_GuanDao = data_raw_GuanDao;
        
        // 检查读取的数据有效性
        if (!data_GuanDao.empty()) 
        {
            // 将接收到的数据写入文件
            ofstream outfile(output_file, ios::binary | ios::app);  // 以二进制模式打开并清空文件
            if (outfile) 
            {
                outfile.write(reinterpret_cast<const char*>(data_GuanDao.data()), data_GuanDao.size());
                outfile.close();
                LOG(INFO) << "Data written to " << output_file;  // 记录写入信息
                cout << "Data written to " << output_file << endl;
            } 
            else 
            {
                cerr << "Error opening file for writing" << strerror(errno) << endl;
                LOG(ERROR) << "Error opening file for writing" << strerror(errno);
            }
        } 
        else 
        {
            cerr << "No valid data received or timeout occurred." << endl;
            LOG(WARNING) << "No valid data received or timeout occurred.";
        }

        // 适当的休眠以防止过快循环
        usleep(50000);  // 休眠50毫秒
    }
    // 关闭串口
    close(usb_serial_GuanDao);
    return 0;
}
// 编译命令
// g++ -o test test.cpp DataExtractor.cpp Thread_ReadUSB.cpp -lglog
