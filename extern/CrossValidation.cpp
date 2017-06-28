#include "CrossValidation.h"

CrossValidation::CrossValidation(unsigned long ulWinSizeWidth, unsigned long ulWinSizeHeight, string strOutputDirectory)
{
	iNumberOfFolds = 2;
	this->ulWinSizeWidth = ulWinSizeWidth;
	this->ulWinSizeHeight = ulWinSizeHeight;
	this->strOutputDirectory = strOutputDirectory;
}

CrossValidation::~CrossValidation()
{
	
}

void CrossValidation::addImages(std::string strPath)
{
	dlib::array<dlib::array2d<unsigned char>> imagesTemp, imagesTemp2;
	std::vector<std::vector<rectangle>> boxesTemp, boxesTemp2;
	dlib::load_image_dataset(imagesTemp, boxesTemp, strPath);
	dlib::load_image_dataset(imagesTemp2, boxesTemp2, strPath);
	for (unsigned long i = 0; i < imagesTemp.size(); i++)
	{
		allImages.push_back(imagesTemp[i]);		
		shuffledImages.push_back(imagesTemp2[i]);
		allBoxes.push_back(boxesTemp[i]);
		shuffledBoxes.push_back(boxesTemp2[i]);
	}
}

void CrossValidation::addC(float fNewC)
{
	vecC.push_back(fNewC);
}

void CrossValidation::addEpsilon(float fNewEpsilon)
{
	vecEpsilon.push_back(fNewEpsilon);
}

void CrossValidation::setCs(std::vector<float> vecC)
{
	this->vecC = vecC;
}

void CrossValidation::setEpsilons(std::vector<float> vecEpsilon)
{
	this->vecEpsilon = vecEpsilon;
}

void CrossValidation::splitImagesIntoFolds()
{
	if (iNumberOfFolds < 2)
		iNumberOfFolds = 2;
	splitImagesIntoFolds(iNumberOfFolds);
}

void CrossValidation::splitImagesIntoFolds(int iNumberOfFolds)
{
	if (iNumberOfFolds < 2)
		iNumberOfFolds = 2;
	this->iNumberOfFolds = iNumberOfFolds;
	shuffleImages();
	dlib::array<dlib::array2d<unsigned char>> tempImages;
	std::vector<std::vector<dlib::rectangle>> tempBoxes;
	unsigned long imagesPerFold = shuffledImages.size() / (iNumberOfFolds+1);
	cout << "Folding image database...\n";
	//cout << "imagesPerFold: " << imagesPerFold << "\n";
	unsigned long currentFold = -1;
	for (unsigned long i = 0; i < shuffledImages.size(); i++)
	{
		if ((i % imagesPerFold) == 0){
			if (currentFold == iNumberOfFolds)
			{
				cout << iNumberOfFolds << "+1(for testing) balanced folds of size " << imagesPerFold << " created. " << shuffledImages.size()-i << " images discarded.\n";
				return;
			}
			foldedImagesVector.push_back(tempImages);
			foldedBoxesVector.push_back(tempBoxes);
			currentFold++;
			//cout << "new fold for i: " << i << "\n";
		}
		foldedImagesVector[currentFold].push_back(shuffledImages[i]);
		foldedBoxesVector[currentFold].push_back(shuffledBoxes[i]);
	}
}

void CrossValidation::showImages()
{
	//shuffleImages();
	cout << "Number of loaded images: " << allImages.size() << "\n";
	image_window win, win2;
	win.set_title("Original"); win2.set_title("Shuffled");
	for (unsigned long j = 0; j < allImages.size(); j++)
	{
		cout << j << ":" << allImages[j].size() << " ";
		win.clear_overlay();
		win2.clear_overlay();
		win.set_image(allImages[j]);
		win2.set_image(shuffledImages[j]);
		win.add_overlay(allBoxes[j], rgb_pixel(255, 0, 0));
		win2.add_overlay(shuffledBoxes[j], rgb_pixel(255, 0, 0));
		cout << "press key to show next image...";
		char c = cin.get();
	}
}

void CrossValidation::showFold(int iWhichFold)
{
	if ((unsigned long)iWhichFold > foldedImagesVector.size())
	{
		cout << "There is no fold: " << iWhichFold << ", max number of folds is: " << foldedImagesVector.size() << "\n";
		return;
	}
	cout << "Number of images in fold " << iWhichFold << ": " << foldedImagesVector[iWhichFold].size() << "\n";
	image_window win;
	win.set_title("Fold " + std::to_string(iWhichFold));
	for (unsigned long j = 0; j < foldedImagesVector[iWhichFold].size(); j++)
	{
		cout << j << ":" << foldedImagesVector[iWhichFold].size() << " ";
		win.clear_overlay();
		win.set_image(foldedImagesVector[iWhichFold][j]);
		win.add_overlay(foldedBoxesVector[iWhichFold][j], rgb_pixel(255, 0, 0));
		cout << "press key to show next image...";
		char c = cin.get();
	}
}

void CrossValidation::showAllFolds()
{
	cout << "Number of folds: " << iNumberOfFolds << "+1 test fold\n";
	cout << "Number of all loaded images: " << allImages.size() << "\n";
	for (unsigned long j = 0; j < foldedImagesVector.size(); j++)
	{
		cout << "Showing all images of fold: " << j << "\n";
		showFold(j);
	}		
}

void CrossValidation::showCurrentImages()
{
	cout << "Number of current images: " << currentImages.size() << "\n";
	image_window win;
	win.set_title("currentImages");
	for (unsigned long j = 0; j < currentImages.size(); j++)
	{
		cout << j << ":" << currentImages[j].size() << " ";
		win.clear_overlay();
		win.set_image(currentImages[j]);
		win.add_overlay(currentBoxes[j], rgb_pixel(255, 0, 0));
		cout << "press key to show next image...";
		char c = cin.get();
	}
}

void CrossValidation::executeCrossValidation(bool bVisualize, bool bUseNNR)
{
	Helper helper("LOGFILE.txt", true);				
	helper.Log("\nCross-validation test with:\n");
	if (bUseNNR)
		helper.Log("\nNNR active with strength: 10\n");
	helper.Log("C\t\t\tEpsilon\t\t\tFold\t\tTime(ms,train)\tTime(ms,fold)\tTime(ms,real)\tPrecision\tRecall\tAvg. Precision\n");
	for (unsigned long i = 0; i < vecC.size(); i++)
	{
		for (unsigned long j = 0; j < vecEpsilon.size(); j++)
		{
			for (unsigned long k = 1; k < foldedImagesVector.size(); k++)
			{
				
				cout << "\nCross-validation test, progress: " << i*(vecEpsilon.size()*(foldedImagesVector.size() - 1)) + j*(foldedImagesVector.size() - 1) + k << " of " << vecC.size()*vecEpsilon.size()*(foldedImagesVector.size() - 1) << ", now:\n";
				cout << "C\tEpsilon\tFold\n";
				cout << vecC[i] << "\t" << vecEpsilon[j] << "\t" << k << "\n";
				helper.Log(std::to_string(vecC[i]) + "\t" + std::to_string(vecEpsilon[j]) + "\t\t" + std::to_string(k) + "\t\t\t");
				combineFoldsIntoTrainingData(k);
				if (bVisualize)
					showCurrentImages();
				runTrainingSession(vecC[i], vecEpsilon[j], k, bVisualize, bUseNNR);
				//helper.Log("\n");
			}
		}
	}
}

// private
void CrossValidation::shuffleImages()
{
	std::vector<unsigned long> randomIndex;
	for (unsigned long i = 0; i < allImages.size(); i++)
	{
		//shuffledImages.push_back(allImages[i]);
		randomIndex.push_back(i);
		//dlib::assign_image(allImages[i], shuffledImages[i]);
	}
	std::srand((unsigned int)time(NULL));
	std::default_random_engine engine(std::rand());
	std::shuffle(std::begin(randomIndex), std::end(randomIndex), engine);
	cout << "\nRandomized swaps... ";
	for (unsigned long i = 0; i < randomIndex.size(); i++)
		cout << i << "<->" << randomIndex[i] << " ";
	cout << "\n";

	for (unsigned long i = 0; i < shuffledImages.size(); i++){
		//dlib::assign_image(allImages[randomIndex[i]], shuffledImages[i]);
		//dlib::resize_image(allImages[randomIndex[i]], shuffledImages[i]);
		shuffledImages[i].swap(allImages[randomIndex[i]]);
		shuffledBoxes[i].swap(allBoxes[randomIndex[i]]);
		//swapBoxes(shuffledBoxes[i], allBoxes[i]);
		//shuffledBoxes.push_back(allBoxes[randomIndex[i]]);
	}
}

void CrossValidation::swapBox(dlib::rectangle& box1, dlib::rectangle& box2)
{
	dlib::rectangle tempbox;
	tempbox = box1;
	box1 = box2;
	box2 = tempbox;
}

void CrossValidation::combineFoldsIntoTrainingData(int skippedFold)
{
	dlib::array<dlib::array2d<unsigned char>> tempImages;
	currentImages.clear();
	currentBoxes.clear();
	//if(skippedFold == 1)
	//currentImages.push_back(tempImages); 
	//currentImages.push_back(tempImages);	
	//cout << "Current Size:" << currentImages.size() << "\n";

	if ((unsigned long)skippedFold > foldedImagesVector.size())
	{
		cout << "There is no fold: " << skippedFold << ", max number of folds is: " << foldedImagesVector.size() << "\n";
		return;
	}

	for (unsigned long i = 1; i < foldedImagesVector.size(); i++)
	{
		if (skippedFold != i)
		{
			cout << "Pushed: ";
			for (unsigned long j = 0; j < foldedImagesVector[i].size(); j++)
			{
				cout  << i << "." << j << ": " << foldedImagesVector[i][j].size() << " ";
				currentImages.push_back(foldedImagesVector[i][j]); // Apparantly this moves the data into the new vector
				//cout << "tempsize folded: " << foldedImagesVector[i][j].size() << " ";
				//cout << "tempsize current: " << currentImages[currentImages.size()-1].size() << "\n";
				Helper::copyImage(currentImages[currentImages.size() - 1], foldedImagesVector[i][j]); // Therefore we have to copy the image back into the folds
				//cout << "endsize folded: " << foldedImagesVector[i][j].size() << " ";
				//cout << "endsize current:" << currentImages[currentImages.size() - 1].size() << " ";
				currentBoxes.push_back(foldedBoxesVector[i][j]);	
			}
			cout << "\n";
		}
	}
}

void CrossValidation::runTrainingSession(float fC, float fEpsilon, unsigned long skippedFold, bool bVisualize, bool bUseNNR)
{
	Helper helper("LOGFILE.txt", true);
	Timer t1 = Timer();
	// Pre-processing 
	// Double size of images and markup boxes
	if (currentImages[0].nc() < 1000) // TODO add function that averages the input images height
	{
		upsample_image_dataset<pyramid_down<2> >(currentImages, currentBoxes);
		if (foldedImagesVector[0][0].nc() < 1000)
			upsample_image_dataset<pyramid_down<2> >(foldedImagesVector[0], foldedBoxesVector[0]);
	}

	// Flip images as poses can be seen as symmetrical, doubles the training size
	if (currentImages.size() < 150)
		add_image_left_right_flips(currentImages, currentBoxes);

	cout << "Number of training images: " << currentImages.size() << endl;
	cout << "Number of testing images:  " << foldedImagesVector[0].size() << endl;

	// Define the detector based on Felzenszwalb's version of HOG
	// 6 means to use an image pyramid that downsamples the image at a ratio of 5/6.  
	// Running the detector over each pyramid level in a sliding window fashion   
	typedef scan_fhog_pyramid<pyramid_down<6>> image_scanner_type;
	image_scanner_type scanner;
	Helper::pickBestHogWindowSize(currentBoxes, ulWinSizeWidth, ulWinSizeHeight, ulWinSizeWidth*ulWinSizeHeight);
	scanner.set_detection_window_size(ulWinSizeWidth, ulWinSizeHeight); // Size of the sliding window detector.

	// Define the trainer of the decision making SVM
	// A bigger C encourages it to fit the training data, but might lead to overfitting.
	// The trainer will run until the "risk gap" is less than a pre-defined epsilon, 0.1 to 0.01 should be plenty accurate
	if (bUseNNR) // set a NNR if requested
	{
		scanner.set_nuclear_norm_regularization_strength(10.0);
		cout << "Using a NNR with strength 10 for this training.\n";
	}
	structural_object_detection_trainer<image_scanner_type> trainer(scanner);
	trainer.set_num_threads(4); // Set this to the number of processing cores on your machine.
	trainer.set_c(fC);
	trainer.be_verbose();  // Trainer will print it's progress to the console.  
	trainer.set_epsilon(fEpsilon);

	// Now make sure all the boxes are obtainable by the scanner.  
	std::vector<std::vector<rectangle> > removed;
	removed = remove_unobtainable_rectangles(trainer, currentImages, currentBoxes);
	removed = remove_unobtainable_rectangles(trainer, foldedImagesVector[0], foldedBoxesVector[0]);
	// if we weren't able to get all the boxes to match then throw an error 
	if (Helper::containsAnyBoxes(removed))
	{
		long lRemovedCounter = 0;
		for (long l = 0; l < removed.size(); l++)
			lRemovedCounter += removed[l].size();
		cout << "Removed " << lRemovedCounter << " unobtainable boxes from training data.";
	}

	// Execute the SVM training
	object_detector<image_scanner_type> detector;

	cout << "\nCommence CV-TRAINING:\n";
	detector = trainer.train(currentImages, currentBoxes);

	// Test the learned detector on the training data, print the precision, recall, and then average precision.
	// ToDo log those values in utilisable output format
	// ToDo log the time performance(in milliseconds) of a detection
	// ToDo test with dlib cv method
	cout << "Fold EVALUATION:\n";	
	dlib::matrix<double, 1, 3> tempevaluationmatrix;
	t1.reset();
	tempevaluationmatrix = test_object_detection_function(detector, currentImages, currentBoxes);
	helper.Log(int(t1.getPassedTime() / currentImages.size())); helper.Log(": \t"); 
	helper.Log(Helper::matToString(tempevaluationmatrix));
	helper.Log("\t\t\t\t\t\t\t\t\t\t");
	cout << "Training results (on trained data): " << tempevaluationmatrix;
	cout << " in ms per image: " << t1.getPassedTime() / currentImages.size() << "\n";
	t1.reset();
	tempevaluationmatrix = test_object_detection_function(detector, foldedImagesVector[skippedFold], foldedBoxesVector[skippedFold]);
	helper.Log(int(t1.getPassedTime() / foldedImagesVector[skippedFold].size())); helper.Log(": \t");
	helper.Log(Helper::matToString(tempevaluationmatrix));
	helper.Log("\t\t\t\t\t\t\t\t\t\t");
	cout << "Testing results (on skipped fold rate for cv-error): " << tempevaluationmatrix;
	cout << " in ms per image: " << t1.getPassedTime() / foldedImagesVector[skippedFold].size() << "\n";
	t1.reset();
	tempevaluationmatrix = test_object_detection_function(detector, foldedImagesVector[0], foldedBoxesVector[0]);
	helper.Log(int(t1.getPassedTime() / foldedImagesVector[0].size())); helper.Log(": \t");
	helper.Log(Helper::matToString(tempevaluationmatrix));
	//helper.Log("\t\t");
	cout << "Real world testing results (on test fold):: " << tempevaluationmatrix;
	cout << " in ms per image: " << t1.getPassedTime() / foldedImagesVector[0].size() << "\n";
	// Save detector(s) to hard disk
	//serialize(strOutputDirectory + "\\" + Helper::getDateAndTimeString() + "\\Fold" + std::to_string(skippedFold) + ".svm") << detector;
	//cout << "Saved detector to: " << strOutputDirectory << "\\" << Helper::getDateAndTimeString() << "\\Fold" << std::to_string(skippedFold) << ".svm";	
	serialize(strOutputDirectory + "\\C" + std::to_string(fC) + "Eps" + std::to_string(fEpsilon) + "Fold" + std::to_string(skippedFold) + ".svm") << detector;
	dlib::save_png(draw_fhog(detector), strOutputDirectory + "\\vis\\C" + std::to_string(fC) + "Eps" + std::to_string(fEpsilon) + "Fold" + std::to_string(skippedFold) + ".png");
	cout << "Saved detector to: " << strOutputDirectory << "\\C" << fC << "Eps" << fEpsilon << "Fold" << std::to_string(skippedFold) << ".svm";

	// Draw visualization of learned HOG for detector window
	// Not sure why this can be done with a single window for Felzenszwalb's version of HOG  (part-based cascade??)

	// Display the test images with the detection boxes overlayed
	if (bVisualize)
	{
		image_window hogwin(draw_fhog(detector), "Learned fHOG detector " + Helper::getDateAndTimeString() + " Fold " + std::to_string(skippedFold) );
		image_window win1, win2, win3;
		std::vector<rectangle> dets1 = detector(currentImages[0]);
		std::vector<rectangle> dets2 = detector(foldedImagesVector[skippedFold][0]);
		std::vector<rectangle> dets3 = detector(foldedImagesVector[0][0]);
		win1.set_title("Training");	win2.set_title("Skipped"); win3.set_title("Real world");
		win1.set_image(currentImages[0]); win1.add_overlay(dets1, rgb_pixel(255, 0, 0));
		win2.set_image(foldedImagesVector[skippedFold][0]); win2.add_overlay(dets2, rgb_pixel(255, 0, 0));
		win3.set_image(foldedImagesVector[0][0]); win3.add_overlay(dets3, rgb_pixel(255, 0, 0));
		cin.get();
	}

	if (bVisualize)
	{	
		image_window win;
		array2d<unsigned char> image_scaled;
		for (unsigned long i = 0; i < foldedImagesVector[0].size(); ++i)
		{
			// Run the detector and get the face detections.
			std::vector<rectangle> dets = detector(foldedImagesVector[0][i]);

			// Scale detections and image to window size
			helper.prepareToWindowScaling(win, foldedImagesVector[0][i]);
			helper.scaleDetectionsToWindow(dets);
			helper.scaleImageToWindow(foldedImagesVector[0][i], image_scaled);

			// Draw image and detections
			win.clear_overlay();
			win.set_image(image_scaled);
			win.add_overlay(dets, rgb_pixel(255, 0, 0));								
			helper.completeToWindowScaling(win);

			cout << "Hit enter to process the next image..." << endl;
			cin.get();
		}
	}
}