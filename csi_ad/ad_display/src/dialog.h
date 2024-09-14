#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QDebug>
#include <QScreen>
#include <QVector>
#include <QThread>
#include <QGuiApplication>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_math.h>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include <termios.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/types.h>
}

#include "capture_thread.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(char *dev_name, uint32_t channel, enum InterfaceType interface,
                    uint32_t sample_num, struct Ad76x6Params *ad76x6_params, QWidget *parent = 0);
    ~Dialog();

    /* Unix signal handler */
    static void handle_signal(int sig);

public:
    QVector<QwtPlotCurve *> m_curve;
    QVector<QPolygonF *> data;
    CaptureThread *m_ad76x6Thread;
    CaptureParams *capture_params;

private slots:
    void update_data();

private:
    void qwt_init(uint32_t channels);
    void qwt_clean();

private:
    Ui::Dialog *ui;

    int16_t *voltage_data;
    uint32_t channels;
    uint32_t sample_num;
};

#endif // DIALOG_H
