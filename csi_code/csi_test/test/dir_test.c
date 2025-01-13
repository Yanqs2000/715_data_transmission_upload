#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

int is_directory_empty(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    struct dirent *entry;

    if (dir == NULL) {
        perror("opendir failed");
        return -1; // 目录打开失败
    }

    // 遍历目录中的条目
    while ((entry = readdir(dir)) != NULL) {
        // 忽略当前目录（.）和上一级目录（..）
        if (entry->d_name[0] != '.') {
            closedir(dir);
            return 0; // 目录不为空
        }
    }

    closedir(dir);
    return 1; // 目录为空
}

int main() {
    const char *folder_path = "/mnt/nas"; // 替换为要检查的目录路径

    int result = is_directory_empty(folder_path);
    if (result == 1) {
        printf("The directory is empty.\n");
    } else if (result == 0) {
        printf("The directory is not empty.\n");
    } else {
        printf("An error occurred while checking the directory.\n");
    }

    return 0;
}
