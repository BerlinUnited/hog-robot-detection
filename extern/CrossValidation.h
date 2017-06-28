#pragma once

#include "Global.h"

class CrossValidation
{
public:
	CrossValidation(unsigned long ulWinSizeWidth, unsigned long ulWinSizeHeight, std::string strOutputDirectory);
	~CrossValidation();

	//static bool containsAnyBoxes(const std::vector<std::vector<dlib::rectangle>>& boxes);


	// void prepareToWindowScaling(dlib::image_window& win, const dlib::array2d<unsigned char>& imageToScale);
	void addImages(std::string strPath);
	void addC(float fNewC);
	void addEpsilon(float fNewE);
	void setCs(std::vector<float> vecC);
	void setEpsilons(std::vector<float> vecEpsilon);
	void splitImagesIntoFolds();
	void splitImagesIntoFolds(int iNumberOfFolds);
	void showImages();
	void showFold(int iWhichFold);
	void showAllFolds();
	void showCurrentImages();
	void executeCrossValidation(bool bVisualize=false, bool bUseNNR=false);

private:
	dlib::array<dlib::array2d<unsigned char>> allImages, shuffledImages, currentImages;
	dlib::array<dlib::array<dlib::array2d<unsigned char>>> foldedImagesVector;
	std::vector<std::vector<dlib::rectangle>> allBoxes, shuffledBoxes, currentBoxes;
	std::vector<std::vector<std::vector<dlib::rectangle>>> foldedBoxesVector;
	std::vector<float> vecC;
	std::vector<float> vecEpsilon;
	int iNumberOfFolds;
	unsigned long ulWinSizeWidth, ulWinSizeHeight;
	std::string strOutputDirectory;

	void shuffleImages();
	void swapBox(dlib::rectangle& box1, dlib::rectangle& box2);
	void combineFoldsIntoTrainingData(int skippedFold);
	void runTrainingSession(float fC, float fEpsilon, unsigned long skippedFold, bool bVisualize, bool bUseNNR);

	// Logging
	//bool Log(std::string WriteToLog);
	//bool Log(float WriteToLog);
	//bool Log(int WriteToLog);
};

