#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
//#define IP "192.168.1.110" // 服务器端IP地址
#define IP "127.0.0.1" // 本地回环地址
#define MAXLINE 1044 // 数据包大小 20字节包头 + 1024字节数据内容

// 定义tPing结构体
typedef struct {
    uint16_t pack_hdr;    // 数据包头 0x6162
    uint32_t pack_cnt;    // TP分包计数
    uint32_t pack_sum;    // TP总包数
    uint16_t ping_hdr;    // 发射周期头  5557有发射头，5555无
    uint32_t ping_cnt;    // 周期计数
    uint32_t valid;       // 本包有效声学数据
} tPing;

int main() 
{ 
    int sockfd;         
    struct sockaddr_in server; 
    struct sockaddr_in client; 
    int sin_size; 
    int num;
    char buffer[MAXLINE];  // 用于接收完整的数据包

    // 打开文件，保存接收到的数据
    FILE *file = fopen("received_data.bin", "wb");
    if (file == NULL) {
        perror("fopen");
        exit(1);
    }

    // 创建UDP套接字
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
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
        num = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr *)&client, &sin_size);
        if (num < 0) 
        {
            perror("recvfrom");
            exit(1);
        }

        // 解析数据包的前20字节为 tPing 结构
        tPing *ping_data = (tPing *)buffer;
        printf("Received packet %u/%u from %s, ping count: %u, valid data: %u\n",
               ping_data->pack_cnt, ping_data->pack_sum,
               inet_ntoa(client.sin_addr),
               ping_data->ping_cnt, ping_data->valid);
        
        // 打印包头内容
        printf("Pack header: 0x%x\n", ping_data->pack_hdr);

        // 将完整的1044字节数据包写入文件
        fwrite(buffer, 1, num, file);
        fflush(file);  // 确保每次都写入磁盘

        // 发送回执给客户端
        sendto(sockfd, "Received", 8, 0, (struct sockaddr *)&client, sin_size);

        // 检查是否收到退出命令
        if (!strcmp(buffer + 20, "quit")) // 比较数据包中数据内容部分（第21字节开始）
        {
            printf("Quit command received. Closing server.\n");
            break;
        }
    }


    // 关闭文件和套接字
    fclose(file);
    close(sockfd);
    
    return 0;
}

