#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>

#define GPIO_CHIP "/dev/gpiochip0" // GPIO 芯片
#define GPIO_PH6 230               
#define GPIO_PH7 231               

int main() 
{
    struct gpiod_chip *chip;
    struct gpiod_line *line_ph6, *line_ph7;
    int value_ph6, value_ph7;

    // 打开 GPIO 芯片
    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) 
    {
        perror("Error: Unable to open GPIO chip");
        return -1;
    }

    // 获取 PH6 和 PH7 的 GPIO 线
    line_ph6 = gpiod_chip_get_line(chip, GPIO_PH6);
    line_ph7 = gpiod_chip_get_line(chip, GPIO_PH7);
    if (!line_ph6 || !line_ph7) 
    {
        perror("Error: Unable to get GPIO lines");
        gpiod_chip_close(chip);
        return -1;
    }

    // 设置 PH6 和 PH7 为输出，默认电平为低（0）
    if (gpiod_line_request_output(line_ph6, "gpio_control", 0) < 0) 
    {
        perror("Error: Unable to configure GPIO PH6");
        gpiod_chip_close(chip);
        return -1;
    }

    if (gpiod_line_request_output(line_ph7, "gpio_control", 0) < 0) 
    {
        perror("Error: Unable to configure GPIO PH7");
        gpiod_chip_close(chip);
        return -1;
    }

    // 设置 PH6 和 PH7 为高电平
    gpiod_line_set_value(line_ph6, 1);
    gpiod_line_set_value(line_ph7, 1);
    
    // 读取并打印引脚状态
    value_ph6 = gpiod_line_get_value(line_ph6);
    value_ph7 = gpiod_line_get_value(line_ph7);
    printf("PH6 state: %d, PH7 state: %d\n", value_ph6, value_ph7);


    // 释放 GPIO 线
    gpiod_line_release(line_ph6);
    gpiod_line_release(line_ph7);
    gpiod_chip_close(chip);

    return 0;
}
//gcc -o set_gpio set_gpio.c -lgpiod