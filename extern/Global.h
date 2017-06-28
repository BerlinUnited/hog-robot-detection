/*
## Global.h ##########################################################################
A global interface for the whole project, can be used by other classes as a "blackboard"
to savefly pass (persisting) information from one module to another.
It is also used to define global enumarations and includes and such.
######################################################################################
*/

#pragma once
// defines for conditional compiling:

// Force no console window
// #pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )  

// External includes
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <chrono>
#include <algorithm>
#include <random>

#include <dlib/svm_threaded.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_processing.h>
#include <dlib/data_io.h>
#include <dlib/config_reader.h>
#include <dlib/cmd_line_parser.h>
//#include <dlib/opencv.h>
//#include <opencv2/highgui/highgui.hpp>

// internal includes
#include "Helper.h"
#include "Timer.h"
#include "CrossValidation.h"

// Defines, typ definitions, macros, etc
using namespace std;
using namespace dlib;

#ifndef UNICODE  
	typedef std::string String;
#else
	typedef std::wstring String;
#endif

typedef enum eAppState
{
	INIT,
	TRAINING,
	TESTING,
	TRACKING,
	END
};

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;

// Forward declaration of classes
class Helper;

// Gobal instances
