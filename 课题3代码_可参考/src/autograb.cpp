
#include "autograb.h"

#define grab_log
using namespace std;
bool g_run_loop = true;


int auto_grab(Parameter& params, time_t loop_start_time, int stopgrab_time)
{
    int exitCode = 0;
    //set image width
    const int width = 480;
    //set image height
    const int height = 1200;

    // int stopgrab_time = 10;

    std::vector<double> FrameAndExtime = camera_init(params);

    //Data saving location
    std::string Ch_path2 = "/home/camera/ssd/imgs/";
    // std::string Ch_path2 = "/home/camera/Nas/LzTeam/";
    //Log save location
    std::string uploadlog_dir = "/home/camera/ssd/upload_logdir/";

    std::string root_path = Ch_path2 + GetTimeStamp();
    std::string time_stamp = GetTimeStamp();

    std::string date_str = get_date_string();

    std::string upload_filename = uploadlog_dir + date_str + ".log";

    std::string hdr_filename = root_path + "/" + time_stamp + ".hdr";
    std::string bil_zstd_filename = root_path + "/" + time_stamp + ".bil.zst";
    
    //
    std::ofstream hdr_file;
    std::ofstream upload_file;

    upload_file.open(upload_filename, std::ios::app);
    if(!upload_file.is_open())
    {
        // std::cout<<"upload log file open failed " <<std::endl;
        LOG(ERROR) << "upload log file open failed ";
        return -1;
    }

    // Create output parent's dir if not exists.
    if (std::filesystem::exists(root_path))
    {
        if (std::filesystem::is_regular_file(root_path))
        {
            // std::cout << "The path is a file, not a directory." << std::endl;
            LOG(ERROR) << "The path is a file, not a directory.";
            return 1;
        }
    }
    else
    {
        std::filesystem::create_directory(root_path);
    }

    // save ins time
    // int  sync_flag = params.INS_flag;
    if(params.INS_flag)
    {
     int sit = save_inctime(root_path);
     if(sit == -1)
     {
        LOG(ERROR) << "save inctime failed";
     }
    }
    int img_nums = 0;
    // Before using any pylon methods, the pylon runtime must be initialized.
    PylonInitialize();

    try
    {
        // Create an instant camera object with the first found camera device.
        Pylon::CBaslerUniversalInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
        
        // Print the model name of the camera.
        std::cout << "Using device: " << camera.GetDeviceInfo().GetModelName() << std::endl;
        LOG(INFO) << "Using device: " << camera.GetDeviceInfo().GetModelName();

        camera_set(camera, FrameAndExtime[0], FrameAndExtime[1]);


        // Start the grabbing of c_countOfImagesToGrab images.
        // The camera device is parameterized with a default configuration which
        // sets up free-running continuous acquisition.
        // std::string start_time = GetTimeStamp();
        time_t start_time;
        time(&start_time);
        camera.StartGrabbing();

        // This smart pointer will receive the grab result data.
        CGrabResultPtr ptrGrabResult;

        //从内存中压缩
        std::ofstream outFileStream;
        ZSTD_CCtx* cctx = initCompression(bil_zstd_filename, outFileStream);
        if(cctx == nullptr)
        {
            // std::cout<<"Failed to initialize compression"<<std::endl;
            LOG(ERROR) << "Failed to initialize compression";
            return 2;
        }

        //Save image size
        size_t imageSize;
        //get image
        uint16_t *pImageBuffer;
        
        //Whether to directly compress the original data
        bool if_raw_compress = true;
        if(if_raw_compress)
        {
            // std::cout<<"-------Direct data raw compression------"<<std::endl;
            LOG(INFO) << "-------Direct data raw compression------";
        }else{
            // std::cout<<"-----Merge the data into one and then compress it----"<<std::endl;
            LOG(INFO) << "-----Merge the data into one and then compress it----";

        }

        while (camera.IsGrabbing())
        {
            // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
            camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);


            time_t grab_end_time;
            time(&grab_end_time);
            if((grab_end_time - loop_start_time) > stopgrab_time)
            {
                //结束
                camera.StopGrabbing();
                // std::cout<<"Stop Grabbing"<<std::endl;
                LOG(INFO) << "Stop Grabbing";
            }
            // Image grabbed successfully?
            if (ptrGrabResult->GrabSucceeded())
            {
                // Access the image data.
                pImageBuffer = (uint16_t *)ptrGrabResult->GetBuffer();

                // //存入图片尺寸
                if(img_nums < 1)
                {
                    imageSize = ptrGrabResult->GetImageSize();
                }
                    
                //流压缩
                if(!Bil2Compress(cctx, outFileStream, pImageBuffer, imageSize, width, height, if_raw_compress))
                {
                    std::cout<<"auto_grab.cpp: Compression failed"<<std::endl;
                    LOG(ERROR) << "auto_grab.cpp: Compression failed";
                    return 3;
                }
                
                img_nums++;

#ifdef PYLON_WIN_BUILD
                // Display the grabbed image.
                Pylon::DisplayImage(1, ptrGrabResult);
#endif
            }
            else
            {
                // std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
                LOG(ERROR) << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription();
            }
        }
        
        
        //流压缩结束，
        Bil2Compress_finish(cctx, outFileStream, pImageBuffer, imageSize, width, height, if_raw_compress);
        
        //结束
        camera.StopGrabbing();
        double ResultFPS = camera.ResultingFrameRate.GetValue();
        time_t end_time;
        time(&end_time);

        //Save directory name
        upload_file << time_stamp << std::endl;

        std::cout << "grab_time: " << end_time - start_time <<" s"<<std:: endl;

        CreatehdrFile(hdr_file, hdr_filename, 1200, img_nums);
        
        std::cout << "Success to Compress BIL file: "<< time_stamp << std::endl;
        std::cout << "Frames: " << img_nums << std::endl;
        std::cout << "WriteSpeed: " << img_nums / (end_time - start_time) << " Frames/second" << std::endl;
        std::cout << "FPS: " << ResultFPS << std::endl;
        LOG(INFO) << "Success to Compress BIL file: "<< time_stamp;
        LOG(INFO) << "Frames: " << img_nums;
        LOG(INFO) << "WriteSpeed: " << img_nums / (end_time - start_time) << " Frames/second";
        LOG(INFO) << "FPS: " << ResultFPS;

    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        LOG(ERROR) << e.what();
    }

    std::cerr << endl
    //           << "Press enter to exit." << endl;
    // while (std::cin.get() != '\n')
        ;
    // Releases all pylon resources.
    PylonTerminate();

    std::cout<<"-----Division line-------" << std::endl;
    LOG(INFO) << "-----Division line-------";
    upload_file.close();
    return exitCode;
}

int save_inctime(std::string root_path)
{
    std::vector<uint8_t> data;
    std::string time_str;

    time_t start_time;
    time(&start_time);

    int usb_serial = USB_init("/dev/ttyUSB0");
    if(usb_serial < 0)
    {
        std::cout<<"usb init failed"<<std::endl;
        return -1;
    }
    while(true)
    {
        std::vector<std::uint32_t> dateTime;
        time_t end_time;
        time(&end_time);
        data = read_process(usb_serial, 110);
        //数据提取
        int dex = DataExtractor_raw(data, dateTime);
        //终端同步时间
        if(dex > 0 )
        {
            int get_instr = get_inctimestr_raw(dateTime, time_str);
            if(get_instr < 0)
            {
                continue;
            }
            //std::cout<<"get_inctimestr successful: " <<std::endl;
	    LOG(INFO)<<"get_INStime: "<< time_str;
            break;
        }
        if(end_time - start_time > 2)
        {
            LOG(ERROR) << "get_INStime failed";
            //关闭串口
            close(usb_serial);
            return -1;
        }
    }
    std::string ins_time = root_path + "/" + time_str + ".log";
    std::ofstream ins_time_file;
    ins_time_file.open(ins_time, std::ios::app);
    if(!ins_time_file.is_open())
    {
        std::cout<<"ins_time_file file open failed " <<std::endl;
        return -1;
    }
    ins_time_file << time_str << std::endl;
    ins_time_file.close();
    //关闭串口
    close(usb_serial);
    return 1;
}

int save_inctime_raw(std::string root_path)
{
    std::vector<uint8_t> data;
    std::string time_str;

    time_t start_time;
    time(&start_time);

    int usb_serial = USB_init("/dev/ttyUSB0");
    if(usb_serial < 0)
    {
        std::cout<<"usb init failed"<<std::endl;
        return -1;
    }
    while(true)
    {
        std::vector<std::uint32_t> dateTime;
        time_t end_time;
        time(&end_time);
        data = read_process(usb_serial, 110);
        //数据提取
        int dex = DataExtractor_raw(data, dateTime);
        //终端同步时间
        if(dex > 0 )
        {
            int get_instr = get_inctimestr_raw(dateTime, time_str);
            if(get_instr < 0)
            {
                continue;
            }
            std::cout<<"get_inctimestr successful" <<std::endl;
            break;
        }
        if(end_time - start_time > 2)
        {
            LOG(INFO) << "get_inctimestr successful";
            //关闭串口
            close(usb_serial);
            return -1;
        }
    }
    std::string ins_time = root_path + "/" + time_str + ".log";
    std::ofstream ins_time_file;
    ins_time_file.open(ins_time, std::ios::app);
    if(!ins_time_file.is_open())
    {
        std::cout<<"ins_time_file file open failed " <<std::endl;
        return -1;
    }
    ins_time_file << time_str << std::endl;
    ins_time_file.close();
    //关闭串口
    close(usb_serial);
    return 1;
}
std::string GetTimeStamp()
{
    // Get current time.
    struct tm t;
    time_t now;
    time(&now);
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&t, &now);
#else
    localtime_r(&now, &t);
#endif
    char cNowTime[64];
    strftime(cNowTime, sizeof(cNowTime), "%Y%m%d-%H%M%S", &t);
    std::string stime(cNowTime);

    return stime;
}


//获取时间戳
std::string get_date_string()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm bt{};
#ifdef _MSC_VER
    localtime_s(&bt, &in_time_t);
#else
    localtime_r(&in_time_t, &bt); // POSIX
#endif

    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%d"); // 格式为年月日
    return oss.str();
}

void signalHandler(int signum)
{
    std::cout << "Stop Grabbing!" << std::endl;
    g_run_loop = false;
}

