#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

#define PORT 8080
//#define IP_ADDRESS "192.168.1.104"  // 目标 IP 地址
#define IP_ADDRESS "127.0.0.1"  // 本地 IP 地址

// 定义发送的数据包
uint8_t start_command[32] = 
{
    0xF0, 0x00, 0x20, 0x00, 0x0A, 0x0A, 0x0A, 0x0A,
    0x0B, 0x0B, 0x0A, 0x0A, 0x00, 0x00, 0x0B, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0D, 0x0A,
};

int main() 
{
    int sockfd;
    struct sockaddr_in server_addr, reply_addr;
    char buffer[128];  // 用于接收来自接收端的回复
    socklen_t reply_addr_len = sizeof(reply_addr);

    // 创建 UDP 套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        perror("socket creation failed");
        exit(1);
    }

    // 设置目标服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);  // 目标端口
    
    if (inet_pton(AF_INET, IP_ADDRESS, &server_addr.sin_addr) <= 0) 
    {
        perror("Invalid address or address not supported");
        exit(1);
    }

    // 发送数据包
    ssize_t bytes_sent = sendto(sockfd, start_command, sizeof(start_command), 0,
                                 (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bytes_sent == -1) 
    {
        perror("sending failed");
        close(sockfd);
        exit(1);
    }

    printf("Sent %d bytes to %s:%d\n", bytes_sent, IP_ADDRESS, PORT);

    // 等待并接收来自接收端的回复
    ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&reply_addr, &reply_addr_len);
    if (bytes_received == -1) {
        perror("recvfrom failed");
        close(sockfd);
        exit(1);
    }

    // 打印接收到的消息
    buffer[bytes_received] = '\0';  // 确保字符串终止
    printf("Received %d bytes from %s:%d\n", bytes_received, inet_ntoa(reply_addr.sin_addr), ntohs(reply_addr.sin_port));
    
    printf("Reply from ARM:\n");

    for (int i = 0; i < 32; i++) 
    {
        printf("%02X ", buffer[i]);

        if((i + 1) % 8 == 0)
        {
            printf("\n");
        }
    }

    // 比较发送的数据包和接收到的数据包
    if (bytes_received == sizeof(start_command) && memcmp(buffer, start_command, sizeof(start_command)) == 0) 
    {
        printf("Right: The received message is identical to the sent message.\n");
    } 
    else 
    {
        printf("Wrong: The received message differs from the sent message.\n");
    }

    // 关闭套接字
    close(sockfd);
    return 0;
}
//gcc -o send_initial_command send_initial_command.c
