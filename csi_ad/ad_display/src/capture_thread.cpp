/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file capture_thread.cpp
 *
 * @brief capture_thread module: Realize capture AD data by spi or csi interface
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2023-5-10
 **/

#include <QDebug>

#include "capture_thread.h"

/* Ring buffer num */
#define RING_BUFFER_NUM  400

RingBuffer *g_ring_buf = NULL;

CaptureThread::CaptureThread(CaptureParams *params, QObject *parent) : QThread(parent) {
    /* Init ring buffer */
    g_ring_buf = ring_buffer_init(params->ad76x6_params.size, RING_BUFFER_NUM);
    if (!g_ring_buf) {
        printf("Ring buffer init failed!\n");
        exit(-1);
    }

    /* Init ad76x6 interface */
    m_ad76x6 = ad76x6_create(params->dev_name, params->iface, &params->ad76x6_params);
    if (!m_ad76x6) {
        printf("ad76x6 create failed!\n");
        exit(-1);
    }

    m_iface = params->iface;
}

CaptureThread::~CaptureThread() {
    ring_buffer_destory(g_ring_buf);
    ad76x6_destroy(m_ad76x6);
}

void CaptureThread::run() {
    int ret, status = 0;

    if (m_iface == IF_CSI) {
        /* AD capture */
        status = ad76x6_capture_start(m_ad76x6);
        if (status == -1) {
            printf("ad76x6 capture error\n");
            quit();
        }

        while (!this->isInterruptionRequested()) {
            /* Get one frame */
            if (!ad76x6_get_buffer(m_ad76x6))
                continue;

            /* Put one frame into ring buffer */
            ret = ring_buffer_put(g_ring_buf, (void *)m_ad76x6->data);
            if (ret) {
                emit sig_data();
            } else {
                printf("ring buffer is full\n");
            }

            ad76x6_put_buffer(m_ad76x6);
        }

        ad76x6_capture_stop(m_ad76x6);
    } else if (m_iface == IF_SPI) {
        while (!this->isInterruptionRequested()) {
            /* Get buffer from spi */
            if (!ad76x6_get_buffer(m_ad76x6))
                continue;

            /* Put data into ring buffer */
            ret = ring_buffer_put(g_ring_buf, (void *)m_ad76x6->data);
            if (ret) {
                emit sig_data();
            } else {
                printf("ring buffer is full\n");
            }
        }
    }

    quit();
}
