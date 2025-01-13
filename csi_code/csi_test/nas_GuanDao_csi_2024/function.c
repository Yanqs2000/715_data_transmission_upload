#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>

#include "ini.h"  // 需要下载并包含 ini.h 库
#include "v4l2_device_control.h"
#include "parameter_parser.h"
#include "function.h"

volatile bool g_quit = false;//用于表示是否需要退出程序
volatile sig_atomic_t data_received = 0; // 新增标志位，表示是否接收到正确的数据包
pthread_mutex_t data_received_mutex = PTHREAD_MUTEX_INITIALIZER; // 保护共享变量的互斥锁
uint8_t initial_command_buffer[32]; // 起始命令数据接收缓冲区
char all_filenames[1024][128] = {0}; // 1024表示文件名个数，128表示文件名长度
FILE *initial_command_file;
FILE *read_initial_command_file = NULL;

int file_cnt = 0;
int nas_cnt = 2;
int Final_File_Flag = 1;

pthread_mutex_t nas_mutex = PTHREAD_MUTEX_INITIALIZER;//互斥锁
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;//等待线程

struct _Params params; // params 结构体用于存储接收数据参数
struct ThreadParams nas_thread_params;
// 预期的接收数据包
uint8_t start_uart6_packet[32] = 
{
    0xF0, 0x00, 0x20, 0x00, 0x0A, 0x0A, 0x0A, 0x0A,
    0x0B, 0x0B, 0x0A, 0x0A, 0xF7, 0xF7, 0x0B, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0D, 0x0A,
};

uint8_t end_uart6_packet[32] = 
{
    0xF0, 0x00, 0x20, 0x00, 0x0B, 0x0A, 0x0A, 0x0A,
    0x0B, 0x0B, 0x0A, 0x0A, 0xF7, 0xF7, 0x0B, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0D, 0x0A,
};

//串口初始化函数
int USB_init_FPGA(const char* serial_device) 
{
    int serial_fd = open(serial_device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd == -1) 
    {
        perror("Failed to open serial uart6");
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

// USB初始化
int USB_init_GuanDao(const char* filename)//接受一个字符指针 filename，表示串口设备的文件名
{
    // 假设usb_serial是您已经配置和打开的串口文件描述符
    // 使用open函数以只读模式打开指定的串口文件。O_NOCTTY 表示不将此设备设为控制终端，O_SYNC 表示同步读写。
    int usb_serial = open(filename, O_RDONLY | O_NOCTTY | O_SYNC);
    if (usb_serial < 0) 
    {
        perror("Failed to open serial uart3");
        return -1;
    }

    //定义一个termios结构体tty来保存串口配置，并将其内容初始化为零
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    // 调用 tcgetattr获取当前串口配置
    if (tcgetattr(usb_serial, &tty) != 0) 
    {
        perror("Error from tcgetattr: uart3");
        return -1;
    }

    // 设置串口的波特率为9600。cfsetospeed 和 cfsetispeed 分别设置输出和输入波特率。
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;//CS8表示数据位为8位
    tty.c_iflag &= ~IGNBRK;                    //IGNBRK表示忽略Break信号
    tty.c_lflag = 0;                           //c_lflag和c_oflag设置为0，禁用特殊处理    
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;                        //VMIN设置至少接收0个字符
    tty.c_cc[VTIME] = 1;                       //VTIME设置超时时间为x个十分之一秒
    tty.c_iflag &= ~(ICRNL | INLCR);  // 禁用 \r 到 \n 的转换,和tty.c_oflag = 0;功能相同

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);    //禁用软件流控制（IXON, IXOFF）
    tty.c_cflag |= (CLOCAL | CREAD);           //启用接收和本地模式（CREAD, CLOCAL）
    tty.c_cflag &= ~(PARENB | PARODD);         //禁用奇偶校验（PARENB, PARODD）
    tty.c_cflag &= ~CSTOPB;                    //设置为一个停止位（CSTOPB）
    tty.c_cflag &= ~CRTSCTS;                   //禁用 RTS/CTS 硬件流控制（CRTSCTS）

    //使用tcsetattr应用配置。如果失败，记录错误并返回-1
    if (tcsetattr(usb_serial, TCSANOW, &tty) != 0)
    {
        perror("Error from tcgetattr: uart3");
        close(usb_serial);
        return -1;
    }

    return usb_serial;
}

// 解析INI文件
int config_handler(void* user, const char* section, const char* name, const char* value) 
{
    struct _Params* params = (struct _Params*)user;
    //printf("Parsing section: %s, name: %s, value: %s\n", section, name, value);  // 调试信息

    if (strcmp(section, "parameters") == 0) 
    {
        if (strcmp(name, "device") == 0) 
        {
            if (params->device == NULL) 
            {
                params->device = strdup(value);  // 只在第一次分配内存时调用 strdup
            }
        } 
        else if (strcmp(name, "type") == 0) 
        {
            if (params->type == NULL) 
            {
                params->type = strdup(value);  // 只在第一次分配内存时调用 strdup
            }
        } 
        else if (strcmp(name, "format") == 0) 
        {
            if (params->format == NULL) 
            {
                params->format = strdup(value);  // 只在第一次分配内存时调用 strdup
            }
        } 
        else if (strcmp(name, "width") == 0) 
        {
            params->width = strtoul(value, NULL, 10);  // 存储宽度
        } 
        else if (strcmp(name, "height") == 0) 
        {
            params->height = strtoul(value, NULL, 10);  // 存储高度
        } 
        else if (strcmp(name, "fps") == 0) 
        {
            params->fps = strtoul(value, NULL, 10);  // 存储帧率
        } 
        else if (strcmp(name, "number") == 0) 
        {
            params->number = strtoul(value, NULL, 10);  // 存储数量
        }
        else if (strcmp(name, "ONE_FILE_FRAMES") == 0) 
        {
            params->one_file_frames = strtoul(value, NULL, 10);  // 存储每个文件的帧数
        }
        else if (strcmp(name, "data_folder") == 0) 
        {
            if (params->data_folder == NULL) 
            {
                params->data_folder = strdup(value);  // 存储数据文件夹路径
            }
        }
        else if (strcmp(name, "nas_folder") == 0) 
        {
            if (params->nas_folder == NULL) 
            {
                params->nas_folder = strdup(value);  // 存储NAS文件夹路径
            }
        }
        else if (strcmp(name, "if_nas") == 0) 
        {
            if (strcmp(value, "true") == 0)
            {
                params->if_nas = true;  // 解析为 true
            }
            else if (strcmp(value, "false") == 0)
            {
                params->if_nas = false;  // 解析为 false
            }
        }
        else if (strcmp(name, "if_GuanDao") == 0) 
        {
            if (strcmp(value, "true") == 0)
            {
                params->if_GuanDao = true;  // 解析为 true
            }
            else if (strcmp(value, "false") == 0)
            {
                params->if_GuanDao = false;  // 解析为 false
            }
        }
        else if (strcmp(name, "if_delete_start_command") == 0) 
        {
            if (strcmp(value, "true") == 0)
            {
                params->if_delete_start_command = true;  // 解析为 true
            }
            else if (strcmp(value, "false") == 0)
            {
                params->if_delete_start_command = false;  // 解析为 false
            }
        }
    }

    return 1;  // 返回非零表示继续处理
}

bool parse_parameter_from_config(struct _Params* params, const char* config_file)
{
    /* default values */
    params->device = NULL;
    params->type = "usb";
    params->format = NULL;
    params->width = 0;
    params->height = 0;
    params->data_folder = NULL;
    params->nas_folder = NULL;

    // 解析config.ini文件
    if (ini_parse(config_file, config_handler, params) < 0) 
    {
        printf("Error reading INI file: %s\n", config_file);
        return false;
    }

    // 检查必须的参数是否存在
    if (params->device == NULL || params->width == 0 || params->height == 0) 
    {
        return false;
    }

    return true;
}

// 网络接收函数（接收到正确的数据包后设置标志）
int receive_network_data(int sockfd, struct sockaddr_in *client_addr) 
{
    socklen_t addr_len = sizeof(*client_addr);

    // 等待数据包到达
    int recv_len = recvfrom(sockfd, initial_command_buffer, sizeof(initial_command_buffer), 0, 
                             (struct sockaddr *)client_addr, &addr_len);
    if (recv_len == -1) 
    {
        perror("recvfrom");
        return -1;
    }

    // for (int i = 0; i < 32; i++) 
    // {
    //     printf("%02X ", initial_command_buffer[i]);

    //     if((i + 1) % 8 == 0)
    //     {
    //         printf("\n");
    //     }
    // }

    // 检查接收到的数据包的帧头和帧尾，并确保数据包的长度为32个字节
    if (initial_command_buffer[0] == 0xF0 && initial_command_buffer[1] == 0x00 
        && initial_command_buffer[2] == 0x20 && initial_command_buffer[3] == 0x00 &&  // 检查帧头
        initial_command_buffer[30] == 0x0D && initial_command_buffer[31] == 0x0A &&  // 检查帧尾
        recv_len == 32)  // 确保数据包大小为32字节
    {
        // 如果帧头和帧尾匹配，并且长度为32字节，设置标志表示已接收
        pthread_mutex_lock(&data_received_mutex);
        data_received = 1;
        pthread_mutex_unlock(&data_received_mutex);

        // 向数据来源发送相同内容的数据包
        if (sendto(sockfd, initial_command_buffer, recv_len, 0, 
                   (struct sockaddr *)client_addr, addr_len) == -1)
        {
            perror("sendto");
            return -1;
        }

        initial_command_file = fopen("/mnt/data/csi_code/csi_test/nas_GuanDao_csi/start_command", "wb+");
        if (initial_command_file == NULL) 
        {
            // Error opening file
            perror("Failed to open file1");
            return -1;
        }

        fwrite(initial_command_buffer, sizeof(char), 32, initial_command_file);
        fclose(initial_command_file);
        printf("create start_command");

        printf("Sent the received packet back to %s:%d\n", 
               inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
        return 1;
    }
    else
    {
        // 数据包不符合要求
        printf("Error: Received data packet does not match expected format or length\n");       
        return 0;
    }
}

// 子线程，网络监听函数
void *network_listener(void *arg) 
{
    int sockfd = *(int *)arg;
    struct sockaddr_in client_addr;
    while (!g_quit) 
    {
        // 等待网络数据包到达并与预期数据包进行比较
        if (receive_network_data(sockfd, &client_addr) > 0) 
        {
            // 数据包匹配后，设置标志表示已接收
            printf("Data packet received, notifying main thread...\n");
            
            // 结束子线程
            break;
        }
    }
    return NULL;
}

//获取当前时间作为文件名
void get_filename_from_time(char *filename, size_t len) 
{
    time_t t = time(NULL); // 获取当前时间
    struct tm tm = *localtime(&t); // 转换为本地时间
    snprintf(filename, len, "SX_%04d%02d%02d_%02d%02d%02d.dat", 
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
             tm.tm_hour, tm.tm_min, tm.tm_sec);
}

// 复制文件上传nas
void copy_file(const char *source_path, const char *dest_path) 
{
    FILE *source_file = fopen(source_path, "rb");
    if (source_file == NULL) 
    {
        perror("Failed to open source file");
        exit(EXIT_FAILURE);
    }

    FILE *dest_file = fopen(dest_path, "wb");
    if (dest_file == NULL) 
    {
        perror("Failed to open destination file");
        fclose(source_file);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, 1024, source_file)) > 0) 
    {
        fwrite(buffer, 1, bytes, dest_file);
    }

    fclose(source_file);
    fclose(dest_file);
}

/*目前在ubuntu20.04、Linux5.10系统中，数据接收的速率最大为18MB/s,数据通过网络上传到nas的速率在30-40MB/s之间*/
/*只有当上传速率大于接收速率时，逻辑才成立；当上传速率小于接收速率时，主线程会不断发信号，不知道对子线程有没有影响*/
// 上传nas子线程
void *Upload_nas(void *arg)
{
    char source_path[128];
    char dest_path[128];
    struct ThreadParams *thread_params = (struct ThreadParams *)arg;
    struct timeval time_start_p, time_end_p, time_diff_p;//time_start_p、time_end_p和time_diff_p用于计算子线程程序运行时间。
    uint32_t cost_time_p;
    while(nas_cnt < (thread_params->num + 2))
    {
        pthread_mutex_lock(&nas_mutex);
    
        // 等待 file_cnt
        if (nas_cnt > file_cnt && Final_File_Flag)//
        {
            printf("child thread wait\n");
            pthread_cond_wait(&cond, &nas_mutex);
        }

        printf("sleep 1 second\n");
        sleep(1);//单位：秒

        pthread_mutex_unlock(&nas_mutex);

        printf("child thread continue\n");

        if (all_filenames[nas_cnt - 1][0] == 0)
        {
            printf("Because the program ended prematurely, next file path do not exist, so over the upload\n");
            break;
        } 

        sprintf(source_path, "%s%s", params.data_folder, all_filenames[nas_cnt - 1]);
        sprintf(dest_path, "%s%s", params.nas_folder, all_filenames[nas_cnt - 1]);

        gettimeofday(&time_start_p, NULL);
        copy_file(source_path, dest_path);
        
        gettimeofday(&time_end_p, NULL);
        timersub(&time_end_p, &time_start_p, &time_diff_p);
        cost_time_p = time_diff_p.tv_sec * 1000.0 + time_diff_p.tv_usec / 1000;

        nas_cnt++;
        printf("\nchild thread time: %d ms\n", cost_time_p);
    }
    return NULL;
}

// 将文件名加入数组
void add_filename_to_array(const char *filename, int index)
{
    if (index < 1024) 
    {
        // 将文件名复制到数组中的指定位置
        strncpy(all_filenames[index], filename, 128 - 1);
        all_filenames[index][128 - 1] = '\0'; // 确保字符串以空字符结尾
    } 
    else 
    {
        printf("Array is full! Cannot add more filenames.\n");
    }
}

// GuanDao读取函数
uint8_t* read_process(int usb_serial, int total_bytes, const uint8_t* frame_header, size_t header_size) 
{

    uint8_t byte;
    uint8_t buffer[1100]; 
    int buffer_index = 0;

    static uint8_t frame[110];

    while (true) 
    {
        // Read one byte from the serial port
        int num_bytes = read(usb_serial, &byte, 1);

        if (num_bytes > 0) 
        {
            // Add the byte to the buffer
            if (buffer_index < sizeof(buffer)) 
            {
                buffer[buffer_index] = byte;
                buffer_index++;
            } 
            else 
            {
                // Buffer overflow, reset the buffer
                buffer_index = 0;
            }
        } 
        else if (num_bytes == 0) 
        {
            printf(" GuanDao no data received\n");
            break;
        } 
        else 
        {
            printf("Error while reading GuanDao\n");
            break;
        }

        if (buffer_index >= total_bytes) 
        {
            bool found = true;

            for (size_t i = 0; i < header_size; ++i) 
            {
                if (buffer[i] != frame_header[i]) 
                {
                    found = false;
                    break;
                }
            }

            if (found) 
            {
                memcpy(frame, buffer, total_bytes); // 复制有效帧到frame
                memmove(buffer, buffer + total_bytes, buffer_index - total_bytes); // 移除已处理的帧
                buffer_index -= total_bytes;
                return frame;
            } 
            else 
            {
                memmove(buffer, buffer + 1, --buffer_index); // Remove the first byte
                printf("data mismatch\n");
            }
        }
    }
    return NULL;
}

// 时间、地点抽取函数
int DataExtractor_raw(const uint8_t *frame, size_t frame_size, uint8_t *date_time, size_t date_time_size)
{
    // Ensure frame and date_time sizes are sufficient
    if (frame_size < 110)
    {
        printf("Data receive error: frame size = %zu\n", frame_size);
        return -1;
    }

    if (date_time_size < 22)
    {
        printf("Data receive error: date_time buffer size is too small\n");
        return -1;
    }

    // Extract relevant data to date_time array
    date_time[0] = frame[6];
    date_time[1] = frame[7];
    date_time[2] = frame[8];
    date_time[3] = frame[9];
    date_time[4] = frame[10];
    date_time[5] = frame[11];
    date_time[6] = frame[12];
    date_time[7] = frame[13];
    date_time[8] = frame[26];
    date_time[9] = frame[27];
    date_time[10] = frame[28];
    date_time[11] = frame[29];
    date_time[12] = frame[30];
    date_time[13] = frame[31];
    date_time[14] = frame[90];
    date_time[15] = frame[91];
    date_time[16] = frame[92];
    date_time[17] = frame[93];
    date_time[18] = frame[94];
    date_time[19] = frame[95];
    date_time[20] = frame[96];
    date_time[21] = frame[97];

    return 1;
}



