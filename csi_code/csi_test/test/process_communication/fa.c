#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <stdint.h>

#define SHM_SIZE 256
#define SEM_NAME "/my_semaphore"
#define SHM_KEY 1234 // 固定的键值

int main() {
    int shmid;
    uint8_t *str;             // 修改为 uint8_t 类型指针
    sem_t *sem;

    // 创建共享内存
    shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }
    
    str = (uint8_t *)shmat(shmid, NULL, 0); // 共享内存连接
    if (str == (uint8_t *)(-1)) {
        perror("shmat");
        exit(1);
    }

    // 创建信号量
    sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    uint8_t data[SHM_SIZE]; // 修改为 uint8_t 类型数组
    while (1) {
        // 模拟读取十六进制数据
        for (size_t i = 0; i < SHM_SIZE; i++) {
            data[i] = i % 256; // 模拟数据，范围在 0-255
        }

        // 使用信号量进行同步
        sem_wait(sem);
        memcpy(str, data, SHM_SIZE); // 使用 memcpy 复制数据
        sem_post(sem);

        sleep(1); // 模拟读取间隔
    }

    // Detach and destroy
    shmdt(str);
    shmctl(shmid, IPC_RMID, NULL);
    sem_close(sem);
    sem_unlink(SEM_NAME);

    return 0;
}