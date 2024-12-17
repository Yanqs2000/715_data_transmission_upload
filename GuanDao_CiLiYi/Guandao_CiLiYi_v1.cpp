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
extern vector<uint8_t> read_process(int usb_serial, int total_bytes, const vector<uint8_t> frame_header);
extern int DataExtractor_raw(vector<uint8_t> &frame, vector<std::uint32_t> &date_time);

const vector<uint8_t> GuanDao_frame_header = {0xAA, 0x55, 0x5A, 0xA5};//定义惯导帧头
const vector<uint8_t> CiLiYi_frame_header = {0x4C, 0x57, 0x3C, 0x00};//定义磁力仪帧头

int main(int argc, char* argv[]) 
{
    const char* log_dir = "./logs";
    struct stat info;
    if (stat(log_dir, &info) != 0) 
    {
        mkdir(log_dir, 0777);  // Create logs directory with full permissions
    }
    
    // 初始化glog
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = log_dir;  // 设置日志文件目录
    FLAGS_logtostderr = false; // 仅输出到文件
    
    const char* usb_device_GuanDao = "/dev/ttyS3";  // ttyS3为惯导串口输入
    const char* usb_device_CiLiYi = "/dev/ttyS4";  // ttyS4为磁力仪串口输入 
    int GuanDao_total_bytes = 110;  // 惯导需要读取的字节数为110
    int CiLiYi_total_bytes = 60;    // 磁力仪需要读取的字节数为60
    const char* output_file = "output_data.dat";  // 输出文件名

    // 初始化惯导USB串口
    int usb_serial_GuanDao = USB_init(usb_device_GuanDao);
    if (usb_serial_GuanDao < 0) 
    {
        LOG(ERROR) << "Failed to initialize GuanDao USB serial." << strerror(errno);  // 使用glog记录错误
        cerr << "Failed to initialize GuanDao USB serial." << endl; // 将错误消息输出到标准错误流,表示USB串口初始化失败
        return -1;
    }

    // 初始化磁力仪USB串口
    int usb_serial_CiLiYi = USB_init(usb_device_CiLiYi);
    if (usb_serial_CiLiYi < 0) 
    {
        cerr << "Failed to initialize CiLiYi USB serial." << endl; // 将错误消息输出到标准错误流,表示USB串口初始化失败
        LOG(ERROR) << "Failed to initialize CiLiYi USB serial." << strerror(errno);
        return -1;
    }
    
    // 从串口读取数据
    while (true) 
    {
        vector<uint8_t> data_raw_GuanDao = read_process(usb_serial_GuanDao, GuanDao_total_bytes, GuanDao_frame_header);
        vector<uint8_t> data_raw_CiLiYi = read_process(usb_serial_CiLiYi, CiLiYi_total_bytes, CiLiYi_frame_header);
        vector<uint8_t> data_GuanDao;
        vector<uint8_t> data_CiLiYi;

        DataExtractor_raw(data_raw_GuanDao, data_GuanDao);
        data_CiLiYi = data_raw_CiLiYi;
        
        // 检查读取的数据有效性
        if (!data_GuanDao.empty() && !data_CiLiYi.empty()) 
        {
            // 将接收到的数据写入文件
            ofstream outfile(output_file, ios::binary | ios::app);  // 以二进制模式打开并清空文件
            if (outfile) 
            {
                outfile.write(reinterpret_cast<const char*>(data_CiLiYi.data()), data_CiLiYi.size());
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
    close(usb_serial_CiLiYi);
    google::ShutdownGoogleLogging();  // 程序结束时关闭glog
    return 0;
}
// 编译命令
// g++ -o Guandao_CiLiYi Guandao_CiLiYi.cpp DataExtractor.cpp Thread_ReadUSB.cpp -lglog
// 还需要加nas上传功能，文件名按照时间命名（但是文件一个月才能写满，所以多少字节一个文件需要跟甲方商量）

// 应该优先保存磁力仪数据，如果没有磁力仪数据，那么惯导数据无需保存；有磁力仪数据，但是没有惯导数据，则用0代替，需要保存