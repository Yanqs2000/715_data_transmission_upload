#pragma once

#include "ini.h"
#include <iostream>
#include <string>


class Parameter
{
public:
    int FPS = 120;          //帧率
    int EXTIME = 4000;      //曝光时间
    int grab_time = 600;    //每次拍摄时长
    int grab_count = 144;   //拍摄次数
    int Sync_time = 60;     //等待同步时间
    int if_Sync = 1;    //是否开启时间同步
    int INS_flag = 1; //Inertial Navigation System 是否开启惯导

    //glog
    std::string glog_dir = "/home/camera/ssd/glog_file";
};


int read_config(Parameter &params, const char *filename);


