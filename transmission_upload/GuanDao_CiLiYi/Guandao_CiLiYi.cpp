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
#include <chrono>  // for high_resolution_clock
#include <iomanip>

using namespace std;

extern int USB_init(const char* filename);
extern vector<uint8_t> read_process(int usb_serial, int total_bytes, const vector<uint8_t> frame_header);
extern int DataExtractor_raw(vector<uint8_t> &frame, vector<std::uint32_t> &date_time);
extern void get_filename_from_time(char *filename, size_t len);

const vector<uint8_t> GuanDao_frame_header = {0xAA, 0x55, 0x5A, 0xA5};//定义惯导帧头
const vector<uint8_t> CiLiYi_frame_header = {0x4C, 0x57, 0x3C, 0x00};//定义磁力仪帧头


int main(int argc, char* argv[]) 
{
    const char* log_dir = "/mnt/data/GuanDao_CiLiYi/logs/";
    struct stat log_info;
    if (stat(log_dir, &log_info) != 0) 
    {
        mkdir(log_dir, 0777);  // Create logs directory with full permissions
    }

    const char* data_dir = "/mnt/data/GuanDao_CiLiYi/data/";
    struct stat data_info;
    if (stat(data_dir, &data_info) != 0) 
    {
        mkdir(data_dir, 0777);  // Create data directory with full permissions
    }

    const char* data_nas_dir = "/mnt/nas/GuanDao_CiLiYi/data/";
    struct stat data_nas_info;
    if (stat(data_nas_dir, &data_nas_info) != 0) 
    {
        mkdir(data_nas_dir, 0777);  // Create data directory with full permissions
    }

    // Initialize glog
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = log_dir;  // Set log directory
    FLAGS_logtostderr = false; // Only output to file

    const char* usb_device_GuanDao = "/dev/ttyS3";  // ttyS3 for GuanDao (惯导)
    const char* usb_device_CiLiYi = "/dev/ttyS4";  // ttyS4 for CiLiYi (磁力仪)
    int GuanDao_total_bytes = 110;  // Bytes to read from GuanDao
    int CiLiYi_total_bytes = 60;    // Bytes to read from CiLiYi
    char file_name_time[64];
    char output_file[128];
    char output_file_nas[128];
    get_filename_from_time(file_name_time, sizeof(file_name_time)); // 生成文件名
    
    // 指定文件路径
    snprintf(output_file, sizeof(output_file), "%s%s", data_dir, file_name_time); // 构建完整的文件路径
    snprintf(output_file_nas, sizeof(output_file_nas), "%s%s", data_nas_dir, file_name_time); // 构建完整的文件路径

    // Initialize USB ports
    int usb_serial_GuanDao = USB_init(usb_device_GuanDao);
   
    if (usb_serial_GuanDao < 0) 
    {
        LOG(ERROR) << "Failed to initialize GuanDao USB serial: " << strerror(errno);
        cerr << "Failed to initialize GuanDao USB serial." << endl;
        return -1;
    }

    int usb_serial_CiLiYi = USB_init(usb_device_CiLiYi);
    
    if (usb_serial_CiLiYi < 0) 
    {
        LOG(ERROR) << "Failed to initialize CiLiYi USB serial: " << strerror(errno);
        cerr << "Failed to initialize CiLiYi USB serial." << endl;
        return -1;
    }

    // 获取程序启动时间
    auto start_time = std::chrono::high_resolution_clock::now();

    // Main data reading loop
    while (true) 
    {
        
        // 计算工作时间
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);

        // 检查是否已经工作超过1分钟
        if (elapsed_time.count() >= 60) 
        {
            cout << "work time over" << endl;
            break;  // 跳出循环
        }
        
        vector<uint8_t> data_raw_CiLiYi = read_process(usb_serial_CiLiYi, CiLiYi_total_bytes, CiLiYi_frame_header);
        // cout << "data_raw_CiLiYi = "<< std::hex << std::setw(2) << std::setfill('0')
        //      << static_cast<int>(data_raw_CiLiYi[58]) << std::endl;
        vector<uint8_t> data_raw_GuanDao = read_process(usb_serial_GuanDao, GuanDao_total_bytes, GuanDao_frame_header);

        vector<uint8_t> data_CiLiYi, data_GuanDao;
        
        // Process CiLiYi (magnetometer) data
        if (!data_raw_CiLiYi.empty()) 
        {
            // If CiLiYi data exists, use it
            data_CiLiYi = data_raw_CiLiYi;
            
            // If GuanDao data exists, use it; otherwise, fill with zeros
            if (!data_raw_GuanDao.empty()) 
            {
                DataExtractor_raw(data_raw_GuanDao, data_GuanDao);
            } 
            else 
            {
                data_GuanDao.assign(GuanDao_total_bytes, 0);  // Fill GuanDao data with zeros
            }

            // Write data to file
            ofstream outfile(output_file, ios::binary | ios::app);
            ofstream outfile_nas(output_file_nas, ios::binary | ios::app);
            if (outfile && outfile_nas) 
            {
                outfile.write(reinterpret_cast<const char*>(data_CiLiYi.data()), data_CiLiYi.size());
                outfile.write(reinterpret_cast<const char*>(data_GuanDao.data()), data_GuanDao.size());
                
                outfile_nas.write(reinterpret_cast<const char*>(data_CiLiYi.data()), data_CiLiYi.size());
                outfile_nas.write(reinterpret_cast<const char*>(data_GuanDao.data()), data_GuanDao.size());

                outfile.close();
                outfile_nas.close();
                LOG(INFO) << "Data written to " << output_file;
                cout << "Data written to " << output_file << endl;
            } 
            else 
            {
                LOG(ERROR) << "Error opening file for writing: " << strerror(errno);
                cerr << "Error opening file for writing" << strerror(errno) << endl;
            }
        } 
        else 
        {
            LOG(WARNING) << "No CiLiYi data received, skipping write operation.";
            cerr << "No CiLiYi data received, skipping write operation." << endl;
        }

        // Clear variables for next loop
        data_raw_CiLiYi.clear();
        data_raw_GuanDao.clear();
        data_CiLiYi.clear();
        data_GuanDao.clear();

        // Sleep to prevent fast looping
        usleep(50000);  // Sleep for 50 milliseconds
    }

    // Close USB serial ports
    close(usb_serial_GuanDao);
    close(usb_serial_CiLiYi);
    google::ShutdownGoogleLogging();  // Close glog

    return 0;
}
// 编译命令
// g++ -o Guandao_CiLiYi Guandao_CiLiYi.cpp DataExtractor.cpp Thread_ReadUSB.cpp -lglog
// 问题：磁力仪数据FF->FD->FB
// 不同时启动的问题