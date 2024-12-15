#include "Thread_ReadUSB.h"
#include "gloghelper.h"

const std::vector<uint8_t> frame_header = {0xAA, 0x55, 0x5A, 0xA5};
const size_t GuanDao_total_bytes = 110; // 假设数据帧总长度为110个字节

// std::mutex mutex;
// std::condition_variable cv;
// std::deque<uint8_t> buffer;


int USB_init(const char* filename)
{
// 假设 usb_serial 是您已经配置和打开的串口文件描述符
    int usb_serial = open(filename, O_RDONLY | O_NOCTTY | O_SYNC);
    if (usb_serial < 0) {
        // std::cerr << "Error opening usb_serial: " << strerror(errno) << std::endl;
        LOG(ERROR) << "Error opening usb_serial: " << strerror(errno);
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);
    // 获取当前串口配置
    if (tcgetattr(usb_serial, &tty) != 0) {
        std::cerr << "Error from tcgetattr: " << strerror(errno) << std::endl;
        LOG(ERROR) << "Error from tcgetattr: " << strerror(errno);
        return -1;
    }
    // 设置波特率为9600，这是一个常见的波特率，你需要根据你的设备来设置
    cfsetospeed(&tty, B230400);
    cfsetispeed(&tty, B230400);

    // 其他串口配置...
        // 设置8N1模式
//     tty.c_cflag &= ~PARENB; // 清除奇偶校验位，禁用奇偶校验（最常见）
//     tty.c_cflag &= ~CSTOPB; // 清除停止位字段，通信中只使用一个停止位（最常见）
//     //tty.c_cflag &= ~CSIZE; // 清除所有设置数据大小的位
//     tty.c_cflag |= CS8; // 每个字节8位（最常见）
//     tty.c_cflag &= ~CRTSCTS; // 禁用RTS/CTS硬件流控（最常见）
//    // tty.c_cflag |= CREAD | CLOCAL; // 打开READ & 忽略控制线（CLOCAL = 1）


//     // tty.c_lflag &= ~ICANON; // 禁用规范模式
//     // tty.c_lflag &= ~ECHO; // 禁用回显
//     // tty.c_lflag &= ~ECHOE; // 禁用擦除回显
//     // tty.c_lflag &= ~ECHONL; // 禁用换行回显
//     // tty.c_lflag &= ~ISIG; // 禁用INTR, QUIT, SUSP的解释

//     // tty.c_iflag &= ~(IXON | IXOFF | IXANY); // 关闭软件流控制
//     // tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // 禁用接收字节的特殊处理

//     // tty.c_oflag &= ~OPOST; // 防止输出字节的特殊解释（例如，换行符）
//     // tty.c_oflag &= ~ONLCR; // 防止换行转换为回车换行

//     // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT IN LINUX)
//     // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT IN LINUX)

//     tty.c_cc[VTIME] = 5;    // 等待高达1s（10分之一秒），一旦接收到任何数据就返回。
//     tty.c_cc[VMIN] = 1;      // 最小接收字符数为0，这意味着`read`可以立即返回，不管有没有数据。
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1;     // 等待高达1s（10分之一秒），一旦接收到任何数据就返回。
    tty.c_cc[VTIME] = 5;         // 最小接收字符数为0，这意味着`read`可以立即返回，不管有没有数据。

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    //int flags = fcntl(usb_serial,F_GETFL,0); //设置阻塞模式
    //fcntl(usb_serial, F_SETFL,flags | O_NONBLOCK);

    if (tcsetattr(usb_serial, TCSANOW, &tty) != 0)
    {
        // std::cerr << "Error from tcsetattr: " << strerror(errno) << std::endl;
        LOG(ERROR) << "Error from tcsetattr: " << strerror(errno);
        return -1;
    }
    
    return usb_serial;
}

std::vector<uint8_t> read_process(int usb_serial, int GuanDao_total_bytes)
{
    const std::vector<uint8_t> frame_header = {0xAA, 0x55, 0x5A, 0xA5};
    uint8_t byte;
    std::deque<uint8_t> buffer;
    std::vector<uint8_t> frame(GuanDao_total_bytes);
    time_t start_time;
    time(&start_time);
    
    while (true) 
    {
	time_t end_time;
	time(&end_time);
	if((end_time - start_time) > 3) //超过3秒则退出
	{
	
		LOG(ERROR) << "read_process:[overtime 3s]read usb data error";
		break;
	}
        int num_bytes = read(usb_serial, &byte, 1);
        if (num_bytes > 0) {
            buffer.push_back(byte);
        }
        bool found = false;  
        if(buffer.size() >= 110)
        {
             if (std::equal(frame_header.begin(), frame_header.end(), buffer.begin())) {
                found = true;
                std::copy(buffer.begin(), buffer.begin() + GuanDao_total_bytes, frame.begin());
                buffer.erase(buffer.begin(), buffer.begin() + GuanDao_total_bytes);
                
                return frame; 

            } else {
                buffer.pop_front();
            }
        }
        
    }

    return frame;
}


// 串口读取线程函数
// void readSerial(int usb_serial, SafeQueue<uint8_t>& data) {
//     uint8_t byte;
//     while (true) {
//     int num_bytes = read(usb_serial, &byte, 1);
//     if (num_bytes > 0) {
//         // std::lock_guard<std::mutex> lock(mutex);
//         data.push(byte);
//         // cv.notify_one();
//     } 
//         // else if (num_bytes < 0) {
//         //     std::cerr << "Error reading: " << strerror(errno) << std::endl;
//         //     break;
//         // }
        
//     }
// }

// // 数据处理线程函数 团结就是力量，团结就是力量，这力量是铁，这力量是钢
// void processDataFrame(std::vector<uint8_t> frame) {
//     // 在这里处理帧数据
//     // ...
//     std::cout << "Processing a frame of data..." << std::endl;
//     DataExtractor(frame);

// }

// 数据处理线程函数
// void processData(SafeQueue<uint8_t>& buffer, SafeVector<uint8_t>& data) {
//     // std::unique_lock<std::mutex> lock(mutex);
//     std::vector<uint8_t> frame(GuanDao_total_bytes);
//     // frame.reserve(GuanDao_total_bytes);
//     while (true) {
//         // cv.wait(lock, [] { return buffer.size() >= GuanDao_total_bytes; });

//         // 寻找帧头
//         bool found = false;    
//         while (buffer.size() >= GuanDao_total_bytes && !found) 
//         {
//             //  std::cout<<"buffer size0: "<< std::dec<<buffer.size() <<std::endl;
//             // auto start = std::chrono::high_resolution_clock::now();
//             if (std::equal(frame_header.begin(), frame_header.end(), buffer.begin())) {
//                 found = true;
//                 std::copy(buffer.begin(), buffer.begin() + GuanDao_total_bytes, frame.begin());
//                 buffer.erase(GuanDao_total_bytes);

//             } else {
//                 buffer.pop();
//             }
//         }
//         // lock.unlock();
//         if (found) {
//             if (frame.capacity() == GuanDao_total_bytes) {
//                 data.clear();
//                 data.copyfrom(frame);
//             }

//             // for(int i = 0; i < GuanDao_total_bytes; ++i)
//             // {
//             //     std::cout<< std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
//             // }
//             // std::cout<<std::endl;
//         }
//         // lock.lock();
//     }
// }
