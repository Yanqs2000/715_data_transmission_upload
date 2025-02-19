# 715_data_transmission_upload
715 Linux 嵌入式 c/c++语言 数据传输与上传方案

## 1.连接
### 1.1 远程连接（包括AUV版本和普通远程连接）
远程连接的目的在于能够通过ssh连接到板子，并启动板子上的程序。

### 1.2 使用软件
Xshell（远程连接的终端）、Xftp（远程连接的文件传输）--两者配套使用
软件下载网址：https://www.xshell.com/zh/xshell/
软件下载网址：https://www.xshell.com/zh/xftp/

### 1.3 远程连接步骤

#### 1.3.1 Xshell连接（AUV版本）
- 首先使用Xshell等软件进行远程连接，先新建一个会话
- 然后配置远程主机
名称可以自己定义，例如：715_远程AUV；协议选SSH；主机地址：47.104.21.115；端口：6029
如图所示：
![alt text](远程AUV_配置主机.png)
- 接着配置用户密码
用户名：root；密码：123
![alt text](远程AUV_配置用户密码.png)

#### 1.3.2 Xshell连接（普通远程连接）
- 首先使用Xshell等软件进行远程连接，先新建一个会话
- 然后配置远程主机
名称可以自己定义，例如：715_测试；协议选SSH；主机地址：192.168.1.116(以路由器分配的地址为准)；端口：22
如图所示：
![alt text](配置主机.png)
- 接着配置用户密码
用户名：root；密码：123
![alt text](配置用户密码.png)


### 1.4 Xftp连接
由于xshell和xftp是配套使用的，在完成连接xshell后，可一键直接连接xftp，如图所示：
![alt text](一键连接xftp.png)

#### 1.4.1 Xftp单独连接
- 首先使用Xftp等软件进行远程单独连接，先新建一个会话
- 然后配置远程主机
名称可以自己定义，例如：715测试；协议选SFTP；主机地址：192.168.1.116(以路由器分配的地址为准)；端口：22
用户名：root；密码：123
如图所示：
！[alt text](xftp单独连接.png)

### 1.5 nas自动挂载
系统启动后会执行nas自动挂载服务，若未接nas或挂载失败，服务会在20s后自动取消。
成功则如图所示：
![alt text](nas挂载成功.png)

## 2. 声学程序使用方法简要介绍
### 2.1 csi代码位置
代码路径位置: /mnt/data/csi_code/csi_test/nas_GuanDao_csi 文件夹
主程序: csi_test.c
编译命令: make  注: 只需cd到nas_GuanDao_csi目录，然后在命令行中输入make即可
启动命令: ./csi_test

### 2.2 配置文件
配置文件路径位置: /mnt/data/csi_code/csi_test/nas_GuanDao_csi/config.ini
配置文件内容:
```
; This is a configuration for csi
[parameters]
device = /dev/video0
format = BGGR
width = 1024
height = 512
number = 2100
ONE_FILE_FRAMES = 2048
data_folder = /mnt/data/csi_code/FPGA_data/
nas_folder = /mnt/nas/SX/FPGA_data/
if_nas = false
if_GuanDao = false
if_delete_start_command = false
if_CLY_process_communication = false
```
其中device、format、宽度、高度无需更改;
number为采集帧数,若要长时间运行，请将值设的大一些;
ONE_FILE_FRAMES为每个文件包含的帧数，2048表示每个文件包含2048帧数据，约为1GB，写满后会新建新的文件;
data_folder为硬盘中存储数据的文件夹路径;
nas_folder为nas中存储数据的文件夹路径,请首先将nas挂载到系统;
if_nas为是否需要上传到nas，true表示需要上传到nas，false不进行上传;
if_GuanDao为是否有惯导数据输入，true表示有惯导数据输入，false没有惯导数据输入;
if_delete_start_command为是否需要删除起始命令，true表示需要删除起始命令，那么下一次则需要重新通过网络接收起始命令;
目前使用false，不进行删除，起始命令保存在start_command文件中，只需修改该文件就能配置;
if_CLY_process_communication = false为进程间通信，旨在将惯导数据进行共享，目前（20250120）还未调通惯导，保持false。

### 2.3 上位机程序(发送起始命令，目前未使用)
路径位置: /mnt/data/csi_code/csi_test/send/send_inital_command 文件夹
主程序: send_inital_command.c
编译命令: gcc -o send_initial_command send_initial_command.c
上位机程序通过UDP协议进行起始命令的发送，由于上位机程序和FPGA数据接收程序在同一块板子中，所以上位机使用
127.0.0.1作为ip地址，端口号为8082，若要更改，请修改send_inital_command.c文件中的宏定义
#define PORT 8082
#define IP_ADDRESS "127.0.0.1"  // IP 地址
启动命令: ./send_initial_command

### 2.4 两个程序(声和上位机)使用流程
1. 开启两个终端，第一个首先启动声学程序，cd到该位置: /mnt/data/csi_code/csi_test/nas_GuanDao_csi
   启动命令为./csi_test
2. 终端会显示wait starting的字样，等待起始命令的发送
3. 接着第二个终端启动上位机程序，cd到该位置: /mnt/data/csi_code/csi_test/send/send_inital_command
   启动命令为./send_initial_command
4. 上位机程序会向ARM发送起始命令，起始命令会被保存到start_command，ARM通过串口6转发给FPGA，FPGA开始发送数据
   目前还未做开机自启、异常重启情况等处理，
   每次启动都需要重复1-3的步骤
5. CTRL+c可以提前结束程序，程序会将已经接收到的帧保存在硬盘中

## 3. 磁力仪接收程序使用方法简要介绍
### 3.1 磁力仪接收程序位置
路径位置: /mnt/data/GuanDao_CiLiYi/GuanDao_CiLiYi_code/ 文件夹
主程序: GuanDao_CiLiYi.cpp
编译命令: make
启动命令: ./GuanDao_CiLiYi

### 3.2 配置文件
配置文件路径位置: /mnt/data/GuanDao_CiLiYi/GuanDao_CiLiYi_code/config.ini
配置文件内容:
```
; This is a configuration for CiLiYi_GuanDao
[path] 
data_dir   = /mnt/data/GuanDao_CiLiYi/data/              ; data path
log_dir    = /mnt/data/GuanDao_CiLiYi/logs/              ; log path
nas_dir    = /mnt/nas/GuanDao_CiLiYi/data/               ; data path

[other]
if_nas     = false                                        ; nas
if_GuanDao = false                                        ; GuanDao
work_time  = 15                                          ; work time
```
data_dir为硬盘中存储数据的文件夹路径;
log_dir为硬盘中存储该程序日志文件的文件夹路径;
nas_dir为nas中存储数据的文件夹路径,请首先将nas挂载到系统;
if_nas为是否需要上传到nas，true表示需要上传到nas，false不进行上传;
if_GuanDao为是否有惯导数据输入，true表示有惯导数据输入，false没有惯导数据输入;
work_time为程序运行时间，单位为秒，时间一到程序会自动停止运行(可设置持续运行，但需要改代码)

### 3.3 磁力仪接收程序使用流程
1. 开启一个终端，启动磁力仪接收程序，启动命令为./GuanDao_CiLiYi
   在磁力仪程序中，还未做上位机网络接收功能，启动之后，便会接收数据
   若无数据输入，程序会返回全0的数据包进行保存。
   还未做开机自启、命令保存、异常重启情况等处理
2. CTRL+c可以提前结束程序，程序会将已经接收到的数据保存在硬盘中





