#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>

#include "ini.h"                               // 需要下载并包含 ini.h 库
#include "v4l2_device_control.h"
#include "parameter_parser.h"
#include "function.h"

#define NETWORK_PORT 8080                      // 设定接收网络数据包的端口
#define TOTAL_BYTES 110                        // 惯导总数110字节
#define HEADER_SIZE 4                          // 帧头4字节
#define FRAME_HEADER {0xAA, 0x55, 0x5A, 0xA5}  // 定义惯导帧头

extern volatile bool g_quit;                   // 用于表示是否需要退出程序
extern volatile sig_atomic_t data_received;    // 新增标志位，表示是否接收到正确的数据包
extern pthread_mutex_t data_received_mutex;    // 保护共享变量的互斥锁
//extern uint8_t start_uart6_packet[32];       // 发送给串口6的数据包，开始发送(用于测试，并未实际使用)
extern uint8_t end_uart6_packet[32];           // 发送给串口6的数据包，结束发送(正在使用)
extern uint8_t initial_command_buffer[32];     // 起始命令数据接收缓冲区
extern char all_filenames[1024][128];          // 1024表示文件名个数，128表示文件名长度

int file_cnt;
int nas_cnt;
int Final_File_Flag;
pthread_mutex_t nas_mutex;//互斥锁
pthread_cond_t cond;//等待线程

extern struct _Params params; // params 结构体用于存储接收数据参数
extern struct ThreadParams nas_thread_params;

//接收到 SIGINT 信号（即Ctrl+C）时，设置 g_quit 为 true，表示需要退出程序。
static void sig_handle(int signal)
{
    g_quit = true;
    pthread_cond_signal(&cond); //发送信号给线程，使得线程结束等待
    Final_File_Flag = 0;
}

int main(int argc, char **argv)
{
    int frame_cnt = 0;//frame_cnt 用于计算帧的数量
    FILE *file = NULL;

    pthread_t network_thread;// 网络子线程
    pthread_t thread_upload_nas;// 上传nas子线程

    struct timeval time_start, time_end, time_diff;//time_start、time_end 和 time_diff 用于计算程序运行时间等。
    uint32_t cost_time;
    float average_rate;

    char output_file[64];
    char file_name_time[64];
    char filename[128];
    char nas_folder_path[64];
    char full_command[128];
    char base_command[64] = "sudo mkdir -p ";

    const char* serial_device_FPGA    = "/dev/ttyS6"; // 串口6(FPGA)设备路径
    const char* serial_device_GuanDao = "/dev/ttyS3"; // 串口3(GuanDao)设备路径

    struct stat output_info;
    struct stat output_nas_info;

    // 定义帧头
    uint8_t frame_header[HEADER_SIZE] = FRAME_HEADER;

    // 配置串口
    int serial_fd_FPGA = USB_init_FPGA(serial_device_FPGA);
    if (serial_fd_FPGA == -1) 
    {
        return -1;
    }
    
    int serial_fd_GuanDao = USB_init_GuanDao(serial_device_GuanDao);
    if (serial_fd_GuanDao == -1) 
    {
        return -1;
    }

    memset(&params, 0, sizeof(params));//使用 memset 函数将 params 结构体的内存空间初始化为零。
    
    // 首先尝试从 config.ini 文件加载参数
    if (!parse_parameter_from_config(&params, "config.ini")) 
    {
        printf("Failed to load parameters from config.ini\n");
        exit(2);
        return -1;
    }
    
    if (params.data_folder != NULL) 
    {
        strncpy(output_file, params.data_folder, sizeof(output_file) - 1);
        output_file[sizeof(output_file) - 1] = '\0';  // 确保字符串以 null 结尾
    }

    if (stat(output_file, &output_info) != 0) 
    {
        mkdir(output_file, 0777);  // Create logs directory with full permissions
    }

    if (params.nas_folder != NULL) 
    {
        strncpy(nas_folder_path, params.nas_folder, sizeof(nas_folder_path) - 1);
        nas_folder_path[sizeof(nas_folder_path) - 1] = '\0';  // 确保字符串以 null 结尾
    }

    //调用 parse_parameter 函数解析命令行参数，并将解析结果存储在 params 结构体中。如果解析失败，打印错误信息并退出程序。
    //if (parse_parameter(&params, argc, argv) == false) 
    //{
        //printf("Please try --help to see usage.\n");
        //exit(2);
    //}

    /* Ctrl+c handler */
    signal(SIGINT, sig_handle);

    printf("initializing...\n");

    // 创建网络套接字以接收数据包
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(NETWORK_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("bind");
        return -1;
    }

    // 创建子线程来监听网络数据包
    if (pthread_create(&network_thread, NULL, network_listener, &sockfd) != 0) 
    {
        perror("pthread_create");
        return -1;
    }

    int file_num = params.number / params.one_file_frames + 1;
    memset(&nas_thread_params, 0, sizeof(nas_thread_params));
    nas_thread_params.num = file_num;

    // 检查文件夹是否存在，并且配置文件中启用nas,F_OK 表示检查文件是否存在
    if (access(nas_folder_path, F_OK) == 0 && params.if_nas == true) 
    {    
        if (pthread_create(&thread_upload_nas, NULL, &Upload_nas, (void *)&nas_thread_params)) 
        {
            perror("pthread create");
            return -1;
        }
    }
    else if (access(nas_folder_path, F_OK) == -1 && params.if_nas == true)
    {
        // 执行挂载命令
        int ret = system("sudo systemctl start camera.service");
        if (ret == -1) 
        {
            perror("Failed to start camera.service");
            return -1;
        }
        sleep(2); // 等待挂载完成

        if (stat(nas_folder_path, &output_nas_info) != 0) 
        {   
            // 使用 sprintf 将基本命令和路径拼接成完整命令
            snprintf(full_command, sizeof(full_command), "%s%s", base_command, nas_folder_path);

            int ret = system(full_command);
            if (ret == -1) 
            {
                perror("Failed to execute mkdir command");
                return -1;
            }
        }

        if (access(nas_folder_path, F_OK) == 0 && params.if_nas == true)
        {
            if (pthread_create(&thread_upload_nas, NULL, &Upload_nas, (void *)&nas_thread_params)) 
            {
                perror("pthread create");
                return -1;
            }
        }
    }
    printf("wait starting\n");

    // 主线程等待子线程接收到正确的数据包
    while (!g_quit) 
    {
        // 检查是否接收到正确的数据包
        pthread_mutex_lock(&data_received_mutex);
        if (data_received) 
        {
            pthread_mutex_unlock(&data_received_mutex);
            printf("Starting image capture...\n");
            break;
        }
        pthread_mutex_unlock(&data_received_mutex);
        usleep(100000); // 避免CPU占用过高
    }

    //然后调用 v4l2_device_open 函数打开摄像头设备，并设置摄像头的参数，包括设备名称、类型、格式、宽度、高度和帧率等。
    V4l2Device *camera = v4l2_device_open(params.device, params.type, params.format,
                                              params.width, params.height, params.fps);

    printf("User Inputs:\n"
            "    Device       : %s\n"
            "    Fomat        : %s\n"
            "    Width        : %d\n"
            "    Height       : %d\n"
            "    frames       : %d\n"
            "\n",
            params.device,
            params.format,
            params.width,
            params.height,
            params.number);

    v4l2_device_setup(camera);
    v4l2_device_stream_on(camera);

    // 向串口发送数据包
    ssize_t bytes_written_start = write(serial_fd_FPGA, initial_command_buffer, sizeof(initial_command_buffer));
    if (bytes_written_start < 0) 
    {
        perror("start data failed to write to serial uart6");
        close(serial_fd_FPGA);
        return -1;
    }
    printf("Start data successfully sent to %s\n",serial_device_FPGA);

    //使用 gettimeofday 函数获取当前时间作为程序开始时间
    gettimeofday(&time_start, NULL);

    add_filename_to_array("This is a filename array", 0);
    while (!g_quit) 
    {
        /* Get one frame */
        if(frame_cnt % params.one_file_frames == 0)
        {
            file_cnt++;
            
            if(nas_cnt <= file_cnt)
            {
                pthread_cond_signal(&cond); //发送信号给线程，使得线程结束等待
                printf("\nsend signal cond\n");
            }

            get_filename_from_time(file_name_time, sizeof(file_name_time)); // 生成文件名

            // 指定文件路径
            snprintf(filename, sizeof(filename), "%s%s", output_file, file_name_time); // 构建完整的文件路径

            printf("%s",filename);
            file = fopen(filename, "ab");
            if (file == NULL) 
            {
                printf("can not open file\n");
                g_quit = true;
            }

            // 将文件名加入到数组
            add_filename_to_array(file_name_time, file_cnt);

            // 将起始命令写入文件
            fwrite(initial_command_buffer, sizeof(uint8_t), sizeof(initial_command_buffer), file);
        }

        if (v4l2_device_get_buffer(camera, 3)) //调用 v4l2_device_get_buffer 函数获取一帧数据。
        {
            ++frame_cnt;
            
            uint8_t date_time[22];
            if (params.if_GuanDao == true)
            {
                // 调用读取函数
                uint8_t* GuanDao_frame = read_process(serial_fd_GuanDao, TOTAL_BYTES, frame_header, HEADER_SIZE);
                if (GuanDao_frame == NULL)
                {
                    memset(date_time, 0, sizeof(date_time));  // 将所有元素设置为0
                }
                else
                {
                    DataExtractor_raw(GuanDao_frame, TOTAL_BYTES, date_time, sizeof(date_time));
                }
            }
                   
            if (params.number == frame_cnt) //如果 params.number 等于 frame_cnt，则将 g_quit 设置为 true，表示需要退出程序
            {
                g_quit = true;
            }

            printf("\r\033[KAccept Frames: %d", frame_cnt);

            size_t data_number = fwrite(camera->data, sizeof(unsigned char), params.width * params.height, file);
            if (data_number == 0)
            {
                printf("The %d frames write error\n",frame_cnt);
                g_quit = true;
            }

            if (params.if_GuanDao == true)
            {
                size_t GuanDao_number = fwrite(date_time, sizeof(uint8_t), sizeof(date_time), file);
                if (GuanDao_number == 0)
                {
                    printf("GuanDao data write error\n");
                    g_quit = true;
                }
            }

            if (params.number == frame_cnt) //如果 params.number 等于 frame_cnt，则将 g_quit 设置为 true，表示需要退出程序
            {
                /*双保险：若最后一帧出现时，子线程已经写完前一个在等待，那么发送信号给子进程不用再等待了*/
                /*若最后一帧出现时，子线程依旧在写前一个文件，那么子线程在下一个循环时，直接跳过等待*/
                Final_File_Flag = 0;
                pthread_cond_signal(&cond); //发送信号给条件变量
                printf("\nsend final signal cond\n");
                g_quit = true;
            }

            fflush(stdout);
        } 
        else 
        {
            printf("Get frames failed!\n");
            break;
        }
        if(frame_cnt % params.one_file_frames == 0 || Final_File_Flag == 0)
	    {
            fclose(file);
	    }

        v4l2_device_put_buffer(camera);
    }
    printf("\n");

    /* Calculate the acquisition time and average rate */
    gettimeofday(&time_end, NULL);
    timersub(&time_end, &time_start, &time_diff);
    cost_time = time_diff.tv_sec * 1000.0 + time_diff.tv_usec / 1000;
    average_rate = (params.width / 1024 * params.height * frame_cnt / 1024 / (cost_time / 1000.0));
    printf("time: %d ms, rate: %g MB/s\n", cost_time, average_rate);

    v4l2_device_stream_off(camera);

    // 向串口发送关闭数据包
    ssize_t bytes_written_end = write(serial_fd_FPGA, end_uart6_packet, sizeof(end_uart6_packet));
    if (bytes_written_end < 0) 
    {
        perror("End data failed to write to serial uart6");
        close(serial_fd_FPGA);
        return -1;
    }
    printf("End data successfully sent to %s\n",serial_device_FPGA);
    
    close(sockfd);
    close(serial_fd_FPGA); // 关闭串口

    pthread_join(thread_upload_nas, NULL);//等待线程结束
    return 0;
}