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

using namespace std;

int USB_init(const char* filename);

vector<uint8_t> read_process(int usb_serial, int GuanDao_total_bytes);