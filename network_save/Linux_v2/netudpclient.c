#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 

#define PORT 8080
#define MAXLINE 1044 // tPing结构体占20字节，数据部分占1024字节

// 定义tPing结构体
typedef struct {
    uint16_t pack_hdr;    // 数据包头 0x6162
    uint32_t pack_cnt;    // TP分包计数
    uint32_t pack_sum;    // TP总包数
    uint16_t ping_hdr;    // 发射周期头 5557
    uint32_t ping_cnt;    // 周期计数
    uint32_t valid;       // 本包有效声学数据 1024
} tPing;

int main(int argc, char *argv[]) 
{ 
    int fd, numbytes;   
    char buffer[MAXLINE]; // 完整的1044字节数据包
    struct hostent *he;   // 用于存储IP地址         
    struct sockaddr_in server, reply; // 服务器端和客户端的地址结构

    if(argc != 2) 
    {  
        printf("Usage: %s <IP Address>\n", argv[0]);
        exit(1); 
    }

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

    // 填充 tPing 结构体
    tPing ping_data;
    ping_data.pack_hdr = htons(0x6162);  // 数据包头
    ping_data.pack_cnt = htonl(1);       // TP分包计数，假设为1
    ping_data.pack_sum = htonl(1);       // TP总包数，假设为1
    ping_data.ping_hdr = htons(5557);    // 发射周期头
    ping_data.ping_cnt = htonl(1);       // 周期计数
    ping_data.valid = htonl(1024);       // 本包有效声学数据为1024字节

    // 将 tPing 结构体拷贝到 buffer 前20字节
    memcpy(buffer, &ping_data, sizeof(tPing));

    // 填充 buffer 的后1024字节为1到1024的数字
    for (int i = 0; i < 1024; i++) 
	{
        buffer[20 + i] = (char)((i + 1) % 256); // 1~1024数字转换为字节存储
    }

    // 发送消息到服务器端
    sendto(fd, buffer, MAXLINE, 0, (struct sockaddr *)&server, sizeof(struct sockaddr));

    while(1) 
    {
        int len;
        len = sizeof(struct sockaddr_in);

        // 接收来自服务器的反馈
        if ((numbytes = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&reply, &len)) == -1)
        { 
            perror("recvfrom"); 
            exit(1); 
        }

        // 检查接收数据是否来自预期的服务器
        if (len != sizeof(struct sockaddr) || memcmp((const void *)&server, (const void *)&reply, len) != 0) 
        {
            printf("Receive message from other server.\n");
            continue;
        }

        // 打印服务器端反馈的消息
        buffer[numbytes] = '\0'; // 确保字符串终止
        printf("Server Message: %s\n", buffer); 

        // 读取用户输入，准备发送新的数据包
        printf("Next sending input (or type 'quit' to exit): ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strlen(buffer) - 1] = '\0'; // 去除换行符

        // 发送新消息到服务器端
        sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *)&server, sizeof(struct sockaddr));

        // 如果输入 "quit"，则退出循环
        if(strcmp(buffer, "quit") == 0)
            break;
    }

    close(fd); // 关闭套接字
    return 0;
}
