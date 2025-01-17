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
#include <signal.h>
#include <chrono>
#include <csignal>


using namespace std;

extern int USB_init(const char* filename);
extern vector<uint8_t> read_process(int usb_serial, int GuanDao_total_bytes);
extern int DataExtractor_raw(vector<uint8_t> &frame, vector<std::uint32_t> &date_time);

int usb_serial;
string name;
std::chrono::time_point<std::chrono::high_resolution_clock> start_time_1;

int read_number = 0;
int write_number = 0;

// 临时中断
void signal_handler(int signal) 
{
    cout << "\nProgram Interrupts" << endl;
    cout << "read_number: " << read_number << endl;
    cout << "write_number: " << write_number << endl;
    // 计算工作时间
    //auto current_time_1 = std::chrono::high_resolution_clock::now();
    //auto elapsed_time_1 = std::chrono::duration_cast<std::chrono::seconds>(current_time_1 - start_time_1);
    //cout << name << " " << elapsed_time_1.count() << " seconds" << endl;
    close(usb_serial);
    exit(0);
}


int main() 
{
    //const char* usb_device = "/dev/ttyS3";  // ttyS3为惯导串口输入
    const char* usb_device = "/dev/ttyS4";  // ttyS4为磁力仪串口输入 
    int GuanDao_total_bytes = 110;  // 根据你的协议设置需要读取的字节数
    const char* output_file = "output_data.bin";  // 输出文件名

    string name;
    if (GuanDao_total_bytes == 60)
    {
        name = "CiLiYi";
    }

    if (GuanDao_total_bytes == 110)
    {
        name = "GuanDao";
    }

    // 初始化USB串口
    usb_serial = USB_init(usb_device);
    if (usb_serial < 0) 
    {
        cerr << "Failed to initialize USB serial." << endl; // 将错误消息输出到标准错误流,表示USB串口初始化失败
        return -1;
    }

    signal(SIGINT, signal_handler);
    // 获取程序启动时间
    start_time_1 = std::chrono::high_resolution_clock::now();

    // 从串口读取数据
    while (true) 
    {
        vector<uint8_t> data_raw = read_process(usb_serial, GuanDao_total_bytes);
        read_number++;
        vector<uint8_t> data;
        //DataExtractor_raw(data_raw, data);
        data = data_raw;
        
        // 检查读取的数据有效性
        if (!data.empty()) 
        {
            // 将接收到的数据写入文件
            ofstream outfile(output_file, ios::binary | ios::app);  // 以二进制模式打开
            if (outfile) 
            {
                outfile.write(reinterpret_cast<const char*>(data.data()), data.size());
                write_number++;
                outfile.close();
                cout << "Data written to " << output_file << endl;
            } 
            else 
            {
                cerr << "Error opening file for writing: " << strerror(errno) << endl;
            }
        } 
        else 
        {
            cerr << "No valid data received or timeout occurred." << endl;
        }

        // 适当的休眠以防止过快循环
        usleep(1000);  // 休眠1毫秒
    }
    cout << "read_number: " << read_number << endl;
    cout << "write_number: " << write_number << endl;

    // 计算工作时间
    //auto current_time_1 = std::chrono::high_resolution_clock::now();
    //auto elapsed_time_1 = std::chrono::duration_cast<std::chrono::seconds>(current_time_1 - start_time_1);
    //cout << name << " " << elapsed_time_1.count() << " seconds" << endl;
    // 关闭串口
    close(usb_serial);
    return 0;
}
// 编译命令
// g++ -o Guandao Guandao.cpp DataExtractor.cpp Thread_ReadUSB.cpp -lglog