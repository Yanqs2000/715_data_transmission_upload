/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file dialog.cpp
 *
 * @brief dialog module: Draw the AD curve.
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2022-8-22
 **/

#include "dialog.h"
#include "ui_dialog.h"

#define SAMPLES_PER_CYCLE 1024

extern RingBuffer *g_ring_buf;

Dialog::Dialog(char *dev_name, uint32_t channel, enum InterfaceType interface,
               uint32_t sample_num, struct Ad76x6Params *ad76x6_params, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    this->setWindowState(Qt::WindowFullScreen);

    /* Get display device num */
    QList<QScreen *> list_screen = QGuiApplication::screens();
    this->resize(list_screen.at(0)->geometry().width(), list_screen.at(0)->geometry().height());

    ui->qwtPlot->setParent(this);
    ui->qwtPlot->resize(this->width(), this->height());

    /* Init qwt */
    qwt_init(channel);

    this->channels = channel;
    this->sample_num = sample_num;
    voltage_data = (int16_t *)calloc(1, ad76x6_params->size);

    /* Init ad76x6 capture */
    capture_params = (CaptureParams *)calloc(1, sizeof(CaptureParams));
    strcpy(capture_params->dev_name, dev_name);
    memcpy(&capture_params->ad76x6_params, ad76x6_params, sizeof(struct Ad76x6Params));
    capture_params->iface = interface;

    /* Start capture thread */
    m_ad76x6Thread = new CaptureThread(capture_params, this);
    connect(m_ad76x6Thread, &CaptureThread::sig_data, this, &Dialog::update_data, Qt::QueuedConnection);
    m_ad76x6Thread->start();
}

Dialog::~Dialog() {
    disconnect(this);
    if(m_ad76x6Thread->isRunning()) {
        /* Stop thread */
        disconnect(m_ad76x6Thread);
        m_ad76x6Thread->requestInterruption();
        m_ad76x6Thread->wait();
        delete m_ad76x6Thread;
    }

    qwt_clean();
    free(capture_params);
    free(voltage_data);

    delete ui;
}

void Dialog::handle_signal(int) {
    qApp->quit();
}

void Dialog::qwt_init(uint32_t channels) {
    uint32_t i;

    /* qwtplot init */
    ui->qwtPlot->setAutoReplot(true);
    ui->qwtPlot->setAxisScale(QwtPlot::yLeft, -8, 8);
    ui->qwtPlot->setAxisScale(QwtPlot::xBottom, 0, SAMPLES_PER_CYCLE);

    Qt::GlobalColor color[] = {Qt::black, Qt::black, Qt::blue, Qt::red,
                               Qt::red, Qt::green, Qt::black, Qt::cyan,
                               Qt::magenta, Qt::yellow, Qt::darkRed, Qt::darkGreen,
                               Qt::darkBlue, Qt::darkCyan, Qt::darkMagenta, Qt::darkYellow};

    for (i = 0; i < channels; i++) {
        QwtPlotCurve *curve = new QwtPlotCurve;
        QPolygonF *cdata = new QPolygonF;
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);
        curve->setPen(QPen(color[i]));
        curve->attach(ui->qwtPlot);

        this->m_curve.append(curve);
        this->data.append(cdata);
    }

    ui->qwtPlot->replot();
}

void Dialog::qwt_clean() {
    uint32_t i;
    for (i = 0; i < this->channels; i++) {
        delete this->m_curve[i];
        delete this->data[i];
    }
}

void Dialog::update_data() {
    uint32_t i, j, k;

    ring_buffer_get(g_ring_buf, (void *)voltage_data);

    for (k = 0; k < this->channels; k++)
        this->data[k]->clear();

    for (i = 0, j = 0; j < this->sample_num; i += this->channels, j++) {
        for (k = 0; k < this->channels; k++) {
            *this->data[k] << QPointF(j, (float)voltage_data[i + k] * 10 / 65535);
        }
    }

    for (k = 0; k < this->channels; k++)
        this->m_curve[k]->setSamples(*this->data[k]);
}
