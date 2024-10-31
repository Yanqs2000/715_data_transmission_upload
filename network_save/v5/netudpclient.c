#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <stdint.h>
#include <time.h>

#define PORT 8080
#define MAXLINE 1044 // tPing结构体占20字节，数据部分占1024字节
#define TOTAL_PACKETS 100 //每个tPing发送100个包
#define INTERVAL 400 // （1000：1毫秒）

// 定义tPing结构体
typedef struct 
{
    uint16_t pack_hdr;    // 数据包头 0x6162
    uint32_t pack_cnt;    // TP分包计数
    uint32_t pack_sum;    // TP总包数
    uint16_t ping_hdr;    // 发射周期头 5557
    uint32_t ping_cnt;    // 周期计数
    uint32_t valid;       // 本包有效声学数据字节数 1024
    char data[1024];      // 数据内容1024个字节
}__attribute__((packed)) tPing;

int main(int argc, char *argv[]) 
{ 
    if(argc != 3) 
    {  
        printf("Usage: %s <IP Address> <Duration in seconds>\n", argv[0]);
        exit(1); 
    }

    int fd, numbytes;
    int duration = atoi(argv[2]);       // 获取工作周期，每0.1秒为1个工作周期
    uint32_t j = 0;
    char buffer[MAXLINE]; 
    char tPing_buffer[MAXLINE];// 完整的1044字节数据包

    struct timespec start, end;
    double total_time;

    //struct hostent 是一个用于存储主机信息的结构体
    struct hostent *he;   // 用于存储IP地址         
    struct sockaddr_in server, reply; // 服务器端和客户端的地址结构

    if((he = gethostbyname(argv[1])) == NULL)
    { 
        printf("gethostbyname() error\n"); 
        exit(1); 
    } 

    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) // 创建UDP套接字
    { 
        printf("socket error\n"); 
        exit(1); 
    }

    // 初始化服务器端地址结构
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET; 
    server.sin_port = htons(PORT);  
    server.sin_addr = *((struct in_addr *)he->h_addr);

    clock_gettime(CLOCK_MONOTONIC, &start);  // 开始计时
    while (duration) // 持续工作
    {
        for(int i = 0; i < TOTAL_PACKETS; i++)
        {
            // 填充 tPing 结构体
            tPing ping_data;
            ping_data.pack_hdr = htons(0x6162);            // 数据包头
            ping_data.pack_cnt = htonl(i + 1);             // TP分包计数
            ping_data.pack_sum = htonl(TOTAL_PACKETS);     // TP总包数，为100
            ping_data.ping_hdr = htons(5557);              // 发射周期头
            ping_data.ping_cnt = htonl(j + 1);             // 周期计数
            ping_data.valid = htonl(1024);                 // 本包有效声学数据为1024字节

            // 填充数据部分: 前i个字节为字母 'a'，其余为字母 'A'
            memset(ping_data.data, 'A', sizeof(ping_data.data));   // 全部先填充为 'A'
            memset(ping_data.data, 'a', i + 1);                    // 开头i字节为 'a'
        
            // 将 tPing 结构体拷贝到 tPing_buffer 中
            memcpy(tPing_buffer, &ping_data, sizeof(tPing));

            // 发送消息到服务器端
            sendto(fd, tPing_buffer, sizeof(tPing), 0, (struct sockaddr *)&server, sizeof(struct sockaddr));
            usleep(INTERVAL);  // 暂停1毫秒
        }
        j = j + 1;
        duration = duration - 1;
        usleep(INTERVAL);  // 暂停1毫秒 
    }
    clock_gettime(CLOCK_MONOTONIC, &end);  // 结束计时

    // 发送消息到服务器端
    sendto(fd, "over", 4, 0, (struct sockaddr *)&server, sizeof(struct sockaddr));

    int len;
    len = sizeof(struct sockaddr_in);

    // 接收来自服务器的反馈
    if ((numbytes = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&reply, &len)) == -1)
    { 
        perror("recvfrom"); 
        exit(1); 
    }
    // 打印服务器端反馈的消息
    buffer[numbytes] = '\0'; // 确保字符串终止
    printf("Server Message: %s\n", buffer); 

    printf("j:%u\n",j);

    // 计算秒和纳秒
    total_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("Function execution time: %f seconds\n", total_time);

    close(fd); // 关闭套接字
    return 0;
}
