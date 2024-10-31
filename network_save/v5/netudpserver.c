#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>

#define PORT 8080
//#define IP "192.168.1.110" // 服务器端IP地址
#define IP "127.0.0.1" // 本地回环地址
#define MAXLINE 1044 // 数据包大小 20字节包头 + 1024字节数据内容
#define EPOCH 20000

// 定义tPing结构体
typedef struct 
{
    uint16_t pack_hdr;    // 数据包头 0x6162
    uint32_t pack_cnt;    // TP分包计数
    uint32_t pack_sum;    // TP总包数
    uint16_t ping_hdr;    // 发射周期头  5557有发射头，5555无
    uint32_t ping_cnt;    // 周期计数
    uint32_t valid;       // 本包有效声学数据
    char data[1024];      // 数据内容1024个字节
}__attribute__((packed)) tPing;


void print_bytes(const char *buffer, int n) 
{
    for (int i = 0; i < n; i++) 
    {
        // 打印每个字节的十六进制表示形式
        printf("%02X ", (unsigned char)buffer[i + 20]);
    }
    printf("\n");
}

// 函数：根据当前时间生成文件名
void get_filename_from_time(char *filename, size_t len) 
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(filename, len, "data_%04d%02d%02d_%02d%02d%02d.bin", 
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
             tm.tm_hour, tm.tm_min, tm.tm_sec);
}

int main() 
{ 
    int sockfd;         
    struct sockaddr_in server; 
    struct sockaddr_in client; 
    int sin_size; 
    int num;
    char buffer[MAXLINE];  // 用于接收完整的数据包
    char filename[64];     // 文件名存储

    //初始化文件夹名
    get_filename_from_time(filename, sizeof(filename));
    // 打开文件，保存接收到的数据
    FILE *file = fopen(filename, "wb");
    if (file == NULL) 
    {
        perror("fopen");
        exit(1);
    }

    // 创建UDP套接字
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    // 增加接收缓冲区大小
    int buffer_size = 8 * 1024 * 1024; // 设置为4MB或其他适当大小
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) == -1) 
    {
        perror("setsockopt(SO_RCVBUF)");
        exit(1);
    }

    // 初始化服务器端地址结构
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(IP);
    
    // 绑定地址
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) 
    {
        perror("bind");
        exit(1);
    }

    sin_size = sizeof(struct sockaddr_in);
    
    while (1)   
    {
        // 接收数据包
        num = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client, &sin_size);
        if (num < 0) 
        {
            perror("recvfrom");
            exit(1);
        }

        // 确保接收到的数据长度为 1044 字节
        if (num != MAXLINE) 
        {
            if (num == 4)
            {
                goto over;
            }
            else
            {
                printf("Received incomplete packet of %d bytes\n", num);
                continue;
            }

        }

        // 解析数据包
        tPing *ping_data = (tPing *)buffer;

        // 将网络字节序转换为主机字节序
        //uint16_t pack_hdr = ntohs(ping_data->pack_hdr);
        uint32_t pack_cnt = ntohl(ping_data->pack_cnt);
        //uint32_t pack_sum = ntohl(ping_data->pack_sum);
        //uint16_t ping_hdr = ntohs(ping_data->ping_hdr);
        uint32_t ping_cnt = ntohl(ping_data->ping_cnt);
        //print("ping_cnt:%u",ping_cnt);
        //uint32_t valid = ntohl(ping_data->valid);

        // printf("Received packet %u/%u from %s, ping count: %u, valid data: %u\n",
        //        pack_cnt, pack_sum,
        //        inet_ntoa(client.sin_addr),
        //        ping_cnt, valid);
        
        // // 打印包头内容
        // printf("Pack header: 0x%x\n", pack_hdr);
        // printf("Ping hdr:%u\n",ping_hdr);
        // print_bytes(buffer,pack_cnt);

        //每20000次ping_cnt时创建新文件,文件大小小于2GB
        if (ping_cnt % EPOCH == 1 && ping_cnt != 1 && pack_cnt == 1) 
        {
            // 关闭当前文件
            fclose(file);
            printf("File %s saved and closed.\n", filename);

            // 生成新的文件名并打开新文件
            get_filename_from_time(filename, sizeof(filename));
            file = fopen(filename, "wb");
            if (file == NULL) 
            {
                perror("fopen");
                exit(1);
            }
            printf("New file %s created.\n", filename);
        }

        // 将完整的1044字节数据包写入文件
        fwrite(buffer, 1, num, file);
        fflush(file);  // 确保每次都写入磁盘
    }
over:
    // 检查是否收到退出命令
    if (!strcmp(buffer, "over"))
    {
        printf("Closing server.\n");
    }

    // 发送回执给客户端
    sendto(sockfd, "Received", 8, 0, (struct sockaddr *)&client, sin_size);

    // 关闭文件和套接字
    fclose(file);
    close(sockfd);
    
    return 0;
}

