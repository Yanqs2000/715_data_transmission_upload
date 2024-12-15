
#pragma once
#include <thread>
#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include "gloghelper.h"

int sync_run_raw(std::vector<std::uint32_t> &date_time);

//将存放日期和时间的数组转换为时间戳形式
int get_inctimestr_raw(std::vector<std::uint32_t>& date_time, std::string& timestr);