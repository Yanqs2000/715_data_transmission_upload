#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <stdint.h>

#define PORT 8080
#define MAXLINE 1044 // tPing结构体占20字节，数据部分占1024字节
#define TOTAL_PACKETS 10 //每个tPing发送100个包

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
    int fd, numbytes;
    uint32_t cnt = 1;   
    char buffer[MAXLINE]; 
    char tPing_buffer[MAXLINE];// 完整的1044字节数据包

    //struct hostent 是一个用于存储主机信息的结构体
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

    for(int i = 0; i < TOTAL_PACKETS; i++)
    {
        // 填充 tPing 结构体
        tPing ping_data;
        ping_data.pack_hdr = htons(0x6162);        // 数据包头
        ping_data.pack_cnt = htonl(i + 1);         // TP分包计数，假设为1
        ping_data.pack_sum = htonl(TOTAL_PACKETS); // TP总包数，假设为1
        ping_data.ping_hdr = htons(5557);          // 发射周期头
        ping_data.ping_cnt = htonl(1);             // 周期计数
        ping_data.valid = htonl(1024);             // 本包有效声学数据为1024字节

        // 填充数据部分: 前i个字节为字母 'a'，其余为字母 'A'
        memset(ping_data.data, 'A', sizeof(ping_data.data)); // 全部先填充为 'A'
        memset(ping_data.data, 'a', i + 1);                    // 开头i字节为 'a'
    
        // 将 tPing 结构体拷贝到 tPing_buffer 中
        memcpy(tPing_buffer, &ping_data, sizeof(tPing));

        // 发送消息到服务器端
        sendto(fd, tPing_buffer, sizeof(tPing), 0, (struct sockaddr *)&server, sizeof(struct sockaddr));
    }
    
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


    
    // printf("type 'quit' to exit: ");
    // fgets(buffer, sizeof(buffer), stdin);
    // buffer[strlen(buffer) - 1] = '\0'; // 去除换行符

    // // 如果输入 "quit"，则退出循环
    // if(strcmp(buffer, "quit") == 0)
    //     break;
    
    // close(fd); // 关闭套接字
    // return 0;

    
    // while(1) 
    // {
    //     int len;
    //     len = sizeof(struct sockaddr_in);

    //     // 接收来自服务器的反馈
    //     if ((numbytes = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&reply, &len)) == -1)
    //     { 
    //         perror("recvfrom"); 
    //         exit(1); 
    //     }

    //     // 检查接收数据是否来自预期的服务器
    //     if (len != sizeof(struct sockaddr) || memcmp((const void *)&server, (const void *)&reply, len) != 0) 
    //     {
    //         printf("Receive message from other server.\n");
    //         continue;
    //     }

    //     // 打印服务器端反馈的消息
    //     buffer[numbytes] = '\0'; // 确保字符串终止
    //     printf("Server Message: %s\n", buffer); 

    //     //目前只能发送一次

    //     // 读取用户输入，准备发送新的数据包
    //     printf("Next sending input (or type 'quit' to exit): ");
    //     fgets(buffer, sizeof(buffer), stdin);
    //     buffer[strlen(buffer) - 1] = '\0'; // 去除换行符

    //     // // 发送新消息到服务器端
    //     // sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *)&server, sizeof(struct sockaddr));

    //     // 如果输入 "quit"，则退出循环
    //     if(strcmp(buffer, "quit") == 0)
    //         break;
    // }
    close(fd); // 关闭套接字
    return 0;
}
