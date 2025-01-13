// Example that shows simple usage of the INIReader class

#include <iostream>
#include "INIReader.h"
#include <string.h>

using namespace std;

struct _Params {
    string      data_dir;
    string      log_dir;
    string      nas_dir;
    bool        if_nas;
    bool        if_GuanDao;
    uint32_t    work_time;  
} params;// params 结构体用于存储接收数据参数

int main()
{
    INIReader reader("config.ini");

    if (reader.ParseError() < 0) 
    {
        std::cout << "Can't load 'config.ini'\n";
        return 1;
    }

    params.data_dir = reader.Get("path", "data_dir", "UNKNOWN");
    params.log_dir = reader.Get("path", "log_dir", "UNKNOWN");
    params.nas_dir = reader.Get("path", "nas_dir", "UNKNOWN");

    params.if_nas = reader.GetBoolean("other", "if_nas", false);
    params.if_GuanDao = reader.GetBoolean("other", "if_GuanDao", false);

    params.work_time = reader.GetInteger("other", "work_time", -1);


    cout << "data_dir: " << params.data_dir << endl;
    cout << "log_dir: " << params.log_dir << endl;
    cout << "nas_dir: "<< params.nas_dir << endl;

    cout << "if_nas: "<< params.if_nas << endl;
    cout << "if_GuanDao: "<< params.if_GuanDao<< endl;

    cout << "work_time: " << params.work_time << endl;


    // std::cout << "Config loaded from 'test.ini': version="
    //           << reader.GetInteger("protocol", "version", -1) << ", unsigned version="
    //           << reader.GetUnsigned("protocol", "version", -1) << ", trillion="
    //           << reader.GetInteger64("user", "trillion", -1) << ", unsigned trillion="
    //           << reader.GetUnsigned64("user", "trillion", -1) << ", name="
    //           << reader.Get("user", "name", "UNKNOWN") << ", email="
    //           << reader.Get("user", "email", "UNKNOWN") << ", pi="
    //           << reader.GetReal("user", "pi", -1) << ", active="
    //           << reader.GetBoolean("user", "active", true) << "\n";
    // std::cout << "Has values: user.name=" << reader.HasValue("user", "name")
    //           << ", user.nose=" << reader.HasValue("user", "nose") << "\n";
    // std::cout << "Has sections: user=" << reader.HasSection("user")
    //           << ", fizz=" << reader.HasSection("fizz") << "\n";
    return 0;
}
//g++ INIReaderExample.cpp INIReader.cpp ini.c -o INIReaderExample


