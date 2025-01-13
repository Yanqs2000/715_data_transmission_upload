#include <stdio.h>
#include <stdlib.h>

int main() 
{
    // 要执行的命令
    const char *command = "sudo systemctl start camera.service";
    
    // 执行命令
    int result = system(command);

    // 检查命令执行结果
    if (result == -1) 
    {
        perror("system");
        return 1;
    }

    printf("Command executed successfully\n");
    return 0;
}
