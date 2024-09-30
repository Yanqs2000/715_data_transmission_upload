#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define IP "192.168.1.110" // 服务器端IP地址
#define MAXLINE 100

int main() 
{ 
	int sockfd;         
	struct sockaddr_in server; 
	struct sockaddr_in client; 
	int sin_size; 
	int num;
	char msg[MAXLINE];  
	char sbuf[MAXLINE] = "Welcome to my server.";

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)//创建套接字  
	{
		perror("socket");
		exit(1);
	}
	// 服务器端地址
	bzero(&server,sizeof(server));
	server.sin_family=AF_INET; 
	server.sin_port=htons(PORT); 
	server.sin_addr.s_addr = inet_addr(IP); 
	
	if(bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) 
	{ 
		perror("bind");
		exit(1); 
	}    

	sin_size = sizeof(struct sockaddr_in); 
	
	while(1)   
	{
		num = recvfrom(sockfd, msg, sizeof(msg) - 1, 0, (struct sockaddr *)&client, &sin_size); 
		if (num < 0)
		{
			perror("recvfrom"); 
			exit(1); 
		} 
		msg[num] = '\0';
		printf("You got a message (%s) from %s\n", msg, inet_ntoa(client.sin_addr)); 
		strcpy(sbuf, msg);
		sendto(sockfd, sbuf, strlen(sbuf), 0, (struct sockaddr *)&client, sin_size); 
		
		if (!strcmp(msg,"quit")) 
			break;
	}

	close(sockfd);   // close listenfd
	
	return 0;
}
