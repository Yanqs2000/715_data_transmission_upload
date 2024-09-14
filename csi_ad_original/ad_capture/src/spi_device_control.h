/* Copyright 2013 Tronlong Elec. Tech. Co. Ltd. All Rights Reserved. */

#ifndef SPI_DEVICE_CONTROL_H
#define SPI_DEVICE_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include <linux/spi/spidev.h>

struct SpiParams {
    uint32_t    size;
    uint32_t    speed;
    uint32_t    mode;
    uint32_t    bits_per_word;
    uint32_t    delay;
};

struct SpiDevice {
    int              spi_dev;
    int              event_dev;
    struct SpiParams params;
    int8_t           *buf;
};

struct SpiDevice* spi_device_init(const char *dev_name, const struct SpiParams *spi_params);
void spi_device_destory(struct SpiDevice *spi_device);
bool spi_device_transfer(struct SpiDevice *spi_device);

#endif
