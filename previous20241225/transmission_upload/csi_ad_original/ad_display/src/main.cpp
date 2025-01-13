/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file mainwindow.cpp
 *
 * @brief Example application main file.
 * Realize the AD data oscillogram.
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2023-5-8
 **/

#include <QApplication>

#include <signal.h>
#include <sys/select.h>
#include <getopt.h>
#include <string.h>

#include "dialog.h"
#include "ad76x6.h"

const char * const VERSION = "1.0";

char interrupt_event[32];

struct CmdLineParams {
    char                  dev_name[32];
    uint32_t              channel;
    uint32_t              sample_num;
    enum InterfaceType    interface;
    struct V4lParams      v4l2_params;
    struct SpiParams      spi_params;
};

/* Short option names */
static const char g_shortopts [] = ":d:e:c:w:h:i:n:s:m:S:b:D:v";

/* Option names */
static const struct option g_longopts [] = {
    { "device",     required_argument,      NULL,       'd' },
    { "channel",    required_argument,      NULL,       'c' },
    { "width",      required_argument,      NULL,       'w' },
    { "height",     required_argument,      NULL,       'h' },
    { "interface",  required_argument,      NULL,       'i' },
    { "size",       required_argument,      NULL,       's' },
    { "mode",       required_argument,      NULL,       'm' },
    { "speed",      required_argument,      NULL,       'S' },
    { "bits",       required_argument,      NULL,       'b' },
    { "delay",      required_argument,      NULL,       'D' },
    { "number",     required_argument,      NULL,       'n' },
    { "event",      no_argument,            NULL,       'e' },
    { "version",    no_argument,            NULL,       'v' },
    { "help",       no_argument,            NULL,        0  },
    { 0, 0, 0, 0 }
};

static void usage(char **argv) {
    fprintf(stdout,
            "Usage: %s [options]\n\n"
            "Options:\n"
            " -d | --device         V4l2 or spi device.\n"
            " -e | --event          interrupt event. \n"
            " -c | --channel        8-channel or 16-channel data input.\n"
            " -w | --width          V4L2 device capture width.\n"
            " -h | --height         V4L2 device capture height.\n"
            " -i | --interface      Interface type. [csi, spi]\n"
            " -s | --size           Data size of one transmission by spi.\n"
            " -m | --mode           Spi mode. [0, 1, 2, 3]\n"
            " -S | --speed          Spi speed.\n"
            " -b | --bits           Bits per word.\n"
            " -D | --delay          Delay (usec).\n"
            " -n | --number         Number of sample points per channel.\n"
            " -v | --version        Display version information.\n"
            " --help                Show help content.\n\n"
            "Example:\n"
            "  # ./%s -i csi -d /dev/video0 -c 8 -w 512 -h 512 -n 16384\n"
            "  # ./%s -i spi -d /dev/spidev0.1 -e /dev/input/event2 -c 8 -S 50000000 -m 3 -b 8 -s 16384 -n 1024\n"
            "", basename(argv[0]),
                basename(argv[0]),
                basename(argv[0]));
}

static bool parse_parameter(struct CmdLineParams *params, int argc, char **argv) {
    int opt;
    memset(params, 0, sizeof(struct CmdLineParams));

    /* Set default value */
    memset(params->v4l2_params.type, 0, sizeof(params->v4l2_params.type));
    strcpy(params->v4l2_params.format, "BGGR");
    params->v4l2_params.width    = 0;
    params->v4l2_params.height   = 0;
    params->v4l2_params.fps      = 0;

    memset(params->dev_name, 0, sizeof(params->dev_name));
    params->channel     = 0;
    params->interface   = IF_INVALID;
    params->sample_num  = 0;

    /* Data size of one transmission by spi */
    params->spi_params.size          = 0;
    params->spi_params.speed         = 0;
    params->spi_params.bits_per_word = 0;
    params->spi_params.mode          = -1;
    params->spi_params.delay         = 0;

    while ((opt = getopt_long(argc, argv, g_shortopts, g_longopts, NULL)) != -1) {
        switch (opt) {
        case 'd':
            strcpy(params->dev_name, optarg);
            break;
        case 'c':
            params->channel = atoi(optarg);
            break;

        case 'w':
            params->v4l2_params.width = atoi(optarg);
            break;

        case 'h':
            params->v4l2_params.height = atoi(optarg);
            break;

        case 'i':
            if (strcmp(optarg, "csi") == 0) {
                params->interface = IF_CSI;
            } else if (strcmp(optarg, "spi") == 0) {
                params->interface = IF_SPI;
            }
            break;

        case 's':
            params->spi_params.size = atoi(optarg);
            break;

        case 'S':
            params->spi_params.speed = atoi(optarg);
            break;

        case 'b':
            params->spi_params.bits_per_word = atoi(optarg);
            break;

        case 'D':
            params->spi_params.delay = atoi(optarg);
            break;

        case 'm':
            switch (atoi(optarg)) {
            case 0:
                params->spi_params.mode = SPI_MODE_0;
                break;
            case 1:
                params->spi_params.mode = SPI_MODE_1;
                break;
            case 2:
                params->spi_params.mode = SPI_MODE_2;
                break;
            case 3:
                params->spi_params.mode = SPI_MODE_3;
                break;
            }
            break;

        case 'n':
            params->sample_num = atoi(optarg);
            break;

        case 'e':
            strcpy(interrupt_event, optarg);
            break;

        /* --help */
        case 0:
            usage(argv);
            exit(0);

        case 'v':
            printf("version : %s\n", VERSION);
            exit(0);

        default:
            fprintf(stderr, "Unknown option %c\n", optopt);
            break;
        }
    }

    if (strlen(params->dev_name) == 0 || params->interface == IF_INVALID
        || params->sample_num == 0) {
        return false;
    }

    if (params->interface == IF_CSI) {
        if (params->v4l2_params.width == 0 || params->v4l2_params.height == 0) {
            return false;
        }
    } else if (params->interface == IF_SPI) {
        if (params->spi_params.size == 0 || params->spi_params.speed == 0
            || params->spi_params.mode == -1 || params->spi_params.bits_per_word == 0) {
            return false;
        }
    }

    return true;
}

static void setup_unix_signal_handlers() {
    struct sigaction term;

    term.sa_handler = Dialog::handle_signal;
    sigemptyset(&term.sa_mask);
    term.sa_flags = 0;
    term.sa_flags |= SA_RESTART;

    sigaction(SIGTERM, &term, 0);
    sigaction(SIGINT, &term, 0);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CmdLineParams params;
    struct Ad76x6Params ad76x6_params;

    /* Parse command line */
    if (parse_parameter(&params, argc, argv) == false) {
        printf("Please try --help to see usage.\n");
        exit(1);
    }

    if (params.interface == IF_CSI) {
        memcpy(&ad76x6_params.sv, &params.v4l2_params, sizeof(struct V4lParams));
        ad76x6_params.size = params.v4l2_params.width * params.v4l2_params.height;
    } else if (params.interface == IF_SPI) {
        memcpy(&ad76x6_params.sv, &params.spi_params, sizeof(struct SpiParams));
        ad76x6_params.size = params.spi_params.size;
    }

    Dialog w(params.dev_name, params.channel, params.interface,
             params.sample_num, &ad76x6_params);
    w.show();

    setup_unix_signal_handlers();

    return a.exec();
}
