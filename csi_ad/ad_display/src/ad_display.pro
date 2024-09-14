#-------------------------------------------------
#
# Project created by QtCreator 2023-05-08T14:12:30
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets svg opengl printsupport concurrent

greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++11
lessThan(QT_MAJOR_VERSION, 5): QMAKE_CXXFLAGS += -std=c++11

TARGET = ad_display
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        main.cpp \
        dialog.cpp \
        capture_thread.cpp \
        ad76x6.c \
        ring_buffer.c \
        v4l2_device_control.c \
        spi_device_control.c

HEADERS += \
        dialog.h \
        ad76x6.h \
        capture_thread.h \
        ring_buffer.h \
        v4l2_device_control.h \
        spi_device_control.h

FORMS    += dialog.ui

QWT_INSTALL_DIR       =   /home/tl/T3_A40i_Workspack/qwt/install
INCLUDEPATH          +=   $$QWT_INSTALL_DIR/include
LIBS                 +=   -L"$$QWT_INSTALL_DIR/lib" -lqwt
