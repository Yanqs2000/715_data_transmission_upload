/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file csi_test.c
 *
 * @brief Example application main file.
 * This application continuously collects the specified frame number image,
 * calculates the time, prints the rate, checks the last frame image data.
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2022-8-15
 **/

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

#include "v4l2_device_control.h"
#include "parameter_parser.h"

volatile bool g_quit = false;//用于表示是否需要退出程序
int sockfd;
struct sockaddr_in server_addr;


//串口初始化函数
int serial_init(const char *port, int baud_rate)
{
    int fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("open serial port");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, baud_rate);
    cfsetospeed(&options, baud_rate);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    tcsetattr(fd, TCSANOW, &options);

    return fd;
}

void init_network_socket(int port)
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
}

void receive_and_forward_data(int serial_fd)
{
    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    while (1) {
        int n = recvfrom(sockfd, (char *)buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&client_addr, &len);
        if (n > 0) {
            write(serial_fd, buffer, n);  // 转发数据到串口
            printf("Forwarded %d bytes to serial\n", n);
        }
    }
}

//接收到 SIGINT 信号（即Ctrl+C）时，设置 g_quit 为 true，表示需要退出程序。
static void sig_handle(int signal)
{
    g_quit = true;
}
//定义了一个名为 check_data 的函数，用于检查帧数据的正确性。该函数接受帧数据指针、宽度和高度作为参数，并返回一个布尔值表示数据的正确性。
bool check_data(const uint8_t *data, uint32_t width, uint32_t height, int frame_cnt)
{
    uint32_t value = 0;
    uint32_t check_value;
    uint32_t error_num = 0;//用于记录错误数量
    int status = 0;//数据是否正确的标志
    int i;

    for (i = 0; i < width * height; i++) 
    {
        check_value = value % 256;
        if (check_value != data[i]) 
        {
            error_num = error_num + 1;
            printf("check_data:%d\n",check_value);
            printf("real_data:%d\n",data[i]);
            status = 1;
        }
        value++;
    }
    printf("check the %d frame and error_num = %d\n",frame_cnt,error_num);
    printf("byte error rate= %.1f%%\n", ((float)error_num / (width * height)) * 100);
    
    if (status == 0) 
    {
        return true;
    } 
    else 
    {
        ////
        printf("check_error");
        g_quit = true;
        return false;
    }
}

int main(int argc, char **argv)
{
    struct _Params params;//params 结构体用于存储参数
    int frame_cnt = 0;//frame_cnt 用于计算帧的数量
    struct timeval time_start, time_end, time_diff;//time_start、time_end 和 time_diff 用于计算程序运行时间等。
    uint32_t cost_time;
    float average_rate;
    //char filenameNas[100];
    char filename[100];
    int file_cnt = 0;
    
    memset(&params, 0, sizeof(params));//使用 memset 函数将 params 结构体的内存空间初始化为零。
    
    //然后调用 parse_parameter 函数解析命令行参数，并将解析结果存储在 params 结构体中。如果解析失败，打印错误信息并退出程序。
    if (parse_parameter(&params, argc, argv) == false) 
    {
        printf("Please try --help to see usage.\n");
        exit(2);
    }

    /* Ctrl+c handler */
    signal(SIGINT, sig_handle);
    
    //然后调用 v4l2_device_open 函数打开摄像头设备，并设置摄像头的参数，包括设备名称、类型、格式、宽度、高度和帧率等。
    V4l2Device *camera = v4l2_device_open(params.device, params.type, params.format,
                                              params.width, params.height, params.fps);

    printf("User Inputs:\n"
            "    Device       : %s\n"
            "    Fomat        : %s\n"
            "    Width        : %d\n"
            "    Height       : %d\n"
            "\n",
            params.device,
            params.format,
            params.width,
            params.height);

    v4l2_device_setup(camera);
    v4l2_device_stream_on(camera);
    //使用 gettimeofday 函数获取当前时间作为程序开始时间
    gettimeofday(&time_start, NULL);
    
    while (!g_quit) 
    {
        /* Get one frame */
        if(frame_cnt % 4096 == 0)
        {
            file_cnt++;
            sprintf(filename, "/mnt/data/frames/frame_%d.bin",file_cnt);
            //sprintf(filenameNas, "/mnt/nas/frame_%d.bin",file_cnt);
            
        }
        if (v4l2_device_get_buffer(camera, 3)) //调用 v4l2_device_get_buffer 函数获取一帧数据。
        {
            ++frame_cnt;
                      
            if (params.number == frame_cnt) //如果 params.number 等于 frame_cnt，则将 g_quit 设置为 true，表示需要退出程序
            {
                g_quit = true;
            }

            printf("\r\033[KAccept Frames: %d", frame_cnt);
            
	          // check_data((uint8_t *)camera->data, params.width, params.height, frame_cnt);
            FILE* file = fopen(filename, "ab");
            if (file == NULL) 
            {
                printf("can not open file\n");
                g_quit = true;
            }
            //fprintf(file, "The %d frames",frame_cnt);
                    
            size_t data_number = fwrite(camera->data, sizeof(unsigned char), params.width * params.height, file);
            if (data_number == 0)
            {
                printf("The %d frames write error\n",frame_cnt);
                g_quit = true;
            }

            fclose(file);
            // FILE* filenas = fopen(filenameNas, "ab");
            // if (filenas == NULL) 
            // {
            //     printf("can not open filenas\n");
            //     g_quit = true;
            // }
            // fprintf(filenas, "The %d frames",frame_cnt);
                    
            // size_t data_numbernas = fwrite(camera->data, sizeof(unsigned char), params.width * params.height, filenas);
            //if (data_number == 0)
            //{
                //printf("The %d frames write error\n",frame_cnt);
                //g_quit = true;
            //}

            //fclose(filenas);
            
            fflush(stdout);

        } 
        else 
        {
            printf("Get frames failed!\n");
            break;
        }


        v4l2_device_put_buffer(camera);
    }
    printf ("\n");

    /* Calculate the acquisition time and average rate */
    gettimeofday(&time_end, NULL);
    timersub(&time_end, &time_start, &time_diff);
    cost_time = time_diff.tv_sec * 1000.0 + time_diff.tv_usec / 1000;
    //average_rate = (params.width / 1024 * params.height / 1024 * frame_cnt / (cost_time / 1000000.0));
    average_rate = (params.width / 1024 * params.height * frame_cnt / 1024 / (cost_time / 1000.0));
    printf("time: %d ms, rate: %g MB/s\n", cost_time, average_rate);

    /* Check the last frame */
    //check_data((uint8_t *)camera->data, params.width, params.height, frame_cnt);

    v4l2_device_stream_off(camera);
    return 0;
}
