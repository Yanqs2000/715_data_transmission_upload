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

using namespace std;

int USB_init(const char* filename);

vector<uint8_t> read_process(int usb_serial, size_t total_bytes, const vector<uint8_t> frame_header);

// UDP 接收命令线程
void udp_listener_thread();

// 判断/mnt/nas是否为空
int is_directory_empty(const char *dir_path);

// 设置gpio高低电平
bool set_gpio_value(int gpio_line, int value);

// 打印char类型数组的值（16进制）
void printHex(const char* command, size_t length);