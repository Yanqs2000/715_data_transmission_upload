/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file ad76x6.c
 *
 * @brief ad76x6 module: Realize capture AD data by spi or csi interface
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2023-4-25
 **/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "ad76x6.h"
#include "v4l2_device_control.h"

bool ad76x6_init_spi(Ad76x6 *ad76x6, struct Ad76x6Params *ad76x6_params)
{
    ad76x6->spi = spi_device_init(ad76x6->module.dev_name, &ad76x6_params->sv.spi_params);
    if (ad76x6->spi == NULL) {
        return false;
    }

    return true;
}

bool ad76x6_init_csi(Ad76x6 *ad76x6, struct Ad76x6Params *ad76x6_params)
{
    struct V4lParams *v4l_params;

    v4l_params = &ad76x6_params->sv.v4l_params;

    ad76x6->camera = v4l2_device_open(ad76x6->module.dev_name, v4l_params->type, v4l_params->format,
                                        v4l_params->width, v4l_params->height, v4l_params->fps);

    if (!ad76x6->camera) {
        perror("Open v4l2 device.");
        return false;
    }

    if (!v4l2_device_setup(ad76x6->camera)) {
        perror("Setup v4l2 device.");
        v4l2_device_close(ad76x6->camera);

        return false;
    }

    return true;
}

Ad76x6 *ad76x6_create(const char *dev_name, int interface, struct Ad76x6Params *ad76x6_params)
{
    Ad76x6 *ad76x6 = NULL;

    ad76x6 = (Ad76x6 *)malloc(sizeof(Ad76x6));
    memset(ad76x6, 0, sizeof(Ad76x6));

    strcpy(ad76x6->module.dev_name, dev_name);
    ad76x6->module.interface    = interface;
    ad76x6->camera              = NULL;

    if (ad76x6->module.interface == IF_CSI) {
        if (!ad76x6_init_csi(ad76x6, ad76x6_params)) {
            goto error;
        }
    } else if (ad76x6->module.interface == IF_SPI) {
        if (!ad76x6_init_spi(ad76x6, ad76x6_params)) {
            goto error;
        }
    } else {
        return NULL;
    }

    return ad76x6;

error:
    if (ad76x6 != NULL) {
        free(ad76x6);
        ad76x6 = NULL;
    }

    return NULL;
}

bool ad76x6_capture_start(Ad76x6 *ad76x6)
{
    bool ret = false;

    if (ad76x6 == NULL) return false;

    if (ad76x6->module.interface == IF_CSI) {
        ret = v4l2_device_stream_on(ad76x6->camera);
    } else {
        return false;
    }

    return ret;
}

bool ad76x6_capture_stop(Ad76x6 *ad76x6)
{
    if (ad76x6 == NULL) return false;

    return v4l2_device_stream_off(ad76x6->camera);
}

bool ad76x6_get_buffer(Ad76x6 *ad76x6)
{
    bool ret = false;

    if (ad76x6 == NULL) return false;

    if (ad76x6->module.interface == IF_CSI) {
        ret = v4l2_device_get_buffer(ad76x6->camera, 3);
        if (!ret) {
            return false;
        }

        ad76x6->data = (int16_t *)ad76x6->camera->data;
    } else if (ad76x6->module.interface == IF_SPI) {
        ret = spi_device_transfer(ad76x6->spi);
        if (!ret) {
            return false;
        }

        ad76x6->data = (int16_t *)ad76x6->spi->buf;
    }

    return true;
}

bool ad76x6_put_buffer(Ad76x6 *ad76x6)
{
    return v4l2_device_put_buffer(ad76x6->camera);
}

void ad76x6_destroy(Ad76x6 *ad76x6)
{
    if (ad76x6 == NULL) {
        return;
    }

    if (ad76x6->camera != NULL) {
        v4l2_device_close(ad76x6->camera);
        ad76x6->camera = NULL;
    }

    if (ad76x6->spi != NULL) {
        spi_device_destory(ad76x6->spi);
        ad76x6->spi = NULL;
    }

    free(ad76x6);
    ad76x6 = NULL;
}
