#!/bin/bash

# 定义本地目录和NAS挂载目录
local_dir="/home/camera/ssd/imgs/"
nas_dir="/home/camera/Nas/LzTeam/HST"
log_dir="/home/camera/ssd/logdir"
upload_logidr="/home/camera/ssd/upload_logdir/"
current_date=$(date +'%Y%m%d')

# 设置上次间隔时间
upload_wait_time=65
# 设置检查NAS是否挂载时间
check_Nas_time=30

check_dir()
{
    # Check if log directory exists, if not, create it
    if [ ! -d "$log_dir" ]; then
        mkdir -p "$log_dir"
    fi

    log_file="${log_dir}/${current_date}.log"
    # Check if upload log exists, if not, create it
    if [ ! -f "$log_file" ]; then
        touch "$log_file"
    fi

}

check_Nas()
{
    while [ ! -d "$nas_dir" ]; do
    echo "Nas mounting failed !!!!!" 
    echo "Nas mounting failed !!!!!" >>$log_file
    echo "Start the automatic mounting service"
    sudo systemctl start Nas_mount.service
    sleep $check_Nas_time
    done
    echo "Nas mounted successfully !!!!!"
    echo "Nas mounted successfully !!!!!" >>$log_file

}


upload_file_name=""
# 获取24小时内的可上传目录
get_upload_directories() {
    upload_directories=()
    for file in $(find "$upload_logidr" -maxdepth 1 -maxdepth 1 -type f -name "*.log" -mtime -2); do
        # upload_file_name=$file
        while IFS= read -r line; do
            upload_directories+=("$line")
        done < "$file"
    done
}

# 上传目录到NAS函数
upload_to_nas() {
    check_Nas
    # directory="\$1"
    # timestamp=$(date +"%Y%m%d%H%M%S")
    # new_name="${timestamp}_$(basename "$1")"
    basename=$(basename "$1")
    # 检查目录是否已经存在于NAS中
    if [ ! -e "$nas_dir/$basename" ]; then
        # 等待1分钟以确保目录不在被写入
        # sleep 3

        # 复制目录至NAS
        cp -r "$1" "$nas_dir/"

        # -a：归档模式，保留权限、链接、文件属性等
        # -v：详细模式，输出详细的处理信息
        # -z：压缩数据传输
        # --bwlimit=1000：限制带宽到1000KB/s
        # rsync -r --bwlimit=50000 "$1" "$nas_dir/"
        # 写入日志文件
        echo "Uploaded directory $basename to NAS." >> "$log_file"
        echo "Uploaded directory $basename to NAS."
    else
        echo "Directory $basename already exists in NAS, skipping upload." >> "$log_file"
        echo "Directory $basename already exists in NAS, skipping upload."
    fi
}


# 循环执行上传任务
while true; do
    check_dir
    check_Nas
    # 获取24小时内的可上传目录
    get_upload_directories

    
    # 遍历 upload_directories 数组
    # for dir in "${upload_directories[@]}"; do
    #     echo "Checking directory: $dir"
    # done

    # 找到本地目录中新生成的目录，并上传到NAS
    find "$local_dir" -mindepth 1 -maxdepth 1 -type d -mmin -60 -mmin +0 | while read directory; do
        # 检查目录是否在可上传目录列表中
        if [[ " ${upload_directories[@]} " =~ " $(basename "$directory") " ]]; then
            echo "________uploading________" >> $log_file
            echo "________uploading________"
            upload_to_nas "$directory"
        else
            echo "Directory $(basename "$directory") is not in the list of uploadable directories $upload_file_name , skipping upload." >> "$log_file"
            echo "Directory $(basename "$directory") is not in the list of uploadable directories $upload_file_name , skipping upload."
        fi
    done

    echo "sleep $upload_wait_time s...." >> "$log_file"
    echo "sleep $upload_wait_time s...."
    # 设置等待时间
    sleep $upload_wait_time

done
