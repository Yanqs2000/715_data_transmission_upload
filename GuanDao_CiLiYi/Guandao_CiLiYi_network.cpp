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
#include <signal.h>
#include <numeric> // For std::accumulate
#include "INIReader.h"
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

extern int USB_init(const char* filename);
extern vector<uint8_t> read_process(int usb_serial, size_t total_bytes, const vector<uint8_t> frame_header);
extern int DataExtractor_raw(vector<uint8_t> &frame, vector<std::uint32_t> &date_time);
extern void get_filename_from_time(char *filename, size_t len);

extern struct _Params params;

const vector<uint8_t> GuanDao_frame_header = {0xAA, 0x55, 0x5A, 0xA5};//定义惯导帧头
const vector<uint8_t> CiLiYi_frame_header = {0x4C, 0x57, 0x3C, 0x00};//定义磁力仪帧头

int usb_serial_GuanDao;
int usb_serial_CiLiYi;

int CX_data_package = 0;
int GuanDao_data_package = 0;
int read_process_count = 0;
int write_process_count = 0;

ofstream outfile;
ofstream outfile_nas;
ofstream initial_command;

// 临时中断
void signal_handler(int signal) 
{
    close(usb_serial_GuanDao);
    close(usb_serial_CiLiYi);
    LOG(WARNING) << "Program Interrupts";
    cout << "\nProgram Interrupts" << endl;
    google::ShutdownGoogleLogging();  // Close glog
    cout << "CX_data_package = " << CX_data_package << " GuanDao_data_package = " << GuanDao_data_package << " read_process_count = " << read_process_count << " write_process_count = " << write_process_count << endl;
    exit(0);
}

void check_and_increment(const std::vector<uint8_t>& data_raw,int total_bytes) 
{
    // 检查向量是否全为 0
    bool all_zero = std::all_of(data_raw.begin(), data_raw.end(), [](uint8_t value) 
    {
        return value == 0;
    });

    // 如果不全为 0，则计数器加 1
    if (!all_zero && total_bytes == 60) 
    {
        CX_data_package++;
    }
    else if(!all_zero && total_bytes == 110)
    {
        GuanDao_data_package++;
    } 
}

// UDP 接收命令线程
void udp_listener_thread(const char* output_file, const char* output_file_nas)
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

    char initial_command_buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    while (true) 
    {
        ssize_t n = recvfrom(udp_socket, initial_command_buffer, sizeof(initial_command_buffer), 0, (struct sockaddr*)&client_addr, &len);
        if (n > 0) 
        {
            initial_command_buffer[n] = '\0'; // Null-terminate received data
            cout << "Received command from client: " << initial_command_buffer << endl;

            if (initial_command_buffer[0] == 0x81 && initial_command_buffer[1] == 0x82 
                && initial_command_buffer[3] == 0x0D && initial_command_buffer[4] == 0x0A && 
                initial_command_buffer[5] == 0x4C && initial_command_buffer[6] == 0x57 && 
                initial_command_buffer[27] == 0x00 && initial_command_buffer[28] == 0x00 && 
                initial_command_buffer[29] == 0x0D && initial_command_buffer[30] == 0x0A  // 检查帧尾
                )  // 确保数据包大小为31字节
                {
                    initial_command.open("start_command", ios::binary | ios::trunc);
                    // 将接收到的起始命令写入文件
                    initial_command.write(initial_command_buffer, 31);


                    outfile.open(output_file, ios::binary | ios::app);
                    outfile.write(initial_command_buffer, 31);
                    if (params.if_nas == true)
                    {
                        outfile_nas.open(output_file_nas, ios::binary | ios::app);
                        outfile_nas.write(initial_command_buffer, 31);
                    }
            
                    // 关闭文件
                    initial_command.close();
                    outfile.close();
                    if (params.if_nas == true)
                    {
                        outfile_nas.close();
                    }
                    break;
                }
                else
                {
                    // 数据包不符合要求
                    cout << "Error: Received data packet does not match expected format or length" << endl;
                    cout << "continue to receive right initial command" << endl; 
                }
        }
    }

    close(udp_socket);
}


int main(int argc, char* argv[]) 
{
    vector<uint8_t> data_raw_GuanDao;
    vector<uint8_t> data_raw_CiLiYi;
    vector<uint8_t> data_CiLiYi;
    vector<uint8_t> data_GuanDao;
    INIReader reader("config.ini");

    string base_command = "sudo mkdir -p ";

    if (reader.ParseError() < 0) 
    {
        std::cout << "Can't load 'config.ini'\n";
        return 1;
    }

    params.data_dir = reader.Get("path", "data_dir", "UNKNOWN");
    params.log_dir = reader.Get("path", "log_dir", "UNKNOWN");
    params.nas_dir = reader.Get("path", "nas_dir", "UNKNOWN");
    params.if_nas = reader.GetBoolean("other", "if_nas", false);
    params.if_GuanDao = reader.GetBoolean("other", "if_GuanDao", false);
    params.work_time = reader.GetInteger("other", "work_time", -1);
    
    //const char* log_dir = "/mnt/data/GuanDao_CiLiYi/logs/";
    const std::string log_dir = params.log_dir;
    struct stat log_info;
    // 使用 c_str() 将 std::string 转换为 const char*
    if (stat(log_dir.c_str(), &log_info) != 0) 
    {
        mkdir(log_dir.c_str(), 0777);  // 使用 c_str() 来获取 const char*
    }

    //const char* data_dir = "/mnt/data/GuanDao_CiLiYi/data/";
    const std::string data_dir = params.data_dir;
    struct stat data_info;
    if (stat(data_dir.c_str(), &data_info) != 0) 
    {
        mkdir(data_dir.c_str(), 0777);  // Create data directory with full permissions
    }

    //const char* data_nas_dir = "/mnt/nas/GuanDao_CiLiYi/data/";
    const std::string data_nas_dir = params.nas_dir;

    if (access(data_nas_dir.c_str(), F_OK) == 0 && params.if_nas == true)
    {
        cout << "nas exist" << endl;
    }
    else if(access(data_nas_dir.c_str(), F_OK) == -1 && params.if_nas == true)
    {
        // 执行挂载命令
        int ret = system("sudo systemctl start camera.service");
        if (ret == -1) 
        {
            perror("Failed to start camera.service");
            return -1;
        }
        sleep(2); // 等待挂载完成

        struct stat data_nas_info;
        if (stat(data_nas_dir.c_str(), &data_nas_info) != 0) 
        {
            string full_command = base_command + data_nas_dir;
            
            int ret = system(full_command.c_str());
            if (ret == -1) 
            {
                perror("Failed to execute mkdir command");
                return -1;
            }
        }
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
    snprintf(output_file, sizeof(output_file), "%s%s", data_dir.c_str(), file_name_time); // 构建完整的文件路径
    snprintf(output_file_nas, sizeof(output_file_nas), "%s%s", data_nas_dir.c_str(), file_name_time); // 构建完整的文件路径

    // Initialize USB ports
    usb_serial_GuanDao = USB_init(usb_device_GuanDao);
   
    if (usb_serial_GuanDao < 0) 
    {
        LOG(ERROR) << "Failed to initialize GuanDao USB serial: " << strerror(errno);
        cerr << "Failed to initialize GuanDao USB serial." << endl;
        return -1;
    }

    usb_serial_CiLiYi = USB_init(usb_device_CiLiYi);
    
    if (usb_serial_CiLiYi < 0) 
    {
        LOG(ERROR) << "Failed to initialize CiLiYi USB serial: " << strerror(errno);
        cerr << "Failed to initialize CiLiYi USB serial." << endl;
        return -1;
    }

    signal(SIGINT, signal_handler);

    // Start UDP listener in a separate thread
    std::thread udp_thread(udp_listener_thread, outfile, outfile_nas);
    udp_thread.join(); // Wait until it finishes

    // 获取程序启动时间
    auto start_time = std::chrono::high_resolution_clock::now();

    // Main data reading loop
    while (true) 
    {
        // 计算工作时间
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);

        // 检查是否已经工作超过x秒
        if (elapsed_time.count() >= params.work_time) 
        {
            cout << "work time over" << endl;
            break;  // 跳出循环
        }
        
        if (params.if_GuanDao == true)
        {
            data_raw_GuanDao = read_process(usb_serial_GuanDao, GuanDao_total_bytes, GuanDao_frame_header);
            //cout << "data_raw_GuanDao = "<< std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data_raw_GuanDao[8]) << std::endl;
            DataExtractor_raw(data_raw_GuanDao, data_GuanDao);
        }

        data_raw_CiLiYi = read_process(usb_serial_CiLiYi, CiLiYi_total_bytes, CiLiYi_frame_header);
        //cout << "data_raw_CiLiYi = "<< std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data_raw_CiLiYi[8]) << std::endl;
        read_process_count++;

        // 检测数据中是否全是0
        check_and_increment(data_raw_CiLiYi,CiLiYi_total_bytes);
        if (params.if_GuanDao == true)
        {
            check_and_increment(data_raw_GuanDao,GuanDao_total_bytes);
        }
        
        data_CiLiYi = data_raw_CiLiYi;
        
        // Write data to file
        outfile.open(output_file, ios::binary | ios::app);
        if (params.if_nas == true)
        {
            outfile_nas.open(output_file_nas, ios::binary | ios::app);
        }

        if (outfile) 
        {
            outfile.write(reinterpret_cast<const char*>(data_CiLiYi.data()), data_CiLiYi.size());
            
            if (params.if_GuanDao == true)
            {
                outfile.write(reinterpret_cast<const char*>(data_GuanDao.data()), data_GuanDao.size());
            }
    
            if (access(data_nas_dir.c_str(), F_OK) == 0 && params.if_nas == true)
            {
                outfile_nas.write(reinterpret_cast<const char*>(data_CiLiYi.data()), data_CiLiYi.size());
                if (params.if_GuanDao == true)
                {
                    outfile_nas.write(reinterpret_cast<const char*>(data_GuanDao.data()), data_GuanDao.size());
                }
            }

            write_process_count++;
            outfile.close();
            outfile_nas.close();
            LOG(INFO) << "Data written to " << output_file;
            //cout << "Data written to " << output_file << endl;
        } 
        else 
        {
            LOG(ERROR) << "Error opening file for writing: " << strerror(errno);
            cerr << "Error opening file for writing" << strerror(errno) << endl;
        }

        // Clear variables for next loop
        data_raw_CiLiYi.clear();
        data_raw_GuanDao.clear();
        data_CiLiYi.clear();
        data_GuanDao.clear();

        // Sleep to prevent fast looping
        usleep(1000);  // Sleep for 1 milliseconds
    }

    // Close USB serial ports
    close(usb_serial_GuanDao);
    close(usb_serial_CiLiYi);
    google::ShutdownGoogleLogging();  // Close glog

    cout << "CX_data_package = " << CX_data_package << " GuanDao_data_package = " << GuanDao_data_package << " read_process_count = " << read_process_count << " write_process_count = " << write_process_count << endl;
    return 0;
}
// 编译命令
// g++ -o Guandao_CiLiYi Guandao_CiLiYi_network.cpp DataExtractor.cpp Thread_ReadUSB.cpp INIReader.cpp ini.c -lglog
// 目前能完整接收、不丢数据，只是接收较慢,可能是波特率太低的问题，需要时间来进行传输
// 不同时启动的问题
// 磁力仪和惯导中，read_pcocess有调换，
// 在有数据输入时，惯导180ms，惯导0ms，???





