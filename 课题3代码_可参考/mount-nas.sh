#!/bin/bash

MOUNT_POINT="/home/camera/Nas"
NAS_SHARE="//192.168.1.100/nvme11-183XXXX7920a"
CREDENTIALS="/etc/cifs-credentials"
OPTIONS="credentials=$CREDENTIALS,uid=1000,gid=1000,sec=ntlmssp"

# wait time
wait_time=10

while true; do
    if mountpoint -q "$MOUNT_POINT"; then
        echo "NAS is already mounted."
        exit 0
    else
        echo "Attempting to mount NAS..."
        sudo mount -t cifs "$NAS_SHARE" "$MOUNT_POINT" -o "$OPTIONS"
        if mountpoint -q "$MOUNT_POINT"; then
            echo "NAS mounted successfully."
            exit 0
        else
            echo "Failed to mount NAS. Retrying in 10 seconds..."
            sleep $wait_time
        fi
    fi
done
