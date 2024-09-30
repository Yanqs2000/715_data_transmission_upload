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

#define MAX_FILENAME_LENGTH 50
int file_cnt = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
struct ThreadParams {
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

#define SOURCE_DIR "/mnt/data/frames"
#define DEST_DIR "/mnt/nas/copy"
#define BUFFER_SIZE 1024

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
    while ((bytes = fread(buffer, 1, BUFFER_SIZE, source_file)) > 0) {
        fwrite(buffer, 1, bytes, dest_file);
    }

    fclose(source_file);
    fclose(dest_file);
}

void copy_directory(const char *source_dir, const char *dest_dir) 
{
    DIR *dir = opendir(source_dir);
    if (dir == NULL) 
    {
        perror("Failed to open source directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (entry->d_type == DT_REG) { // If the entry is a regular file
            char source_path[PATH_MAX];
            char dest_path[PATH_MAX];

            snprintf(source_path, sizeof(source_path), "%s/%s", source_dir, entry->d_name);
            snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

            copy_file(source_path, dest_path);
        }
    }

    closedir(dir);
}

static void *Upload_nas(void *arg)
{
    char source_path[MAX_FILENAME_LENGTH];
    char dest_path[MAX_FILENAME_LENGTH];
    struct ThreadParams *thread_params = (struct ThreadParams *)arg;
    int nas_cnt = 2;
    while(nas_cnt < (thread_params->num + 2))
    {
        pthread_mutex_lock(&mutex);
    
        // 等待 file_cnt 变为 2
        printf("nas_cnt = %d\n",nas_cnt);
        printf("file_cnt = %d\n",file_cnt);
        while (nas_cnt > file_cnt)
        {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        printf("thread continue\n");
        sprintf(source_path, "/mnt/data/frames/frame_%d.bin",nas_cnt-1);
        sprintf(dest_path, "/mnt/nas/copy/frame_%d.bin",nas_cnt-1);
        copy_file(source_path, dest_path);
        nas_cnt++;
    }
    
}

int main(int argc, char **argv)
{
    struct _Params params;//params 结构体用于存储参数
    int frame_cnt = 0;//frame_cnt 用于计算帧的数量
    int status = 0;
    struct timeval time_start, time_end, time_diff;//time_start、time_end 和 time_diff 用于计算程序运行时间等。
    uint32_t cost_time;
    float average_rate;
    char filename[MAX_FILENAME_LENGTH];
    pthread_t thread_id;
    struct ThreadParams thread_params;

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
        /* Get one frame */
        if(frame_cnt % 4096 == 0)
        {
            file_cnt++;
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
                      
            if (params.number == frame_cnt) //如果 params.number 等于 frame_cnt，则将 g_quit 设置为 true，表示需要退出程序
            {
                g_quit = true;
            }

            printf("\r\033[KAccept Frames: %d", frame_cnt);
            
	        //check_data((uint8_t *)camera->data, params.width, params.height, frame_cnt);

            fprintf(file, "The %d frames",frame_cnt);
                    
            size_t data_number = fwrite(camera->data, sizeof(unsigned char), params.width * params.height, file);
            if (data_number == 0)
            {
                printf("The %d frames write error\n",frame_cnt);
                g_quit = true;
            }

            fflush(stdout);

        } 
        else 
        {
            printf("Get frames failed!\n");
            break;
        }
	    if(frame_cnt % 4096 == 0)
	    {
            fclose(file);
            //copy_directory(SOURCE_DIR, DEST_DIR);
	    }

        v4l2_device_put_buffer(camera);
    }
    printf ("\n");
    pthread_join(thread_id, NULL);
    /* Calculate the acquisition time and average rate */
    gettimeofday(&time_end, NULL);
    timersub(&time_end, &time_start, &time_diff);
    cost_time = time_diff.tv_sec * 1000.0 + time_diff.tv_usec / 1000;
    //average_rate = (params.width / 1024 * params.height / 1024 * frame_cnt / (cost_time / 1000000.0));
    average_rate = (params.width / 1024 * params.height * frame_cnt / 1024 / (cost_time / 1000.0));
    printf("time: %d ms, rate: %g MB/s\n", cost_time, average_rate);

    /* Check the last frame */
    check_data((uint8_t *)camera->data, params.width, params.height, frame_cnt);

    v4l2_device_stream_off(camera);
    return 0;
err:
    g_quit = true;
    pthread_join(thread_id, NULL);
    v4l2_device_stream_off(camera);
    return status;
}