#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ini.h"

// 定义一个结构体来存储解析后的参数
struct Params {
    char* device;
    char* format;
    unsigned int width;
    unsigned int height;
    unsigned int number;
};

// 解析INI文件内容并将其存储到结构体中
int config_handler(void* user, const char* section, const char* name, const char* value) {
    struct Params* params = (struct Params*)user;

    if (strcmp(section, "parameters") == 0) {
        if (strcmp(name, "device") == 0) {
            params->device = strdup(value);  // 存储设备名称
        } else if (strcmp(name, "format") == 0) {
            params->format = strdup(value);  // 存储格式
        } else if (strcmp(name, "width") == 0) {
            params->width = strtoul(value, NULL, 10);  // 存储宽度
        } else if (strcmp(name, "height") == 0) {
            params->height = strtoul(value, NULL, 10);  // 存储高度
        } else if (strcmp(name, "number") == 0) {
            params->number = strtoul(value, NULL, 10);  // 存储数量
        }
    }
    return 1;  // 返回非零表示继续处理
}

int main() 
{
    struct Params params = {0};  // 初始化结构体

    // 解析config.ini文件并将结果存储到params结构体中
    if (ini_parse("config.ini", config_handler, &params) < 0) 
    {
        printf("Error reading INI file: config.ini\n");
        return -1;
    }

    // 打印解析后的参数
    printf("Device: %s\n", params.device);
    printf("Format: %s\n", params.format);
    printf("Width: %u\n", params.width);
    printf("Height: %u\n", params.height);
    printf("Number: %u\n", params.number);

    // 记得释放动态分配的内存
    free(params.device);
    free(params.format);

    return 0;
}



//gcc -o test test.c ini.c
