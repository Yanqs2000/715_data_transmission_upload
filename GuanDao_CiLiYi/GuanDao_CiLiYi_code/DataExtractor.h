#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <string.h>

using namespace std;

struct _Params {
    string      data_dir;
    string      log_dir;
    string      nas_dir;
    bool        if_nas;
    bool        if_GuanDao;
    uint32_t    work_time;
    bool        if_delete_start_command;  
};// params结构体用于存储接收数据参数

//获取当前时间作为文件名
void get_filename_from_time(char *filename, size_t len);

// 用于将四个字节转换为一个32位整数（小端字节序）
uint32_t bytesToUint32(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);

// 用于将两个字节转换为一个16位整数（小端字节序）
uint16_t bytesToUint16(uint8_t b0, uint8_t b1);

int DataExtractor_raw(std::vector<uint8_t> &frame, std::vector<uint8_t> &date_time);