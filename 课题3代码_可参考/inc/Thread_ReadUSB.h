#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <cstring>
#include <iomanip> // 添加iomanip库以支持设置输出格式
#include <chrono>
#include "DataExtractor.h"
#include "thread_queue.h"
#include "gloghelper.h"
//多线程处理USB串口数据
int thread_test();

int USB_init(const char* filename);

// 串口读取线程函数
void readSerial(int usb_serial, SafeQueue<uint8_t>& data);

// 线性处理
void readSerial(int usb_serial, std::vector<uint8_t>& data);

// 数据处理线程函数
void processData(SafeQueue<uint8_t>& buffer, SafeVector<uint8_t>& data);

// 线性处理
void processData(std::vector<uint8_t>& buffer, std::vector<uint8_t>& data);

// 数据处理线程函数
// void processDataFrame(std::vector<uint8_t> frame);

std::vector<uint8_t> read_process(int usb_serial, int GuanDao_total_bytes);