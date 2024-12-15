#include <bits/stdc++.h>

// #ifdef PYLON_WIN_BUILD
// #include <windows.h>
// #include <pylon/PylonGUI.h>
// #else
// #include <unistd.h>
// #endif

#include <fcntl.h>
#include <zstd.h>   //
#include "common.h"
#include "stream_compress.h"  //
#include "autograb.h"
#include "thread_queue.h"
#include "Thread_ReadUSB.h"
#include "DataExtractor.h"
#include "Sync.h"
#include "config.h"
#include "gloghelper.h"

#define version 20241021  //版本号 威海调试修改 

int grab(Parameter& params)
{
    // int grab_count = 144;
    int grab_count = params.grab_count;
    int current_count = 1; 
    // int stopgrab_time = 600;
    int stopgrab_time = params.grab_time;
    while(grab_count > 0)
    {
        time_t start_time;
        time(&start_time);
        std::cout<<"_____it's "<<current_count << " shooting data____"<< std::endl;
        LOG(INFO) << "_____it's " << current_count << " shooting data____";
        auto_grab(params, start_time, stopgrab_time);

        grab_count --;
        current_count ++; 
        
        std::cout<< std::endl;
        //Shoot after a few seconds of delay
        sleep(3);
    }

    return 1;
}



int main(int argc, char *argv[])
{
    //读取配置文件
    Parameter params;
    const char* config_file = "/home/camera/Topic3_Project/Auto_GCSU_20241022/config.ini";
    read_config(params, config_file);

    //初始化日志
    GlogHelper gh(argv[0], params);
    LOG(INFO) << "_______current version: " << version;
    LOG(INFO) << "_______Function: Shoot, compress, upload, sync_______ ";

    std::cout<< "_______current version: " << version << std::endl;
    std::cout<< "_______Function: Shoot, compress, upload, sync_______ " << std::endl;

    // save data
    // std::vector<std::string> data_time;
    std::vector<uint8_t> data;

    if(params.INS_flag)
    { 
		//读取串口
		int usb_serial = USB_init("/dev/ttyUSB0");
		if(usb_serial < 0)
		{
			// std::cout<<"usb init failed"<<std::endl;
			LOG(ERROR) << "usb init failed";
			return -1;
		}else{
		LOG(INFO) << "_____usb init success" << std::endl;
		}
		time_t start_time;
		time(&start_time);
		if(params.INS_flag == 1)
		{
			LOG(INFO) << "read INS: On";
		}else{
			LOG(INFO) << "read INF: off";
		}
		if(params.if_Sync)
		{
			LOG(INFO)<<"Sync On";
		}else{
			LOG(INFO) << "Sync off";
		}
		while(params.if_Sync)
		{
				time_t end_time;
				time(&end_time);
				std::vector<std::uint32_t> datatime;
				//是否开启同步
				if(params.if_Sync == 0)
				{
					LOG(INFO)<<"Sync OFF";
					break;
				}

				data = read_process(usb_serial, 110);

					//数据提取
				// int dex = DataExtractor(data, std::ref(data_time));
				int dex = DataExtractor_raw(data, std::ref(datatime));

				//终端同步时间
				if(dex > 0)
				{
				
					// int sync =  sync_run(data_time);
					int sync = sync_run_raw(datatime);
					if(sync > 0)
					{
						// std::cout<<"sync time success"<<std::endl;
							LOG(INFO) << "sync time success";
						break;
					}else{
						sleep(10);
					}

				}
			
				if(end_time - start_time > params.Sync_time)  //如果超过设置时间没有同步成功，则退出
				{
					// std::cout<<"sync time failed"<<std::endl;
					LOG(ERROR) << "overtime: sync time failed";
					break;
				}
		}
			//关闭串口
		close(usb_serial);
    }
    //启动拍摄.
    grab(params);

    return 0;   
}


