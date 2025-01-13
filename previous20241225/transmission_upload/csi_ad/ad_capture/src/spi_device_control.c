/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file spi_device_control.c
 *
 * @brief spi_device_control module: Capture ad data by spi bus
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2023-7-5
 **/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>

#include "spi_device_control.h"

extern char interrupt_event[32];

struct SpiDevice* spi_device_init(const char *dev_name, const struct SpiParams *spi_params)
{
    int ret = -1;
    struct SpiDevice *spi_device;

    spi_device = malloc(sizeof(struct SpiDevice));
    if (spi_device == NULL) {
        perror("malloc");
        return NULL;
    }

    memset(spi_device, 0, sizeof(struct SpiDevice));
    memset(&spi_device->params, 0, sizeof(struct SpiParams));
    memcpy(&spi_device->params, spi_params, sizeof(struct SpiParams));

    /* Open event device */
    spi_device->event_dev = open(interrupt_event, O_RDONLY);
    if (spi_device->event_dev < 0) {
        perror("open event device");
        goto error;
    }

    /* Open spi device */
    spi_device->spi_dev = open(dev_name, O_RDWR);
    if (spi_device->spi_dev < 0) {
        perror("open spi device");
        goto error;
    }

    /* Allocate spi rx buffer */
    spi_device->buf = malloc(spi_params->size);
    if (spi_device->buf == NULL) {
        perror("malloc");
        goto error;
    }

    /* Set spi mode */
    ret = ioctl(spi_device->spi_dev, SPI_IOC_WR_MODE32, &spi_params->mode);
    if (ret == -1) {
        perror("SPI_IOC_WR_MODE32");
        goto error;
    }

    /* Set bits per word */
    ret = ioctl(spi_device->spi_dev, SPI_IOC_WR_BITS_PER_WORD, &spi_params->bits_per_word);
    if (ret == -1) {
        perror("SPI_IOC_WR_BITS_PER_WORD");
        goto error;
    }

    /* Set max speed (hz) */
    ret = ioctl(spi_device->spi_dev, SPI_IOC_WR_MAX_SPEED_HZ, &spi_params->speed);
    if (ret == -1) {
        perror("SPI_IOC_WR_MAX_SPEED_HZ");
        goto error;
    }

    return spi_device;

error:
    spi_device_destory(spi_device);

    return NULL;
}

void spi_device_destory(struct SpiDevice *spi_device)
{
    if (!spi_device) {
        return;
    }

    if (spi_device->buf != NULL) {
        free(spi_device->buf);
        spi_device->buf = NULL;
    }

    if (spi_device->event_dev != -1) {
        close(spi_device->event_dev);
        spi_device->event_dev = -1;
    }

    if (spi_device->spi_dev != -1) {
        close(spi_device->spi_dev);
        spi_device->spi_dev = -1;
    }

    free(spi_device);
    spi_device = NULL;
}

bool spi_device_transfer(struct SpiDevice *spi_device)
{
    fd_set read_set;
    struct timeval timeout;
    struct input_event event_buf;
    int ret = -1;

    timeout.tv_sec  = 2;
    timeout.tv_usec = 0;

    FD_ZERO(&read_set);
    FD_SET(spi_device->event_dev, &read_set);

    ret = select(spi_device->event_dev + 1, &read_set, NULL, NULL, &timeout);
    if (ret < 0) {
        perror("select");
        return false;
    } else if (ret == 0) {
        printf("Capture ad timeout.\n");
        return false;
    }

    if (read(spi_device->event_dev, &event_buf, sizeof(struct input_event)) < 0) {
        return false;
    }

    /**
     * If it is a synchronization event, return
    */
    if (event_buf.type == EV_SYN) {
        return false;
    }

    if (event_buf.code != KEY_F24) {
        return false;
    }

    if (event_buf.value != 0) {
        return false;
    }

    struct spi_ioc_transfer tr = {
        .tx_buf         = 0,
        .rx_buf         = (unsigned long)spi_device->buf,
        .len            = spi_device->params.size,
        .delay_usecs    = spi_device->params.delay,
        .speed_hz       = spi_device->params.speed,
        .bits_per_word  = spi_device->params.bits_per_word,
    };

    ret = ioctl(spi_device->spi_dev, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0) {
        return false;
    }

    return true;
}
