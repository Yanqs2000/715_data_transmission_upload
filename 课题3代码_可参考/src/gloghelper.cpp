
#include "gloghelper.h"


void SignalHandler(const char* data, int size)
{
    std::string str = std::string(data, size);
    LOG(ERROR) << "SignalHandler: " << str;
}

bool create_directory_with_permissions(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        std::cout << "Directory does not exist. Creating directory: " << path << std::endl;
        if (std::filesystem::create_directories(path)) {
            std::cout << "Directory created successfully." << std::endl;
            // 设置目录权限，例如 0755
            chmod(path.c_str(), 0755);
        } else {
            std::cerr << "Failed to create directory: " << path << std::endl;
            return false;
        }
    }
    return true;
}

GlogHelper::GlogHelper(char* program, Parameter& param)
{
    std::string glogdir = param.glog_dir;
    // Create output parent's dir if not exists.
    if (!create_directory_with_permissions(glogdir)) {
        std::cerr << "Failed to set up log directory. Exiting." << std::endl;
        exit(EXIT_FAILURE);
    }

    google::InitGoogleLogging(program);
    google::SetStderrLogging(google::ERROR);//设置级别高于等于 google::WARNING 的日志同时输出到屏幕
    FLAGS_colorlogtostderr = true; //设置输出到屏幕的日志显示相应颜色
    
    std::string infofile = glogdir + "/info_";
    std::string waringfile = glogdir + "/waring_";
    std::string errorfile = glogdir + "/error_";
    google::SetLogDestination(google::GLOG_INFO, infofile.c_str());//设置 google::INFO 级别的日志存储路径和文件名前缀
    google::SetLogDestination(google::WARNING, waringfile.c_str());   //设置 google::WARNING 级别的日志存储路径和文件名前缀
    google::SetLogDestination(google::ERROR,errorfile.c_str());   //设置 google::ERROR 级别的日志存储路径和文件名前缀

    FLAGS_logbufsecs =0;        //缓冲日志输出，默认为30秒，此处改为立即输出
    FLAGS_max_log_size =100;  //最大日志大小为 100MB

    FLAGS_stop_logging_if_full_disk = true;     //当磁盘被写满时，停止日志输出
    //google::SetLogFilenameExtension("91_");     //设置文件名扩展，如平台？或其它需要区分的信息
    google::InstallFailureSignalHandler();      //捕捉 core dumped
    google::InstallFailureWriter(&SignalHandler);    //默认捕捉 SIGSEGV 信号信息输出会输出到 stderr，可以通过下面的方法自定义输出>方式：
}

//GLOG内存清理：
GlogHelper::~GlogHelper()
{
    google::ShutdownGoogleLogging();
}
