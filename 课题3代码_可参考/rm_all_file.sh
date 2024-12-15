#!/bin/bash

# 要删除的文件目录列表
directories=(
    "/home/camera/ssd/grab_logdir"
    "/home/camera/ssd/logdir"
    "/home/camera/ssd/upload_logdir"
    "/home/camera/ssd/glog_file"
)

# 遍历每个目录并删除其中的所有文件
for dir in "${directories[@]}"; do
    echo "Deleting files in $dir"
    rm -rf "$dir"/*
done

echo "All specified files have been deleted."
