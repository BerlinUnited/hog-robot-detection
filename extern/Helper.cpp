#include "Helper.h"

double Helper::threshold = 0;

Helper::Helper()
{
	fWinScaleWidth = fWinScaleHeight = fWinMainScale = 1;
	lWinWidthOld = lWinWidth = SCREEN_WIDTH;
	lWinHeightOld = lWinHeight = SCREEN_HEIGHT;

	strLogFileName = "LOGFILE.txt";
}

Helper::Helper(string strLogFileName, bool bAppend)
{
	fWinScaleWidth = fWinScaleHeight = fWinMainScale = 1;
	lWinWidthOld = lWinWidth = SCREEN_WIDTH;
	lWinHeightOld = lWinHeight = SCREEN_HEIGHT;

	this->strLogFileName = strLogFileName;
	std::ofstream fLog;
	if (!bAppend)
	{
		fLog.open(strLogFileName, ios::trunc);
		fLog << "### DLIB HOG - LOGFILE ###\n\n";
		fLog << "Starting logging at: " << getDateAndTimeString() << "\n";
		fLog.close();
	}
	//else
	//fLog.open(strLogFileName, ios::app);
}
Helper::~Helper()
{
}

bool Helper::fileExists(const std::string& name) {
	ifstream f(name.c_str());
	return f.good();
}

string Helper::getDateAndTimeString()
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::stringstream ss;
	ss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
	return ss.str();
}

bool Helper::containsAnyBoxes(const std::vector<std::vector<dlib::rectangle>>& boxes)
{
	bool bReturn = false;

	for (unsigned long i = 0; i < boxes.size(); ++i)
	{
		if (boxes[i].size() != 0){
			bReturn = true;
			std::cout << "Removed " << boxes[i].size() << " unobtainable boxes from image index: " << i << "\n";
		}
	}
	return bReturn;
}

void Helper::pickBestHogWindowSize(const std::vector<std::vector<dlib::rectangle> >& boxes, unsigned long& width, unsigned long& height, const unsigned long target_size)
{
	// find the average width and height
	dlib::running_stats<double> avg_width, avg_height;
	for (unsigned long i = 0; i < boxes.size(); ++i)
	{
		for (unsigned long j = 0; j < boxes[i].size(); ++j)
		{
			avg_width.add(boxes[i][j].width());
			avg_height.add(boxes[i][j].height());
		}
	}

	// now adjust the box size so that it is about target_pixels pixels in size
	double size = avg_width.mean()*avg_height.mean();
	double scale = std::sqrt(target_size / size);

	width = (unsigned long)(avg_width.mean()*scale + 0.5);
	height = (unsigned long)(avg_height.mean()*scale + 0.5);
	// make sure the width and height never round to zero.
	if (width == 0)
		width = 1;
	if (height == 0)
		height = 1;

	std::cout << "Detected (and used) an ideal HOG:\n  Sliding Window: " << width << "x" << height << " size\n";
}

void Helper::copyImage(dlib::array2d<unsigned char>& imageIn, dlib::array2d<unsigned char>& imageOut)
{
	imageOut.set_size(imageIn.nr(), imageIn.nc());
	resize_image(imageIn, imageOut);
}

void Helper::copyImage(dlib::matrix<unsigned char>& imageIn, dlib::array2d<unsigned char>& imageOut)
{
	imageOut.set_size(imageIn.nr(), imageIn.nc());
	resize_image(imageIn, imageOut);
}


void Helper::scaleDetections(std::vector<dlib::rectangle>& detections, float fScale)
{
	for (unsigned int j = 0; j < detections.size(); j++)
	{
		detections[j].set_left(detections[j].left() * (fScale+0.1111)); // Hotfix
		detections[j].set_right(detections[j].right() * (fScale + 0.1111)); // Hotfix
		detections[j].set_bottom(detections[j].bottom() * fScale);
		detections[j].set_top(detections[j].top() * fScale);
	}
}

void Helper::prepareToWindowScaling(dlib::image_window& win, const dlib::array2d<unsigned char>& imageToScale)
{
	win.get_size(lWinWidth, lWinHeight);
	lWinWidthOld = lWinWidth; 
	lWinHeightOld = lWinHeight;

	fWinScaleHeight = static_cast<float> (lWinHeight) / static_cast<float> (imageToScale.nr());
	fWinScaleWidth = static_cast<float> (lWinWidth) / static_cast<float> (imageToScale.nc());
	if (fWinScaleWidth > fWinScaleHeight)
		lWinWidth = imageToScale.nc() * fWinScaleHeight;
	else
		lWinHeight = imageToScale.nr() * fWinScaleWidth;

	fWinMainScale = fWinScaleWidth;
	if (fWinScaleWidth > fWinScaleHeight)
		fWinMainScale = fWinScaleHeight;

	/*win.get_size(lW, lH);
	lW_old = lW; lH_old = lH;
	//win.get_display_size(lWD, lHD);

	fWinScaleH = static_cast<float> (lH) / static_cast<float> (images_test_test[i].nr());
	fWinScaleW = static_cast<float> (lW) / static_cast<float> (images_test_test[i].nc());
	if (fWinScaleW> fWinScaleH)
		lW = images_test_test[i].nc() * fWinScaleH;
	else
		lH = images_test_test[i].nr() * fWinScaleW; */
}

void Helper::scaleImageToWindow(const dlib::array2d<unsigned char>& imgageIn, dlib::array2d<unsigned char>& imageOut)
{
	imageOut.set_size(lWinHeight, lWinWidth);
	resize_image(imgageIn, imageOut);
}

void Helper::scaleDetectionsToWindow(std::vector<dlib::rectangle>& detections)
{
	for (unsigned int j = 0; j < detections.size(); j++)
	{
		detections[j].set_left(detections[j].left() * fWinMainScale);
		detections[j].set_right(detections[j].right() * fWinMainScale);
		detections[j].set_bottom(detections[j].bottom() * fWinMainScale);
		detections[j].set_top(detections[j].top() * fWinMainScale);
	}
}

void Helper::completeToWindowScaling(dlib::image_window& win)
{
	win.set_size(lWinWidthOld, lWinHeightOld);
}

// Logging
bool Helper::Log(std::string WriteToLog)
{
	std::ofstream fLog;
	fLog.open(strLogFileName, ios::app);
	fLog << WriteToLog;
	fLog.close();
	return true;
}

bool Helper::Log(float WriteToLog)
{
	std::ofstream fLog;
	fLog.open(strLogFileName, ios::app);
	fLog << WriteToLog;
	fLog.close();
	return true;
}

bool Helper::Log(int WriteToLog)
{
	std::ofstream fLog; 
	fLog.open(strLogFileName, ios::app);
	fLog << WriteToLog;
	fLog.close();
	return true;
}

// Adjust threshold
void Helper::adjustThreshold(char c)
{
	if (c == '+')
	{
		Helper::threshold += 0.25;
		cout << "Threshold increased by 0.25 to " << Helper::threshold << "\n";
	}
	if (c == '-')
	{
		Helper::threshold -= 0.25;
		cout << "Threshold decreased by 0.25 to " << Helper::threshold << "\n";
	}
}


// Converts
std::string Helper::matToString(dlib::matrix<double, 1, 3> mat)
{
	std::stringstream ss;
	ss << mat;
	return ss.str();
}

std::vector<dlib::rectangle> Helper::pairsToRectangles(std::vector < std::pair<double, dlib::rectangle >> detectionpairs)
{
	std::vector<dlib::rectangle> dets;
	for (unsigned int i = 0; i < detectionpairs.size(); i++)
		dets.push_back(detectionpairs[i].second);
	return dets;
}

bool Helper::matIs100(dlib::matrix<double, 1, 3> mat)
{
	return mat(0, 0) == 1 && mat(0, 1) == 0 && mat(0, 2) == 0;
}
