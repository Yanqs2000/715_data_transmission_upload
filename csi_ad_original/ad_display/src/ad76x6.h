#ifndef AD76X6_H
#define AD76X6_H

#include "v4l2_device_control.h"
#include "spi_device_control.h"

#if defined (__cplusplus)
extern "C" {
#endif

enum InterfaceType {
    IF_INVALID  = 0,
    IF_CSI      = 1,
    IF_SPI      = 2
};

struct V4lParams {
    char        type[16];
    char        format[16];
    uint32_t    width;
    uint32_t    height;
    uint32_t    fps;
};

struct Ad76x6Device {
    char        dev_name[32];
    uint32_t    interface;
};

typedef struct _Ad76x6 {
    int16_t             *data;
    struct Ad76x6Device module;
    V4l2Device          *camera;
    struct SpiDevice    *spi;
} Ad76x6;

struct Ad76x6Params {
    int size;
    union {
        struct SpiParams spi_params;
        struct V4lParams v4l_params;
    } sv;
};

Ad76x6 *ad76x6_create(const char *dev_name, int interface, struct Ad76x6Params *ad76x6_params);
bool ad76x6_capture_start(Ad76x6 *ad76x6);
bool ad76x6_capture_stop(Ad76x6 *ad76x6);
bool ad76x6_get_buffer(Ad76x6 *ad76x6);
bool ad76x6_put_buffer(Ad76x6 *ad76x6);
void ad76x6_destroy(Ad76x6 *ad76x6);

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */

#endif /* AD76X6_H */
