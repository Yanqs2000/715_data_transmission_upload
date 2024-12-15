#pragma once

#include<stdlib.h>
#include <glog/logging.h>
#include <glog/raw_logging.h>
#include <filesystem>
#include "config.h"
#include <sys/stat.h>
#include <sys/types.h>

void SignalHandler(const char* data, int size);

bool create_directory_with_permissions(const std::string& path);

class GlogHelper 
{   // GlogHelper
public:
    //Glog setting
    GlogHelper(char* program, Parameter& params);
    //
    ~GlogHelper();
};
