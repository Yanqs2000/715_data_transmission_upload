#pragma once

#include <vector>
#include<iostream>
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include "config.h"
#include "gloghelper.h"
using namespace Pylon;
using namespace Basler_UniversalCameraParams;

//Initialize camera
std::vector<double> camera_init(Parameter& params);

//Set camera parameters
int camera_set( Pylon::CBaslerUniversalInstantCamera& camera, double FrameRate, double ExTime);

void CreatehdrFile(std::ofstream &hdrFile, const std::string &hdr_filename, const int &sample, const int &cnt);

