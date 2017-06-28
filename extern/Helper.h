#pragma once

#include "Global.h"

class Helper
{
public:
	Helper();
	Helper(std::string strLogFileName, bool bAppend);
	~Helper();

	static bool fileExists(const std::string& name);
	static std::string getDateAndTimeString();
	static bool containsAnyBoxes(const std::vector<std::vector<dlib::rectangle>>& boxes);

	/* Finds the average aspect ratio of the elements of boxes and outputs a width and height such that the aspect ratio is equal to the average and also the area is equal to target_size.  That is, the following will be approximately true:
	#width*#height == target_size
	#width/#height == the average aspect ratio of the elements of boxes. */
	static void pickBestHogWindowSize(const std::vector<std::vector<dlib::rectangle>>& boxes,unsigned long& width, unsigned long& height, const unsigned long target_size);
	static void copyImage(dlib::array2d<unsigned char>& imageIn, dlib::array2d<unsigned char>& imageOut);
	static void copyImage(dlib::matrix<unsigned char>& imageIn, dlib::array2d<unsigned char>& imageOut);
	static void scaleDetections(std::vector<dlib::rectangle>& detections, float fScale);

	void prepareToWindowScaling(dlib::image_window& win, const dlib::array2d<unsigned char>& imageToScale);
	void scaleImageToWindow(const dlib::array2d<unsigned char>& imgageIn, dlib::array2d<unsigned char>& imageOut);
	void scaleDetectionsToWindow(std::vector<dlib::rectangle>& detections);
	void completeToWindowScaling(dlib::image_window& win);

	// Logging
	bool Log(std::string WriteToLog);
	bool Log(float WriteToLog);
	bool Log(int WriteToLog);

	// Convert
	static std::string matToString(dlib::matrix<double, 1, 3> mat);
	static std::vector<dlib::rectangle> pairsToRectangles(std::vector<std::pair<double, dlib::rectangle>> detectionpairs);
	static bool matIs100(dlib::matrix<double, 1, 3> mat);

	// Adjust threshold
	static void adjustThreshold(char c);
	static double threshold;

private:
	float fWinScaleWidth, fWinScaleHeight, fWinMainScale;
	unsigned long lWinWidthOld, lWinHeightOld, lWinWidth, lWinHeight;
	std::string strLogFileName;
};

