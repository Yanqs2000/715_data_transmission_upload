#pragma once

#include "Sync.h"
#include "Thread_ReadUSB.h"
#include "DataExtractor.h"
#include "config.h"
#include "camera.h"
#include "stream_compress.h"
#include "gloghelper.h"
#include "bits/stdc++.h"
std::string GetTimeStamp();

//grab image data 
int auto_grab(Parameter& params, time_t loop_start_time, int stopgrab_time);

//Get timestamp
std::string get_date_string();

void signalHandler(int signum);

int save_inctime(std::string root_path);