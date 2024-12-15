#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip> // 添加iomanip库以支持设置输出格式
#include "thread_queue.h"
#include "gloghelper.h"
uint16_t calChecksum(SafeVector<uint8_t> &data, int size);
uint16_t calChecksum(std::vector<uint8_t> &data, int size);