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

#include "ini.h"  // 需要下载并包含 ini.h 库
#include "v4l2_device_control.h"
#include "parameter_parser.h"

struct ThreadParams 
{
    int num;
};

// 串口初始化
int USB_init_FPGA(const char* serial_device);//串口6(FPGA)初始化函数
int USB_init_GuanDao(const char* filename);//串口3(GuanDao)初始化函数

// 解析INI文件
int config_handler(void* user, const char* section, const char* name, const char* value);
bool parse_parameter_from_config(struct _Params* params, const char* config_file);

// 接收网络数据
int receive_network_data(int sockfd, struct sockaddr_in *client_addr);
void *network_listener(void *arg);

// 获取当前时间作为文件名
void get_filename_from_time(char *filename, size_t len);

// 上传nas子线程
void *Upload_nas(void *arg);
void copy_file(const char *source_path, const char *dest_path);

// 将文件名加入数组
void add_filename_to_array(const char *filename, int index);

// GuanDao读取函数,时间、地点信息抽取函数
uint8_t* read_process(int usb_serial, int total_bytes, const uint8_t* frame_header, size_t header_size);
int DataExtractor_raw(const uint8_t *frame, size_t frame_size, uint8_t *date_time, size_t date_time_size);
