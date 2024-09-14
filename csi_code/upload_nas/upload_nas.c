#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>

#define SOURCE_DIR "/mnt/data/frames"
#define DEST_DIR "/mnt/nas/copy"
#define BUFFER_SIZE 1024

void copy_file(const char *source_path, const char *dest_path) {
    FILE *source_file = fopen(source_path, "rb");
    if (source_file == NULL) {
        perror("Failed to open source file");
        exit(EXIT_FAILURE);
    }

    FILE *dest_file = fopen(dest_path, "wb");
    if (dest_file == NULL) {
        perror("Failed to open destination file");
        fclose(source_file);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, BUFFER_SIZE, source_file)) > 0) {
        fwrite(buffer, 1, bytes, dest_file);
    }

    fclose(source_file);
    fclose(dest_file);
}

void copy_directory(const char *source_dir, const char *dest_dir) {
    DIR *dir = opendir(source_dir);
    if (dir == NULL) 
    {
        perror("Failed to open source directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (entry->d_type == DT_REG) { // If the entry is a regular file
            char source_path[PATH_MAX];
            char dest_path[PATH_MAX];

            snprintf(source_path, sizeof(source_path), "%s/%s", source_dir, entry->d_name);
            snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

            copy_file(source_path, dest_path);
        }
    }

    closedir(dir);
}

int main() {
    struct stat st = {0};
    struct timeval time_start, time_end, time_diff;//time_start、time_end 和 time_diff 用于计算程序运行时间等。
    uint32_t cost_time;
    float average_rate;

    // Create destination directory if it doesn't exist
    if (stat(DEST_DIR, &st) == -1) 
    {
        if (mkdir(DEST_DIR, 0777) != 0) 
        {
            perror("Failed to create destination directory");
            exit(EXIT_FAILURE);
        }
    }

    //使用 gettimeofday 函数获取当前时间作为程序开始时间
    gettimeofday(&time_start, NULL);

    copy_directory(SOURCE_DIR, DEST_DIR);

    printf("All files copied successfully.\n");

    /* 计算程序时间和数据流 */
    gettimeofday(&time_end, NULL);
    timersub(&time_end, &time_start, &time_diff);
    cost_time = time_diff.tv_sec * 1000.0 + time_diff.tv_usec / 1000;
    average_rate = 10 * 1024 / (cost_time / 1000.0);
    printf("time: %d ms, rate: %g MB/s\n", cost_time, average_rate);

    return 0;
}


