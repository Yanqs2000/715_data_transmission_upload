#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>
#include <stdint.h> // 包含 uint8_t 类型

#define SHM_SIZE 256
#define SEM_NAME "/my_semaphore"
#define SHM_KEY 1234 // 固定的键值

int main() {
    int shmid;
    uint8_t *str; // 修改为 uint8_t 类型指针
    sem_t *sem;

    // 获取共享内存
    shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (shmid < 0) {
        perror("shmget");
        return 1;
    }

    str = (uint8_t*)shmat(shmid, NULL, 0);
    if (str == (uint8_t*)(-1)) {
        perror("shmat");
        return 1;
    }

    // 打开信号量
    sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    while (true) {
        // 使用信号量进行同步
        sem_wait(sem);
        
        // 输出数据
        std::cout << "Latest data: ";
        for (size_t i = 0; i < SHM_SIZE; ++i) {
            std::cout << std::hex << static_cast<int>(str[i]) << " "; // 输出十六进制值
        }
        std::cout << std::dec << std::endl; // 恢复为十进制输出

        sem_post(sem);

        sleep(1); // 模拟处理间隔
    }

    // Detach
    shmdt(str);
    sem_close(sem);
    sem_unlink(SEM_NAME);

    return 0;
}