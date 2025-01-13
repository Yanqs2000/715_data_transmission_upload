/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file main.c
 *
 * @brief Realize AD data capture.
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2023-4-24
 **/

#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "ad76x6.h"
#include "ring_buffer.h"

#define CHANNEL_NUM         8   //AD采集通道数
#define BYTE_PER_SAMPLE     2   //每个采样点字节数（高频采样AD是2字节，低频采样AD是3字节）

/* Ring buffer num */
#define RING_BUFFER_NUM     400 //环形缓冲区所能容纳数量

const char * const VERSION = "1.0";

char interrupt_event[32];

//命令行参数解析（v4l2和spi任选其一）
struct CmdLineParams {  
    char        dev_name[32]; //设备名称
    uint32_t    interface;    //接口名称
    uint32_t    sample_num;   //采样点
    struct V4lParams v4l2_params; //v4l2参数：类型、格式、分辨率、帧率
    struct SpiParams spi_params;  //spi参数：类型、速率、模式、数据位、延迟
};

//线程参数：大小、周期、缓冲区、接口类型
struct ThreadParams {
    uint32_t size;
    uint32_t cycle;
    RingBuffer *ring_buf;
    enum InterfaceType iface;
};

sem_t sem_syn; //是一个用于声明POSIX信号量的语句。它是无名信号量（semaphore），是一种只能用于线程间访问的同步机制。

/* Exit flag */
volatile bool g_quit = false;

struct RingBuffer ring_buffer;

/* Short option names */
static const char g_shortopts [] = ":d:e:w:h:i:s:m:S:b:n:D:v"; //解析短参数

/* Option names */
static const struct option g_longopts [] = {
    { "device",     required_argument,      NULL,       'd' },
    { "width",      required_argument,      NULL,       'w' },
    { "height",     required_argument,      NULL,       'h' },
    { "interface",  required_argument,      NULL,       'i' },
    { "size",       required_argument,      NULL,       's' },
    { "mode",       required_argument,      NULL,       'm' },
    { "speed",      required_argument,      NULL,       'S' },
    { "bits",       required_argument,      NULL,       'b' },
    { "delay",      required_argument,      NULL,       'D' },
    { "number",     required_argument,      NULL,       'n' },
    { "event",      no_argument,            NULL,       'e' },
    { "version",    no_argument,            NULL,       'v' },
    { "help",       no_argument,            NULL,        0  },
    { 0, 0, 0, 0 }
};//解析长参数

static void usage(char **argv) //程序所需的参数和用法usage
{
    fprintf(stdout,
            "Usage: %s [options]\n\n"
            "Options:\n"
            " -d | --device         V4l2 or spi device. \n"
            " -e | --event          interrupt event. \n"
            " -w | --width          V4L2 device capture width. \n"
            " -h | --height         V4L2 device capture height. \n"
            " -i | --interface      Interface type. [csi, spi]\n"
            " -s | --size           Data size of one transmission by spi.\n"
            " -m | --mode           Spi mode. [0, 1, 2, 3]\n"
            " -S | --speed          Spi speed.\n"
            " -b | --bits           Bits per word.\n"
            " -D | --delay          Delay (usec).\n"
            " -n | --number         Number of sample points per channel.\n"
            " -v | --version        Display version information.\n"
            " -h | --help           Show help content.\n\n"
            "Example:\n"
            "  # ./%s -i csi -d /dev/video0 -w 512 -h 512 -n 16384\n"
            "  # ./%s -i spi -d /dev/spidev0.1 -e /dev/input/event2 -S 50000000 -m 3 -b 8 -s 16384 -n 1024\n"
            "", basename(argv[0]),
                basename(argv[0]),
                basename(argv[0]));
}

static bool parse_parameter(struct CmdLineParams *params, int argc, char **argv) {
    int opt;
    memset(params, 0, sizeof(struct CmdLineParams));

    /* Set default value */
    memset(params->v4l2_params.type, 0, sizeof(params->v4l2_params.type));
    strcpy(params->v4l2_params.format, "BGGR");
    params->v4l2_params.width    = 0;
    params->v4l2_params.height   = 0;
    params->v4l2_params.fps      = 0;

    memset(params->dev_name, 0, sizeof(params->dev_name));
    params->interface   = IF_INVALID;
    params->sample_num  = 0;

    /* Data size of one transmission by spi */
    params->spi_params.size          = 0;
    params->spi_params.speed         = 0;
    params->spi_params.bits_per_word = 0;
    params->spi_params.mode          = -1;
    params->spi_params.delay         = 0;

    while ((opt = getopt_long(argc, argv, g_shortopts, g_longopts, NULL)) != -1) {
        switch (opt) {
        case 'd':
            strcpy(params->dev_name, optarg);
            break;

        case 'w':
            params->v4l2_params.width = atoi(optarg); //将字符串转换为整数
            break;

        case 'h':
            params->v4l2_params.height = atoi(optarg);
            break;

        case 'i':
            if (strcmp(optarg, "csi") == 0) {
                params->interface = IF_CSI;
            } else if (strcmp(optarg, "spi") == 0) {
                params->interface = IF_SPI;
            }
            break;

        case 's':
            params->spi_params.size = atoi(optarg);
            break;

        case 'S':
            params->spi_params.speed = atoi(optarg);
            break;

        case 'b':
            params->spi_params.bits_per_word = atoi(optarg);
            break;

        case 'D':
            params->spi_params.delay = atoi(optarg);
            break;

        case 'm':
            switch (atoi(optarg)) {
            case 0:
                params->spi_params.mode = SPI_MODE_0;
                break;
            case 1:
                params->spi_params.mode = SPI_MODE_1;
                break;
            case 2:
                params->spi_params.mode = SPI_MODE_2;
                break;
            case 3:
                params->spi_params.mode = SPI_MODE_3;
                break;
            }
            break;

        case 'n':
            params->sample_num = atoi(optarg);
            break;

        case 'e':
            strcpy(interrupt_event, optarg);
            break;

        /* --help */
        case 0:
            usage(argv);
            exit(0);

        case 'v':
            printf("version : %s\n", VERSION);
            exit(0);

        default:
            fprintf(stderr, "Unknown option %c\n", optopt);
            break;
        }
    }

    if (strlen(params->dev_name) == 0 || params->interface == IF_INVALID
        || params->sample_num == 0) {
        return false;
    }

    if (params->interface == IF_CSI) {
        if (params->v4l2_params.width == 0 || params->v4l2_params.height == 0) {
            return false;
        }
    } else if (params->interface == IF_SPI) {
        if (params->spi_params.size == 0 || params->spi_params.speed == 0
            || params->spi_params.mode == -1 || params->spi_params.bits_per_word == 0) {
            return false;
        }
    }

    return true;
}

/**
 * Thread handle function, use to save data
 */
static void *save_data(void *arg)
{
    FILE *fp = NULL;
    struct ThreadParams *thread_params = (struct ThreadParams *)arg;
    int16_t *data;
    int exit_val = 0;
    bool ret;
    int i, cycle = 0;
    struct timespec ts;

    /* Open csv file */
    fp = fopen("data.csv", "w+");
    if (fp == NULL) {
        perror("Open file error\n");
        exit_val = 1;
        goto err;
    }

    data = (int16_t *)malloc(thread_params->size);
    if (!data) {
        perror("Malloc error");
        exit_val = 1;
        goto err;
    }

    while(!g_quit) {
        if (cycle == thread_params->cycle) break;

        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;

        /* Get buffer from ring buffer */
        ret = ring_buffer_get(thread_params->ring_buf, (void *)data);
        if (!ret) {
            sem_timedwait(&sem_syn, &ts);//等待信号量（1秒），超时则继续下个循环

            continue;
        }

        /* Save data to csv */
        for (i = 0; i < (thread_params->size) / 2; i++) {
            fprintf(fp, "%f,", (float)data[i] * 10 / 65535);

            /* Each frame contain data for 8 channels */
            if ((i + 1) % CHANNEL_NUM == 0) {
                fprintf(fp, "\n");
            }
        }

        cycle++;
    }

err:
    fclose(fp);

    pthread_exit((void *)&exit_val);
}

/**
 * Signal handle
 */
void sig_handle(int arg)
{
    g_quit = true;
}

int main(int argc, char *argv[])
{
    struct CmdLineParams params; //定义,命令行参数获取
    Ad76x6 *ad76x6 = NULL; //定义，AD76x6对象为空
    int i = 0;//帧数计数
    int status = 0; //标志位，表示线程创建是否成功
    int cycle = 0; //采样循环次数
    bool ret; //标志位，表示在主线程，是否将数据写入缓冲区
    pthread_t thread_id;  //定义一个线程
    struct ThreadParams thread_params; //线程参数：大小、周期、ring buffer、接口
    RingBuffer *ring_buf = NULL; //缓冲区定义：头、尾、数量、大小、数据
    int ring_buf_size = 0;
    struct Ad76x6Params ad76x6_params;

    /* Parse command line */
    //解析参数
    if (!parse_parameter(&params, argc, argv)) 
    {
        printf("Please try --help to see usage.\n");
        exit(1);
    }

    /* Ctrl+c handler */
    signal(SIGINT, sig_handle);

    if (params.interface == IF_CSI) 
    {
        //计算循环次数=采样点数/单通道一帧采样点的数量
        //单通道一帧采样点的数量=分辨率（宽度*高度）/每个采样点的字节数/通道数
        cycle = params.sample_num / (params.v4l2_params.width * params.v4l2_params.height
                 / BYTE_PER_SAMPLE / CHANNEL_NUM);
        //每个缓冲区的大小，等于宽度*高度（单位：字节数）
        ring_buf_size = params.v4l2_params.width * params.v4l2_params.height;
        //复制参数给AD的参数结构体
        memcpy(&ad76x6_params.sv, &params.v4l2_params, sizeof(struct V4lParams));
    } 
    else if (params.interface == IF_SPI) 
    {
        cycle = params.sample_num / (params.spi_params.size / BYTE_PER_SAMPLE / CHANNEL_NUM);
        ring_buf_size = params.spi_params.size;

        memcpy(&ad76x6_params.sv, &params.spi_params, sizeof(struct SpiParams));
    }

    /* Init ring buffer */
    ring_buf = ring_buffer_init(ring_buf_size, RING_BUFFER_NUM);
    if (!ring_buf) 
    {
        printf("Ring buffer init failed!\n");
        goto err;
    }

    sem_init(&sem_syn, 0, 0); //用于线程同步，第一个0表示进程内共享（同一个进程中的多个线程使用），非0值表示进程间共享
                              //第二个0表示初始值，初始计数值为0（sem_wait，若信号量为0则等待）
    /* Create thread */
    memset(&thread_params, 0, sizeof(thread_params));
    thread_params.size      = ring_buf_size;
    thread_params.iface     = params.interface;
    thread_params.ring_buf  = ring_buf;
    thread_params.cycle     = cycle;
    status = pthread_create(&thread_id, NULL, save_data, (void *)&thread_params); //创建一个线程
    if(status != 0) 
    {
        perror("pthread create");
        goto err;
    }

    /* Init csi or spi */
    ad76x6 = ad76x6_create(params.dev_name, params.interface, &ad76x6_params);
    if (!ad76x6) 
    {
        printf("ad76x6 create failed!\n");
        status = -1;
        goto err;
    }

    if (params.interface == IF_CSI) 
    {
        /* AD capture */
        status = ad76x6_capture_start(ad76x6);
        if (status == -1) 
        {
            printf("ad76x6 capture error\n");
            goto err;
        }

        while (!g_quit) 
        {
            if (g_quit) break;
            if (i == cycle) break;

            /* Get one frame */
            if (!ad76x6_get_buffer(ad76x6)) 
            {
                continue;
            }
            //抓取一帧数据，放入环形缓冲区
            ret = ring_buffer_put(ring_buf, (void *)ad76x6->data);
            if (ret) 
            {
                sem_post(&sem_syn); //用于线程之间的同步，sem_post增加信号量的值，并唤醒等待的线程（sem_wait）——释放信号量。
                i++;
            } 
            else 
            {
                printf("ring buffer is full\n");
            }

            ad76x6_put_buffer(ad76x6);
        }

        ad76x6_capture_stop(ad76x6);
    } 
    else if (params.interface == IF_SPI) {
        while (!g_quit) {
            if (g_quit) break;
            if (i == cycle) break;

            if (!ad76x6_get_buffer(ad76x6)) {
                continue;
            }

            ret = ring_buffer_put(ring_buf, (void *)ad76x6->data);
            if (ret) {
                sem_post(&sem_syn);
                i++;
            } else {
                printf("ring buffer is full\n");
            }
        }
    }

    printf("save data file success. (sample: %d, channel: %d)\n", params.sample_num, CHANNEL_NUM);
    pthread_join(thread_id, NULL);
err:
    g_quit = true;
    sem_post(&sem_syn);

    pthread_join(thread_id, NULL);

    ad76x6_destroy(ad76x6);
    ring_buffer_destory(ring_buf);

    return status;
}
