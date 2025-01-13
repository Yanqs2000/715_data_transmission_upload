#ifndef CAPTURE_THREAD_H
#define CAPTURE_THREAD_H

#include <QThread>
#include <QVector>
#include <QList>
#include <QMetaType>

#include "ad76x6.h"
#include "ring_buffer.h"

typedef struct _CaptureParams {
    char dev_name[32];
    enum InterfaceType iface;
    int size;
    struct Ad76x6Params ad76x6_params;
} CaptureParams;

class CaptureThread : public QThread
{
    Q_OBJECT
public:
    CaptureThread(CaptureParams *params, QObject *parent = nullptr);
    ~CaptureThread();

protected:
    void run();

signals:
    void sig_data();

private:
    Ad76x6 *m_ad76x6;
    enum InterfaceType m_iface;
};

#endif // EVENT_THREAD_H
