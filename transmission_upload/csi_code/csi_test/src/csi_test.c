/*目前在ubuntu20.04、Linux5.10系统中，数据接收的速率为18MB/s,数据通过网络上传到nas的速率在30-40MB/s之间*/
/*只有当上传速率大于接收速率时，逻辑才成立；当上传速率小于接收速率时，主线程会不断发信号，不知道对子线程有没有影响*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#include "v4l2_device_control.h"
#include "parameter_parser.h"

//宏定义
#define MAX_FILENAME_LENGTH 50
#define BUFFER_SIZE 1024

//全局变量
int file_cnt = 0;
int nas_cnt = 2;
int Final_File_Flag = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//互斥锁
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;//等待线程
struct ThreadParams 
{
    int num;
};

volatile bool g_quit = false;//用于表示是否需要退出程序

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

void copy_file(const char *source_path, const char *dest_path) 
{
    FILE *source_file = fopen(source_path, "rb");
    if (source_file == NULL) 
    {
        perror("Failed to open source file");
        exit(EXIT_FAILURE);
    }

    FILE *dest_file = fopen(dest_path, "wb");
    if (dest_file == NULL) 
    {
        perror("Failed to open destination file");
        fclose(source_file);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, BUFFER_SIZE, source_file)) > 0) 
    {
        fwrite(buffer, 1, bytes, dest_file);
    }

    fclose(source_file);
    fclose(dest_file);
}

static void *Upload_nas(void *arg)
{
    char source_path[MAX_FILENAME_LENGTH];
    char dest_path[MAX_FILENAME_LENGTH];
    struct ThreadParams *thread_params = (struct ThreadParams *)arg;
    struct timeval time_start_p, time_end_p, time_diff_p;//time_start_p、time_end_p和time_diff_p用于计算子线程程序运行时间。
    uint32_t cost_time_p;
    while(nas_cnt < (thread_params->num + 2))
    {
        pthread_mutex_lock(&mutex);
    
        // 等待 file_cnt
        if (nas_cnt > file_cnt && Final_File_Flag)//？？？？
        {
            pthread_cond_wait(&cond, &mutex);
        }
        
        printf("sleep 1 second");
        sleep(1);//单位：秒

        pthread_mutex_unlock(&mutex);

        printf("child thread continue\n");

        gettimeofday(&time_start_p, NULL);

        sprintf(source_path, "/mnt/data/frames/frame_%d.bin",nas_cnt-1);
        sprintf(dest_path, "/mnt/nas/copy/frame_%d.bin",nas_cnt-1);

        copy_file(source_path, dest_path);
        
        gettimeofday(&time_end_p, NULL);
        timersub(&time_end_p, &time_start_p, &time_diff_p);
        cost_time_p = time_diff_p.tv_sec * 1000.0 + time_diff_p.tv_usec / 1000;

        nas_cnt++;
        printf("child thread time: %d ms\n", cost_time_p);
    }
}

int main(int argc, char **argv)
{
    struct _Params params;//params 结构体用于存储参数（设备、类型、格式、帧数等）
    int frame_cnt = 0;//frame_cnt 用于计算帧的数量
    int status = 0; //线程创建成功、失败标志
    struct timeval time_start, time_end, time_diff;//time_start、time_end 和 time_diff 用于计算程序运行时间等。
    uint32_t cost_time; //花费时间
    float average_rate; //数据传输速率
    char filename[MAX_FILENAME_LENGTH];
    pthread_t thread_id;
    struct ThreadParams thread_params;

    memset(&params, 0, sizeof(params));//使用memset函数将params结构体的内存空间初始化为零。
    
    //然后调用parse_parameter函数解析命令行参数，并将解析结果存储在params结构体中。如果解析失败，打印错误信息并退出程序。
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

    int file_num = params.number / 4096 + 1;
    memset(&thread_params, 0, sizeof(thread_params));
    thread_params.num = file_num;
    status = pthread_create(&thread_id, NULL, &Upload_nas, (void *)&thread_params);
    if(status != 0) 
    {
        perror("pthread create");
        goto err;
    }

    FILE* file = NULL;
    while (!g_quit) 
    {
        if(frame_cnt % 4096 == 0) //创建二进制文件
        {
            file_cnt++;
            if(nas_cnt <= file_cnt)//？？？改成等号是不是更符合逻辑
            {
                pthread_cond_signal(&cond); //发送信号给线程，使得线程结束等待
                printf("send signal cond\n");
            }
            sprintf(filename, "/mnt/data/frames/frame_%d.bin",file_cnt);
            file = fopen(filename, "ab");
            if (file == NULL) 
            {
                printf("can not open file\n");
                g_quit = true;
            }
        }
        if (v4l2_device_get_buffer(camera, 3)) //调用 v4l2_device_get_buffer 函数获取一帧数据。
        {
            ++frame_cnt;
                    
            printf("\r\033[KAccept Frames: %d", frame_cnt);
            
            //check_data((uint8_t *)camera->data, params.width, params.height, frame_cnt);

            //fprintf(file, "The %d frames",frame_cnt);
                    
            size_t data_number = fwrite(camera->data, sizeof(unsigned char), params.width * params.height, file);
            if (data_number == 0)
            {
                printf("The %d frames write error\n",frame_cnt);
                g_quit = true;
            }

            if (params.number == frame_cnt) //如果 params.number 等于 frame_cnt，则将 g_quit 设置为 true，表示需要退出程序
            {
                /*双保险：若最后一帧出现时，子线程已经写完前一个在等待，那么发送信号给子进程不用再等待了*/
                /*若最后一帧出现时，子线程依旧在写前一个文件，那么子线程在下一个循环时，直接跳过等待*/
                Final_File_Flag = 0;
                pthread_cond_signal(&cond); //发送信号给条件变量
                printf("send final signal cond\n");
                g_quit = true;
            }

            fflush(stdout);

        } 
        else 
        {
            printf("Get frames failed!\n");
            break;
        }
	    if(frame_cnt % 4096 == 0 || Final_File_Flag == 0)
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

    /* Check the last frame */
    //check_data((uint8_t *)camera->data, params.width, params.height, frame_cnt);

    v4l2_device_stream_off(camera);

    pthread_join(thread_id, NULL);//等待线程结束

    return 0;
err:
    v4l2_device_stream_off(camera);
    pthread_join(thread_id, NULL);
    
    g_quit = true;
    return status;
}
