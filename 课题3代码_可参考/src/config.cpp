#include "config.h"
#include "gloghelper.h"
int read_config(Parameter &params, const char *filename)
{
    mINI::INIFile confg_ini(filename);
    mINI::INIStructure ini;
    confg_ini.read(ini);
    if(ini.size() == 0)
    {
        // std::cout<<"read config.ini failed"<<std::endl;
        LOG(ERROR)<<"read config.ini failed"<<std::endl;
        return -1;
    }

    //camera
    params.FPS = std::stoi(ini["params"].get("FPS"));
    params.EXTIME = std::stoi(ini["params"].get("EXTIME"));
    params.grab_count = std::stoi(ini["params"].get("grab_count"));
    params.grab_time = std::stoi(ini["params"].get("grab_time"));
    params.Sync_time = std::stoi(ini["params"].get("Sync_time"));

    //惯导系统
    params.if_Sync = std::stoi(ini["params"].get("if_Sync"));
    params.INS_flag = std::stoi(ini["params"].get("INS_flag"));

    //glog
    params.glog_dir = ini["glog"].get("glog_dir");
    return 0;
}
