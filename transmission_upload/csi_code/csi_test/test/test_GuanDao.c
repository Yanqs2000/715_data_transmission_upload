#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>

#include <linux/serial.h>
#include <sys/ioctl.h>
#include <asm-generic/ioctl.h>
#include <termios.h>
#include <linux/serial.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define TOTAL_BYTES 110
#define HEADER_SIZE 4 // 假设帧头为4字节
#define FRAME_HEADER {0xAA, 0x55, 0x5A, 0xA5};//定义惯导帧头

// 全局变量，用于控制循环
volatile bool keep_running = true;

// 信号处理函数
void handle_signal(int signal) 
{
    if (signal == SIGINT) 
    {
        keep_running = false; // 设置标志为 false，退出循环
    }
}

// USB初始化
int USB_init_GuanDao(const char* filename)//接受一个字符指针 filename，表示串口设备的文件名
{
    // 假设usb_serial是您已经配置和打开的串口文件描述符
    // 使用open函数以只读模式打开指定的串口文件。O_NOCTTY 表示不将此设备设为控制终端，O_SYNC 表示同步读写。
    int usb_serial = open(filename, O_RDONLY | O_NOCTTY | O_SYNC);
    if (usb_serial < 0) 
    {
        perror("Failed to open serial uart3");
        return -1;
    }

    //定义一个termios结构体tty来保存串口配置，并将其内容初始化为零
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    // 调用 tcgetattr获取当前串口配置
    if (tcgetattr(usb_serial, &tty) != 0) 
    {
        perror("Error from tcgetattr: uart3");
        return -1;
    }

    // 设置串口的波特率为9600。cfsetospeed 和 cfsetispeed 分别设置输出和输入波特率。
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;//CS8表示数据位为8位
    tty.c_iflag &= ~IGNBRK;                    //IGNBRK表示忽略Break信号
    tty.c_lflag = 0;                           //c_lflag和c_oflag设置为0，禁用特殊处理    
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;                        //VMIN设置至少接收0个字符
    tty.c_cc[VTIME] = 1;                       //VTIME设置超时时间为x个十分之一秒
    tty.c_iflag &= ~(ICRNL | INLCR);  // 禁用 \r 到 \n 的转换,和tty.c_oflag = 0;功能相同

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);    //禁用软件流控制（IXON, IXOFF）
    tty.c_cflag |= (CLOCAL | CREAD);           //启用接收和本地模式（CREAD, CLOCAL）
    tty.c_cflag &= ~(PARENB | PARODD);         //禁用奇偶校验（PARENB, PARODD）
    tty.c_cflag &= ~CSTOPB;                    //设置为一个停止位（CSTOPB）
    tty.c_cflag &= ~CRTSCTS;                   //禁用 RTS/CTS 硬件流控制（CRTSCTS）

    //使用tcsetattr应用配置。如果失败，记录错误并返回-1
    if (tcsetattr(usb_serial, TCSANOW, &tty) != 0)
    {
        perror("Error from tcgetattr: uart3");
        close(usb_serial);
        return -1;
    }

    struct serial_struct serinfo;
    int ret = ioctl(usb_serial, TIOCGSERIAL, &serinfo);
    
    if (ret != 0) 
    {
        close(usb_serial);
        return -2;
    }

    serinfo.flags |= ASYNC_LOW_LATENCY; // 降低延迟
    
    ioctl(usb_serial, TIOCSSERIAL, &serinfo);

    if (ioctl(usb_serial, TIOCGSERIAL, &serinfo) < 0) 
    {
        perror("TIOCGSERIAL failed");
        return -1;
    }

    serinfo.xmit_fifo_size = 1024*1024; //1M

    ret = ioctl(usb_serial, TIOCSSERIAL, &serinfo);
    if(ret != 0) 
    {
        close(usb_serial);
        return -3;
    }

    return usb_serial;
}

// GuanDao读取函数
uint8_t *read_process(int usb_serial, int total_bytes, const uint8_t* frame_header, size_t header_size) 
{

    uint8_t byte;
    uint8_t buffer[1100]; 
    int buffer_index = 0;

    static uint8_t frame[110];

    while (true) 
    {
        // Read one byte from the serial port
        int num_bytes = read(usb_serial, &byte, 1);

        if (num_bytes > 0) 
        {
            // Add the byte to the buffer
            if (buffer_index < sizeof(buffer)) 
            {
                buffer[buffer_index] = byte;
                buffer_index++;
            } 
            else 
            {
                // Buffer overflow, reset the buffer
                buffer_index = 0;
            }
        } 
        else if (num_bytes == 0) 
        {
            printf("GuanDao no data received\n");
            break;
        } 
        else 
        {
            printf("Error while reading GuanDao\n");
            break;
        }

        if (buffer_index >= total_bytes) 
        {
            bool found = true;

            for (size_t i = 0; i < header_size; ++i) 
            {
                if (buffer[i] != frame_header[i]) 
                {
                    found = false;
                    break;
                }
            }

            if (found) 
            {
                memcpy(frame, buffer, total_bytes); // 复制有效帧到frame
                memmove(buffer, buffer + total_bytes, buffer_index - total_bytes); // 移除已处理的帧
                buffer_index -= total_bytes;
                return frame;
            } 
            else 
            {
                memmove(buffer, buffer + 1, --buffer_index); // Remove the first byte
                printf("data mismatch\n");
            }
        }
    }
    return NULL;
}


// 时间、地点抽取函数
int DataExtractor_raw(const uint8_t *frame, size_t frame_size, uint8_t *date_time, size_t date_time_size)
{
    // Ensure frame and date_time sizes are sufficient
    if (frame_size < 110)
    {
        printf("Data receive error: frame size = %zu\n", frame_size);
        return -1;
    }

    if (date_time_size < 22)
    {
        printf("Data receive error: date_time buffer size is too small\n");
        return -1;
    }

    // Extract relevant data to date_time array
    date_time[0] = frame[6];
    date_time[1] = frame[7];
    date_time[2] = frame[8];
    date_time[3] = frame[9];
    date_time[4] = frame[10];
    date_time[5] = frame[11];
    date_time[6] = frame[12];
    date_time[7] = frame[13];
    date_time[8] = frame[26];
    date_time[9] = frame[27];
    date_time[10] = frame[28];
    date_time[11] = frame[29];
    date_time[12] = frame[30];
    date_time[13] = frame[31];
    date_time[14] = frame[90];
    date_time[15] = frame[91];
    date_time[16] = frame[92];
    date_time[17] = frame[93];
    date_time[18] = frame[94];
    date_time[19] = frame[95];
    date_time[20] = frame[96];
    date_time[21] = frame[97];

    return 1;
}

int main() 
{
    const char* serial_device_GuanDao = "/dev/ttyS3"; // 串口3(GuanDao)设备路径
    int frame_number = 0;
    struct timeval time_start, time_end, time_diff;//time_start、time_end 和 time_diff 用于计算程序运行时间等。
    uint32_t cost_time;
    float average_rate;

    FILE* output_file = fopen("output_data.bin", "wb");
    if (!output_file) 
    {
        perror("Failed to open file for writing");
        return -1;
    }
    
    
    // 配置串口
    int serial_fd_GuanDao = USB_init_GuanDao(serial_device_GuanDao);
    if (serial_fd_GuanDao == -1) 
    {
        return -1;
    }

    // 定义帧头
    uint8_t frame_header[HEADER_SIZE] = FRAME_HEADER;

    // 设置信号处理函数
    signal(SIGINT, handle_signal);

    //使用 gettimeofday 函数获取当前时间作为程序开始时间
    gettimeofday(&time_start, NULL);

    while (keep_running) 
    {
        uint8_t date_time[22];
        // 调用读取函数
        uint8_t* frame = read_process(serial_fd_GuanDao, TOTAL_BYTES, frame_header, HEADER_SIZE);
        
        if (frame != NULL) 
        {
            DataExtractor_raw(frame, TOTAL_BYTES, date_time, sizeof(date_time));
            frame_number++;
            // 成功读取完整的数据包，打印或处理数据
            
            if(frame_number == 1 || frame_number == 2)
            {
                printf("Received valid data frame:\n");
                for (int i = 0; i < 22; i++) 
                {
                    printf("%02X ", date_time[i]);
                    if ((i + 1) % 11 == 0)
                    {
                        printf("\n");
                    }
                } 
            }
        } 
        else 
        {
            // 没有收到数据或发生错误
            memset(date_time, 0, sizeof(date_time));  // 将所有元素设置为0
            for (int i = 0; i < 22; i++) 
            {
                printf("%02X ", date_time[i]);
                if ((i + 1) % 11 == 0)
                {
                    printf("\n");
                }
            } 
            printf("No valid frame received\n");
        }

        fwrite(date_time, sizeof(uint8_t), sizeof(date_time), output_file);

        if(frame_number == 50)
        {
            keep_running = false;
        }
        // 根据数据源发送频率进行适当的延时
        usleep(50000); // 100毫秒延时，对应10Hz发送频率100000
    }
    printf("\n%d\n",frame_number);

    gettimeofday(&time_end, NULL);
    timersub(&time_end, &time_start, &time_diff);
    cost_time = time_diff.tv_sec * 1000.0 + time_diff.tv_usec / 1000;
    printf("time: %d ms\n", cost_time);
    // 关闭串口
    close(serial_fd_GuanDao);

    return 0;
}

