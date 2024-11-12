#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

#define MAXLINE 1044 // 数据包大小
#define EPOCH 20000

#define IP "192.168.1.104" // 服务器端IP地址

// 定义tPing结构体
typedef struct 
{
    uint16_t pack_hdr;    // 数据包头
    uint32_t pack_cnt;    // TP分包计数
    uint32_t pack_sum;    // TP总包数
    uint16_t ping_hdr;    // 发射周期头
    uint32_t ping_cnt;    // 周期计数
    uint32_t valid;       // 本包有效声学数据
    char data[1024];      // 数据内容
}__attribute__((packed)) tPing;

// 线程数据结构
typedef struct 
{
    int port;                   // 端口
    uint32_t fwrite_count;      // fwrite 执行次数
    pthread_mutex_t *mutex;     // 互斥锁
    volatile bool *running;     // 线程运行状态
} ThreadData;

//获取当前时间作为文件名
// void get_filename_from_time(char *filename, size_t len, int port) 
// {
//     time_t t = time(NULL);
//     struct tm tm = *localtime(&t);
//     snprintf(filename, len, "data_%d_%04d%02d%02d_%02d%02d%02d.bin", 
//              port, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
//              tm.tm_hour, tm.tm_min, tm.tm_sec);
// }

void get_filename_from_time(char *filename, size_t len, const char *dir, int port) 
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(filename, len, "%s/data_%d_%04d%02d%02d_%02d%02d%02d.bin", 
             dir, port, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
             tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void *receive_data(void *arg) 
{
    ThreadData *data = (ThreadData *)arg;
    int port = data->port;
    int sockfd;
    struct sockaddr_in server, client;
    int num;
    char buffer[MAXLINE];  
    char filename[64];     

    const char *directory = "/mnt/data";
    get_filename_from_time(filename, sizeof(filename), directory, port);
    FILE *file = fopen(filename, "wb");
    if (file == NULL) 
    {
        perror("fopen");
        pthread_exit(NULL);
    }

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        pthread_exit(NULL);
    }

    // 增加接收缓冲区大小
    int buffer_size = 8 * 1024 * 1024; // 设置为8MB或其他适当大小
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) == -1) 
    {
        perror("setsockopt(SO_RCVBUF)");
        pthread_exit(NULL);
    }

    // 初始化服务器端地址结构
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(IP);

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) 
    {
        perror("bind");
        pthread_exit(NULL);
    }

    int sin_size = sizeof(struct sockaddr_in);

    while (*data->running)   
    {
        num = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client, &sin_size);
        if (num < 0) 
        {
            perror("recvfrom");
            break;
        }

        if (num != MAXLINE) 
        {
            printf("Received incomplete packet of %d bytes on port %d\n", num, port);
            continue;
        }

        // 解析数据包
        // tPing *ping_data = (tPing *)buffer;
        // uint32_t ping_cnt = ntohl(ping_data->ping_cnt);
        // uint32_t pack_cnt = ntohl(ping_data->pack_cnt);

        // 每2000000次写入文件时创建新文件
        if (data->fwrite_count % 2000000 == 0 && data->fwrite_count != 0) 
        {
            data->fwrite_count = 0;
            fclose(file);
            printf("File %s saved and closed for port %d.\n", filename, port);
            get_filename_from_time(filename, sizeof(filename), directory, port);
            file = fopen(filename, "wb");
            if (file == NULL) 
            {
                perror("fopen");
                break;
            }
            printf("New file %s created for port %d.\n", filename, port);
        }

        pthread_mutex_lock(data->mutex);
        fwrite(buffer, 1, num, file);
        fflush(file);

        data->fwrite_count++; // 统计 fwrite 执行次数
        pthread_mutex_unlock(data->mutex);
    }

    fclose(file);
    close(sockfd);
    pthread_exit(NULL);
}

int main() 
{ 
    pthread_t threads[3];                              // 创建三个线程
    ThreadData thread_data[3];                         // 储存三个线程的相关程序数据
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 初始化互斥锁
    volatile bool running = true;
    int ports[] = {8081, 8082, 8083};

    for (int i = 0; i < 3; i++) 
    {
        thread_data[i].port = ports[i];
        thread_data[i].fwrite_count = 0; // 初始化 fwrite 计数
        thread_data[i].mutex = &mutex;
        thread_data[i].running = &running;
        if (pthread_create(&threads[i], NULL, receive_data, &thread_data[i]) != 0) 
        {
            perror("Error creating thread");
            exit(1);
        }
    }

    //sleep(120);     //用于控制接收端的运行时间
    //running = false;

    // 等待线程结束
    for (int i = 0; i < 3; i++) 
    {
        pthread_join(threads[i], NULL);
        printf("Port %d fwrite count: %d\n", ports[i], thread_data[i].fwrite_count);
    }

    return 0;
}
//gcc -o netudpserver netudpserver.c -lpthread