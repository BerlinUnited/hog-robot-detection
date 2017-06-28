// Author: Dominik Krienelke
// Contact: dominik.krienelke@gmail.com
// Code based on dlib fHOG implementation, original program based on dlib fHOG example

/*
	Note that this program executes fastest when compiled with at least SSE2
	instructions enabled.  So if you are using a PC with an Intel or AMD chip
	then you should enable at least SSE2 instructions.  If you are using cmake
	to compile this program you can enable them by using one of the following
	commands when you create the build project:
	cmake path_to_dlib_root/examples -DUSE_SSE2_INSTRUCTIONS=ON
	cmake path_to_dlib_root/examples -DUSE_SSE4_INSTRUCTIONS=ON
	cmake path_to_dlib_root/examples -DUSE_AVX_INSTRUCTIONS=ON
	This will set the appropriate compiler options for GCC, clang, Visual
	Studio, or the Intel compiler.  If you are using another compiler then you
	need to consult your compiler's manual to determine how to enable these
	instructions.  Note that AVX is the fastest but requires a CPU from at least
	2011.  SSE4 is the next fastest and is supported by most current machines.
*/

// #include <dlib/all/source.cpp>
/*#include <dlib/svm_threaded.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_processing.h>
#include <dlib/data_io.h>
#include <dlib/config_reader.h>
#include <dlib/cmd_line_parser.h>

#include <iostream>
#include <fstream>
#include "Timer.h"*/

#include "Global.h"

// EXTERN!
// #define DLIB_JPEG_SUPPORT

// using namespace std;
// using namespace dlib;

int main(int argc, char** argv)
{
	try
	{
		// Init helper class
		Helper helper = Helper("LOGFILE.txt", false);
		
		// Configure cmd line parser
		command_line_parser parser;
		// Add options and descriptions
		parser.add_option("train", "Train a fHOG detector SVM with given training and testing XML.");
		parser.add_option("test", "Test an already trained fHOG detector on testing XML.");
		parser.add_option("track", "Track detected (or selected) objects in a videostream.");
		parser.add_option("cross", "Execute cross-validation evaluation on dataset.");
		parser.add_option("ball", "Train ball specific fHOG with cross-validation and parameter optimization");
		parser.add_option("eval", "Bulk evaluate already existing fHOGs.");
		parser.add_option("pose", "Run a set of pose detectors on a given set of images.");
		parser.add_option("visualize", "Shows extra images and visualizations while running through one program, e.g. for -cross every image splitted by folds is shown.");
		parser.add_option("video", "Tries to process input images as fast as possible, can be used to scan through a video");
		parser.add_option("threshold", "Use incremental threshold values to evaluate learned detector during test or evaluation phase.");
		parser.add_option("nnr", "Use a nuclear norm regularizer for detector training.");
		parser.add_option("classic", "Use a classic HOG detector with one single filter.");
		parser.add_option("e", "Set trainings epsilon as acceptance criteria  0.1 to 0.01 should be plenty accurate.", 1); // takes 1 argument
		parser.add_option("c", "Set trainings C for SVM accepted values are 0.000001 to 1000000, default is 1, higher value of C encourages overfitting to the trainings data", 1);
		parser.add_option("h", "Shows available command line options.");
		parser.add_option("in", "This option can be used run a pre-defined training/test, option requires one parameter, e.g. TEST1, TRAINING1", 1);
		parser.parse(argc, argv); // parses the cmd line
		// const char* one_time_opts[] = { "train", "track", "test", "in" }; 
		// parser.check_one_time_options(one_time_opts); // each parameter only once?
		parser.check_incompatible_options("threshold", "classic"); // incompatible options?
		parser.check_option_arg_range("e", 0.0001, 100.0); // check option range?
		parser.check_option_arg_range("c", 0.00001, 1000000.0); // check option range?

		// check if the -h option was given on the command line and display available options
		if (parser.option("h"))
		{
			cout << "Usage: fHOG_FullNaoDetector.exe -train -test -track -cross -ball -eval -visualize -video -threshold -visualize -nnr -classic --e epsilon_value --c C_value --in config_file_block\n";
			parser.print_options();
			return 0;
		}
		
		// Read config file
		config_reader cr("HOGconfig.txt");
		
		// HOG parameters
		unsigned long ulWinSizeWidth = get_option(cr, "HOG.winSizeWidth", 45);
		unsigned long ulWinSizeHeight = get_option(cr, "HOG.winSizeHeight", 64);
		const int SCALING = get_option(cr, "HOG.scaling", 6); // ignored for now
		bool bShowHOG = false;
		
		// Training config
		std::string strImageDirectory = cr.block("Training")["directory"];
		std::string strTrainingDetectorName = cr.block("Training")["detectorName"];
		std::string strTrainingXML = cr.block("Training")["fileTraining"];
		std::string strTrainingTestXML = cr.block("Training")["fileTesting"];
		float fEpsilon = get_option(cr, "Training.SVM.epsilon", 0.1);
		float fC = get_option(cr, "Training.SVM.c_value", 0.1);

		// Test config(s)		
		string strConfigFileBlock = "Test";
		if (parser.option("in") && parser.option("test")) // Use specific test if chosen
		{
			strConfigFileBlock = parser.option("in").argument();
			cout << "Found command line option to run config: " << strConfigFileBlock << "\n";
		}
		std::string strTestDirectory = cr.block(strConfigFileBlock)["directory"];
		std::string strTestDetectorName = cr.block(strConfigFileBlock)["detectorName"];
		std::string strTestXML = cr.block(strConfigFileBlock)["fileTesting"];

		// Tracking config(s)		
		strConfigFileBlock = "Tracking";
		if (parser.option("in") && parser.option("track")) // Use specific test if chosen
		{
			strConfigFileBlock = parser.option("in").argument();
			//cout << "Found command line option to run config: " << strConfigFileBlock << "\n";
		}
		std::string strTrackingDirectory = cr.block(strConfigFileBlock)["directory"];
		std::string strTrackingDetectorName = cr.block(strConfigFileBlock)["detectorName"];

		// Tracking config(s)		
		strConfigFileBlock = "Video";
		if (parser.option("in") && parser.option("video")) // Use specific test if chosen
			strConfigFileBlock = parser.option("in").argument();
		std::string strVideoDirectory = cr.block(strConfigFileBlock)["directory"];
		std::string strVideoDetectorName = cr.block(strConfigFileBlock)["detectorName"];

		// Cross-validation config(s)		
		strConfigFileBlock = "CrossValidation";
		if (parser.option("in")) // Use specific test if chosen
		{
			strConfigFileBlock = parser.option("in").argument();
		}
		std::string strCrossDirectory = cr.block(strConfigFileBlock)["directory"];

		// Evaluation config(s)		
		strConfigFileBlock = "Evaluation";
		if (parser.option("in") && parser.option("eval")) // Use specific test if chosen
		{
			strConfigFileBlock = parser.option("in").argument();
		}
		std::string strEvaluationDirectory = cr.block(strConfigFileBlock)["directory"];
		std::string strEvaluationXML = cr.block(strConfigFileBlock)["fileEvaluation"];
		float fMaximumFilterStrength = get_option(cr, strConfigFileBlock+".filterstrength", 0.3);
		float fClassicness = get_option(cr, strConfigFileBlock+".classicness", 0.25);
		double dStartThreshold = get_option(cr, strConfigFileBlock + ".startthreshold", -3);
		double dEndThreshold = get_option(cr, strConfigFileBlock + ".endthreshold", 3);
		double dThresholdStep = get_option(cr, strConfigFileBlock + ".treshholdstep", 0.25);

		// Test config(s)		
		strConfigFileBlock = "Pose";
		if (parser.option("in") && parser.option("pose")) // Use specific test if chosen
		{
			strConfigFileBlock = parser.option("in").argument();
			cout << "Found command line option to run config: " << strConfigFileBlock << "\n";
		}
		std::string strPoseDirectory = cr.block(strConfigFileBlock)["directory"];
		std::string strPoseXML = cr.block(strConfigFileBlock)["fileTesting"];

		// Overwrite parameters if options were passed
		if (parser.option("e"))
		{
			fEpsilon = ::atof(parser.option("e").argument().c_str());
			cout << "Overwritten config Epsilon with: " << fEpsilon << "\n";
		}
		if (parser.option("c"))
		{
			fC = ::atof(parser.option("c").argument().c_str());
			cout << "Overwritten config C with: " << fC << "\n";
		}

		// Print out the config values
		cout << "HOG:\n  Sliding Window: " << ulWinSizeWidth << "x" << ulWinSizeHeight << " size\n";
		cout << "  Details: " << cr.block("HOG").block("details")["author"] << "\n";

		cout << "Training:\n  Directory: " << strImageDirectory << "\n";
		cout << "  XML: " << strEvaluationXML << "\n";
		cout << "  XML Test: " << strTrainingTestXML << "\n";
		cout << "  SVM:\n    Epsilon: " << fEpsilon << "\n";
		cout << "    C: " << fC << "\n\n";

		cout << "Testing:\n  Directory: " << strTestDirectory << "\n";
		cout << "  detectorName: " << strTestDetectorName << "\n";		
		cout << "  XML Test: " << strTestXML << "\n\n";

		cout << "Pose classification:\n  Directory: " << strPoseDirectory << "\n";
		cout << "  XML of images: " << strPoseXML << "\n\n";

		cout << "Tracking:\n  Directory: " << strTrackingDirectory << "\n";
		cout << "  detectorName: " << strTrackingDetectorName << "\n\n";

		cout << "CrossValidation:\n  Directory: " << strCrossDirectory << "\n\n";

		cout << "Evaluation:\n  Directory: " << strEvaluationDirectory << "\n";
		cout << "  XML of images: " << strEvaluationXML << "\n";
		cout << "  Maximum filter strength: " << fMaximumFilterStrength << "\n";
		cout << "  Classicness: " << fClassicness << "\n";
		cout << "  Starting threshold: " << dStartThreshold << "\n";
		cout << "  Ending threshold: " << dEndThreshold << "\n";
		cout << "  Threshold stepsize: " << dThresholdStep << "\n\n";

		// Declare the variables that will hold our datasets.
		// images_train for all standing Naos as main training examples
		// images_train_occluded for occluded Naos
		// images_test contains all test images
		// the boxes variables contain the markups of the Naos in the images
		dlib::array<array2d<unsigned char> > images_train, images_train_occluded, images_test;
		std::vector<std::vector<rectangle> > face_boxes_train, face_boxes_train_occluded, face_boxes_test;

		// Load the data. 
		// These XML files list the images in each dataset and also contain the positions of the boxes. 
		// dlib comes with tools for creating and loading XML image dataset files
		// XML markups are created with the imglab tool
		load_image_dataset(images_train, face_boxes_train, strImageDirectory + "/" + strTrainingXML);
		if (Helper::fileExists(strImageDirectory + "\\" + "occluded.xml"))
			load_image_dataset(images_train_occluded, face_boxes_train_occluded, strImageDirectory + "/" + "occluded.xml");
		load_image_dataset(images_test, face_boxes_test, strImageDirectory + "/" + strTrainingTestXML);

		// Pre-processing 
		// Double size of images and markup boxes
		if (images_train[0].nc() < 1000)
		{
			upsample_image_dataset<pyramid_down<2> >(images_train, face_boxes_train);
			upsample_image_dataset<pyramid_down<2> >(images_train_occluded, face_boxes_train_occluded);
			upsample_image_dataset<pyramid_down<2> >(images_test, face_boxes_test);
		}

		// Flip images as poses can be seen as symmetrical, doubles the training size
		if (images_train.size() < 150)
			add_image_left_right_flips(images_train, face_boxes_train);

		// Define the detector based on Felzenszwalb's version of HOG
		// 6 means to use an image pyramid that downsamples the image at a ratio of 5/6.  
		// Running the detector over each pyramid level in a sliding window fashion   
		typedef scan_fhog_pyramid<pyramid_down<6>> image_scanner_type;
		image_scanner_type scanner;
		Helper::pickBestHogWindowSize(face_boxes_train, ulWinSizeWidth, ulWinSizeHeight, ulWinSizeWidth*ulWinSizeHeight);
		scanner.set_detection_window_size(ulWinSizeWidth, ulWinSizeHeight); // Size of the sliding window detector.

		// Define the trainer of the decision making SVM
		// A bigger C encourages it to fit the training data, but might lead to overfitting.
		// The trainer will run until the "risk gap" is less than a pre-defined epsilon, 0.1 to 0.01 should be plenty accurate
		
		if (parser.option("nnr")) // set a NNR if requested
		{
			scanner.set_nuclear_norm_regularization_strength(10.0);
			cout << "Using a NNR for this training... \n\n";
		}

		structural_object_detection_trainer<image_scanner_type> trainer(scanner);
		trainer.set_num_threads(4); // Set this to the number of processing cores on your machine.
		trainer.set_c(fC);
		trainer.be_verbose();  // Trainer will print it's progress to the console.  
		trainer.set_epsilon(fEpsilon);

		// Now make sure all the boxes are obtainable by the scanner.  
		std::vector<std::vector<rectangle> > removed;
		removed = remove_unobtainable_rectangles(trainer, images_train, face_boxes_train);
		removed = remove_unobtainable_rectangles(trainer, images_train_occluded, face_boxes_train_occluded);
		// if we weren't able to get all the boxes to match then throw an error 
		if (Helper::containsAnyBoxes(removed))
		{
			long lRemovedCounter = 0;
			for (long l = 0; l < removed.size(); l++)
				lRemovedCounter += removed[l].size();
			cout << "Removed " << lRemovedCounter << " unobtainable boxes from training data.";
		}

		// Execute the SVM training
		// Minimum detection size is defined by the sliding window (and upsampled versions of it) 
		//
		// The training code performs an input validation check on the training data and will throw an exception if it detects any boxes 
		// that are impossible to detect given your setting of scanning window size and image pyramid resolution.
		// Using:  remove_unobtainable_rectangles(trainer, images_train, face_boxes_train)
		// one can automatically discard these impossible boxes from a training dataset
		// before running the trainer. It is recommended to be careful not throwing away important truth boxes you really care about.  
		// Visualize return the set of removed rectangles to inspect them.
		// Images not marked with a truth box are implicitly treated as a negative examples unlessa set of "ignore boxes" is passed as a third argument to the train() function.
		object_detector<image_scanner_type> detector, detector2, detector3, detector4; // always used

		if (parser.option("train")) // Only run training if asked to
		{
			cout << "\nCommence TRAINING:\n";
			cout << "Number of training images: " << images_train.size() << endl;
			cout << "Number of training occluded images: " << images_train_occluded.size() << endl;
			cout << "Number of training testing images:  " << images_test.size() << endl;

			detector = trainer.train(images_train, face_boxes_train);
			if (Helper::fileExists(strImageDirectory + "\\" + "occluded.xml"))
				detector2 = trainer.train(images_train_occluded, face_boxes_train_occluded);
		
			// Test the learned detector on the training data, print the precision, recall, and then average precision.
			cout << "Training results: " << test_object_detection_function(detector, images_train, face_boxes_train) << endl;
			if (Helper::fileExists(strImageDirectory + "\\" + "occluded.xml"))
				cout << "Training occluded results: " << test_object_detection_function(detector2, images_train_occluded, face_boxes_train_occluded) << endl;
		
			// Test the learned detector on the testing data, print the precision, recall, and then average precision.
			// This is the more important test, to show how the detector performs in the real world
			// ToDo automated cross-validation tests?
			// ToDo log the time performance (in milliseconds) of a detection
			// ToDo log those values
			cout << "Testing results:  " << test_object_detection_function(detector, images_test, face_boxes_test) << endl;
			cout << "Num of filters: " << num_separable_filters(detector) << endl;
			cout << "Num of detectors: " << detector.num_detectors() << endl;
			

			// Draw visualization of learned HOG for detector window
			// Not sure why this can be done with a single window for Felzenszwalb's version of HOG  (part-based cascade??)
			image_window hogwin(draw_fhog(detector), "Learned fHOG detector " + strTrainingDetectorName);
			if (Helper::fileExists(strImageDirectory + "\\" + "occluded.xml"))
				image_window hogwin2(draw_fhog(detector2), "Learned fHOG detector occluded" + strTrainingDetectorName);

			// Display the test images with the detection boxes overlayed
			image_window win;
			array2d<unsigned char> image_scaled;

			for (unsigned long i = 0; i < images_test.size(); ++i)
			{
				// Run the detector and get the face detections.
				std::vector<rectangle> dets = detector(images_test[i]);
				std::vector<rectangle> dets2 = detector2(images_test[i]);

				// Scale detections and image to window size
				helper.prepareToWindowScaling(win, images_test[i]);
				helper.scaleDetectionsToWindow(dets);
				helper.scaleDetectionsToWindow(dets2);
				helper.scaleImageToWindow(images_test[i], image_scaled);
				
				// Draw image and detections
				win.clear_overlay();
				win.set_image(image_scaled);
				win.add_overlay(dets, rgb_pixel(255, 0, 0));
				win.add_overlay(dets2, rgb_pixel(0, 0, 255));
				array2d<unsigned char> testimage;
				//testimage.set_size(600, 800);
				//resize_image(images_test[i], testimage);
				//win.set_image(images_test[i]);								
				helper.completeToWindowScaling(win);

				cout << "Hit enter to process the next image..." << endl;
				cin.get();
			}

			// Save detector(s) to hard disk
			serialize(strImageDirectory + "/" + strTrainingDetectorName) << detector;
			if (Helper::fileExists(strImageDirectory + "\\" + "occluded.xml"))
				serialize(strImageDirectory + "/" + "occluded_" + strTrainingDetectorName) << detector2;
		} // end Training
	
		if (parser.option("test")) // Only run tests if asked to
		{
			cout << "\nCommence TESTING:\n";
			dlib::array<array2d<unsigned char>> images_test_test;
			std::vector<std::vector<rectangle>> face_boxes_test_test;
			//cv::VideoCapture cap;
			Timer t1 = Timer();

			if (!parser.option("video"))
			{
				load_image_dataset(images_test_test, face_boxes_test_test, strTestDirectory + "/" + strTestXML);
				cout << "Number of test images: " << images_test_test.size() << endl;

				// Pre-processing 
				// Double size of images and markup boxes  
				upsample_image_dataset<pyramid_down<2> >(images_test_test, face_boxes_test_test);
			}
			
			// Recall a detector by using the deserialize() function.
			deserialize(strTestDirectory + "/" + strTestDetectorName) >> detector3;
			if (Helper::fileExists(strTestDirectory + "\\" + "occluded_" + strTestDetectorName))
				deserialize(strTestDirectory + "/" + "occluded_" + strTestDetectorName) >> detector4;
	
			// Draw visualization of learned HOG for detector window
			image_window hogwin(draw_fhog(detector3), "Learned fHOG detector " + strTestDetectorName);
			if (Helper::fileExists(strTestDirectory + "\\" + "occluded_" + strTestDetectorName))
				image_window hogwin2(draw_fhog(detector4), "Learned fHOG detector occluded" + strTestDetectorName);

			// Prepare evaluation of multiple HOG detectors with increased testing speed (HOG only computed once)
			std::vector<object_detector<image_scanner_type>> my_detectors;
			my_detectors.push_back(detector3); 
			my_detectors.push_back(detector4);
			//my_detectors.push_back(detector3); my_detectors.push_back(detector3); my_detectors.push_back(detector3);// my_detectors.push_back(detector3); 

			if (parser.option("threshold")) // manipulate the threshold value to accept different set of detections
			{
				for (double d = -5; d < 5; d += 0.25)
				{
					cout << "Test results, adjusted threshold " << d << ": " << test_object_detection_function(detector3, images_test_test, face_boxes_test_test, dlib::test_box_overlap(), d);
				}
			}
			else if (parser.option("classic")) // select/use only the pre-dominant HOG filter not the whole cascade
			{
				double d = -10;
				bool bAtLeastOneFilter = true;
				int iNumberOfFilters = num_separable_filters(detector3);
				object_detector<image_scanner_type> detectorClassicHOG = detector3;
				while (iNumberOfFilters > 1 && d < 0.4 && bAtLeastOneFilter)
				{
					detectorClassicHOG = threshold_filter_singular_values(detectorClassicHOG, d);
					iNumberOfFilters = num_separable_filters(detectorClassicHOG);
					cout << "New num of filters: " << iNumberOfFilters << endl;
					d += 0.01;
					if (iNumberOfFilters <= 0) // if number of separable_filters jumps to 0, use the last value with any filters
					{
						bAtLeastOneFilter = false;
						d -= 0.02;
						detectorClassicHOG = threshold_filter_singular_values(detector3, d);

					}						
				}
				cout << "Final num of filters: " << num_separable_filters(detectorClassicHOG) << " with d of: " << d << endl;
				cout << "Test results fHOG: " << test_object_detection_function(detector3, images_test_test, face_boxes_test_test);
				cout << "Test results classicHOG: " << test_object_detection_function(detectorClassicHOG, images_test_test, face_boxes_test_test);
			}
			else if (parser.option("video"))
			{
				//cap.open("2015-07-20-competition-day2-RoboCanes - GOPR4409HD.mp4"); // skipped, for now use fHOG_FullNaoVideoDetector
				cout << "Live video detection started, evaluation skipped.\n\n";
			}
			else 
				cout << "Test results: " << test_object_detection_function(detector3, images_test_test, face_boxes_test_test);

			if (Helper::fileExists(strTestDirectory + "\\" + "occluded_" + strTestDetectorName))
				cout << "Test occluded results: " << test_object_detection_function(detector4, images_train_occluded, face_boxes_train_occluded) << endl;

			// Display the test images with the detection boxes overlayed
			image_window win; 
			win.set_size(1920, 1200);
			array2d<unsigned char> image_scaled;

			unsigned long ul = 0;
			while (ul < images_test_test.size())
			{
				if (parser.option("video")) // Video Test Run
				{
					/*// Grab a frame
					cv::Mat temp;
					cap >> temp;
					cv_image<bgr_pixel> cimg_currentframe(temp);
					cout << "Frame #: " << ul << endl;
					// Run the detector and get the nao detections. */
				}
				else // Normal Test Run
				{
					// Run the detector and get the nao detections.
					std::vector<rectangle> dets, dets2, dets3;
					std::vector<std::pair<double, rectangle>> detspairs;
					t1.reset();

					detector3(images_test_test[ul], detspairs, Helper::threshold); // run detector and save confidence values
					dets = Helper::pairsToRectangles(detspairs);

					if (Helper::fileExists(strTestDirectory + "/" + "occluded_" + strTestDetectorName))
						dets2 = detector4(images_test_test[ul], Helper::threshold);
					//dets = detector3(images_test_test[ul], Helper::threshold); // parallel test only

					if (!parser.option("video")) // if not a video evaluate detector against all images
					{
						t1.print("Executing detector one by one took ms: ");
						t1.reset();
						dets3 = evaluate_detectors(my_detectors, images_test_test[ul], Helper::threshold);
						// t2 = std::chrono::high_resoluti	on_clock::now();
						// std::cout << "executing all detectors at once took " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " milliseconds\n";
						t1.print("Executing all detectors at once took: ");
					}

					// Show HOG feature map
					if (bShowHOG)
					{
						// Convert the image into a FHOG feature image. The output, hog, is a 2D array of 31 dimensional vectors.
						array2d<matrix<float, 31, 1> > map_fHOGfeatures;
						extract_fhog_features(images_test_test[ul], map_fHOGfeatures);

						cout << "hog image has " << map_fHOGfeatures.nr() << " rows and " << map_fHOGfeatures.nc() << " columns." << endl;

						// Copy the FHOG features into a drawable image
						dlib::matrix<unsigned char> image_fHOGfeatures(draw_fhog(map_fHOGfeatures));
						array2d<unsigned char> image_temp_fHOGfeatures;
						image_window winfhog(draw_fhog(map_fHOGfeatures)); // remove comment to show pure fHOG image
						helper.copyImage(image_fHOGfeatures, image_temp_fHOGfeatures);

						// Scale detections and fHOG image to window size
						//helper.prepareToWindowScaling(winfhog, image_temp_fHOGfeatures);
						helper.scaleDetections(dets, 1.75); // Hotfix
						helper.scaleDetections(dets2, 1.75); // Hotfix
						helper.scaleDetections(dets3, 1.75); // Hotfix
						//helper.scaleImageToWindow(image_temp_fHOGfeatures, image_scaled);
						// SCALING works fine for images but looks terrible for feature map, so draw without scaling!
						helper.copyImage(image_fHOGfeatures, image_scaled);
						dlib::save_png(draw_fhog(map_fHOGfeatures), "lasthog.png");
						cout << "FHOG DRAWN!\n";
					}
					else // normal image, not fHOG image
					{
						// Scale detections and image to window size
						helper.prepareToWindowScaling(win, images_test_test[ul]);
						helper.scaleDetectionsToWindow(dets);
						helper.scaleDetectionsToWindow(dets2);
						helper.scaleDetectionsToWindow(dets3);
						helper.scaleImageToWindow(images_test_test[ul], image_scaled);
					}

					// Draw image and detections
					win.clear_overlay();
					win.set_image(image_scaled);
					//win.add_overlay(dets, rgb_pixel(255, 0, 0));
					win.add_overlay(dets2, rgb_pixel(255, 0, 255));
					//win.add_overlay(dets3, rgb_pixel(0, 255, 0));
					if (dets.size() > 0)
					{
						win.add_overlay(dets[0], rgb_pixel(0, 255, 0), std::to_string(detspairs[0].first) + "= HIGHEST");
						for (unsigned long l = 1; l < dets.size() - 1; l++)
							win.add_overlay(dets[l], rgb_pixel(255, 0, 0), std::to_string(detspairs[l].first));
						if (dets.size() > 1)
							win.add_overlay(dets[dets.size() - 1], rgb_pixel(0, 255, 255), std::to_string(detspairs[dets.size() - 1].first) + "= LOWEST");
					}
					helper.completeToWindowScaling(win);

					cout << "Hit enter to process the next image. Use '+' or '-' to adjust detection threshold. Use 'h' to show/hide HOG feature map." << endl;
					char c = cin.get();
					if (c == '+' || c == '-')
						Helper::adjustThreshold(c);
					else if (c == 'h')
						bShowHOG = !bShowHOG;
					else
						ul++; // increase loop counter unless the same picture has to be evaluated with a new threshold
					//cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					cin.clear();
					fflush(stdin);
				}
			}
		}

		if (parser.option("pose")) // Only pose detection test if asked to
		{
			object_detector<image_scanner_type> detectorStanding, detectorSitting, detectorLying, detectorGoalie; // Detectors for pose detection

			cout << "\nCommence POSE DETECTION:\n";
			dlib::array<array2d<unsigned char>> images_pose_test;
			std::vector<std::vector<rectangle>> nao_boxes_post_test;
			load_image_dataset(images_pose_test, nao_boxes_post_test, strPoseDirectory + "/" + strPoseXML);
			Timer t1 = Timer();

			cout << "Number of test images: " << images_pose_test.size() << endl;

			// Pre-processing 
			// Double size of images and markup boxes  
			upsample_image_dataset<pyramid_down<2> >(images_pose_test, nao_boxes_post_test);

			// Load (already learned) pose specific detectors.
			if (Helper::fileExists(strPoseDirectory + "\\" + "Standing.svm"))
				deserialize(strPoseDirectory + "/" + "Standing.svm") >> detectorStanding;
			if (Helper::fileExists(strPoseDirectory + "\\" + "Sitting.svm"))
				deserialize(strPoseDirectory + "/" + "Sitting.svm") >> detectorSitting;
			if (Helper::fileExists(strPoseDirectory + "\\" + "Lying.svm"))
				deserialize(strPoseDirectory + "/" + "Lying.svm") >> detectorLying;
			if (Helper::fileExists(strPoseDirectory + "\\" + "Lying.svm"))
				deserialize(strPoseDirectory + "/" + "Goalie.svm") >> detectorGoalie;

			// Prepare evaluation of multiple HOG detectors with increased testing speed (HOG only computed once)
			std::vector<object_detector<image_scanner_type>> my_detectors;
			my_detectors.push_back(detectorStanding); my_detectors.push_back(detectorSitting); my_detectors.push_back(detectorLying); my_detectors.push_back(detectorGoalie);

			// Display the test images with the detection boxes overlayed
			image_window win;
			win.set_size(1920, 1200);
			array2d<unsigned char> image_scaled;

			unsigned long ul = 0;
			while (ul < images_pose_test.size())
			{
				// Run the detector and get the nao detections.
				std::vector<rectangle> detsStanding, detsSitting, detsLying, detsGoalie, detsAll;
				std::vector<std::pair<double, rectangle>> detspairs;
				
				t1.reset();
				detectorStanding(images_pose_test[ul], detspairs, Helper::threshold); // run detector and save confidence values
				detsStanding = Helper::pairsToRectangles(detspairs);
				detsSitting = detectorSitting(images_pose_test[ul], Helper::threshold);
				detsLying = detectorLying(images_pose_test[ul], Helper::threshold);
				detsGoalie = detectorGoalie(images_pose_test[ul], Helper::threshold);

				if (!parser.option("video")) // if not a video evaluate detector against all images
				{
					t1.print("Executing detector one by one took ms: ");
					t1.reset();

					detsAll = evaluate_detectors(my_detectors, images_pose_test[ul], Helper::threshold);
					// t2 = std::chrono::high_resoluti	on_clock::now();
					// std::cout << "executing all detectors at once took " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << " milliseconds\n";
					t1.print("Executing all detectors at once took: ");
				}

				// Scale detections and image to window size
				helper.prepareToWindowScaling(win, images_pose_test[ul]);
				helper.scaleDetectionsToWindow(detsStanding);
				helper.scaleDetectionsToWindow(detsSitting);
				helper.scaleDetectionsToWindow(detsLying);
				helper.scaleDetectionsToWindow(detsGoalie);
				helper.scaleImageToWindow(images_pose_test[ul], image_scaled);
			

				// Draw image and detections
				win.clear_overlay();
				win.set_image(image_scaled);
				if (detsStanding.size() > 0)
				{
					win.add_overlay(detsStanding[0], rgb_pixel(50, 255, 50), std::to_string(detspairs[0].first) + "= HIGHEST standing");
					for (unsigned long l = 1; l < detsStanding.size() - 1; l++)
						win.add_overlay(detsStanding[l], rgb_pixel(0, 255, 0), "Standing");
					if (detsStanding.size() > 1)
						win.add_overlay(detsStanding[detsStanding.size() - 1], rgb_pixel(0, 200, 0), std::to_string(detspairs[detsStanding.size() - 1].first) + "= LOWEST standing");
				}
				if (detsSitting.size() > 0)
					for (unsigned long l = 0; l < detsSitting.size(); l++)
						win.add_overlay(detsSitting[l], rgb_pixel(255, 215, 0), "Sitting");
				if (detsLying.size() > 0)
					for (unsigned long l = 0; l < detsLying.size(); l++)
						win.add_overlay(detsLying[l], rgb_pixel(255, 0, 0), "Lying");
				if (detsGoalie.size() > 0)
					for (unsigned long l = 0; l < detsGoalie.size(); l++)
						win.add_overlay(detsGoalie[l], rgb_pixel(0, 0, 255), "Goalie");

				helper.completeToWindowScaling(win);
				if (!parser.option("video")) // if not a video wait for input before processing/showing next image
				{
					cout << "Hit enter to process the next image. Use '+' or '-' to adjust detection threshold. Use 'h' to show/hide HOG feature map." << endl;
					char c = cin.get();
					if (c == '+' || c == '-')
						Helper::adjustThreshold(c);
					else if (c == 'h')
						bShowHOG = !bShowHOG;
					else
						ul++; // increase loop counter unless the same picture has to be evaluated with a new threshold
					//cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					cin.clear();
					fflush(stdin);
				}
				else
					ul++;
			}
		}

		if (parser.option("track")) // Only run tracking if asked to
		{
			cout << "\nCommence TRACKING:\n";
			int iDetectionRefreshRate = 5;
			Timer t1 = Timer();

			deserialize(strTrackingDirectory + "/" + strTrackingDetectorName) >> detector3;
			// Get the list of video frames.  
			std::vector<file> files = get_files_in_directory_tree(strTrackingDirectory, match_ending(".jpg"));
			std::sort(files.begin(), files.end());
			if (files.size() == 0)
			{
				cout << "No .jpg images found in " << strTrackingDirectory << endl;
				return 1;
			}

			// Load the first video frame.  
			array2d<unsigned char> img, image_scaled, imageScaledForTracking;
			load_image(img, files[0]);
		
			imageScaledForTracking.set_size(img.nr() * 2, img.nc() * 2);
			resize_image(img, imageScaledForTracking);

			std::vector<rectangle> dets = detector3(imageScaledForTracking);
			std::vector<rectangle> trackerrects;
			std::vector<correlation_tracker> trackers;
			//trackers.resize(dets.size());

			for (unsigned int i = 0; i < dets.size(); i++)
			{
				trackers.push_back(correlation_tracker());
				trackers[i].start_track(img, rectangle(dets[i].left()/2, dets[i].top()/2, dets[i].right()/2, dets[i].bottom()/2));
				cout << "tracker " << i << ": x,y,width,height: " << trackers[i].get_position().left() << ", " << trackers[i].get_position().top() << ", " << trackers[i].get_position().width() << ", " << trackers[i].get_position().height() << endl;
			}

			image_window win;
			for (unsigned long i = 0; i < files.size(); ++i)
			{
				load_image(img, files[i]);
				t1.reset();
				if (i > 0)
					for (unsigned int k = 0; k < trackers.size(); k++)
						trackers[k].update(img);
				t1.print("Updating trackers took ms: ");
				
				// Scale detections and image to window size
				helper.prepareToWindowScaling(win, img);
				helper.scaleImageToWindow(img, image_scaled);
				win.set_image(image_scaled);
				win.clear_overlay();

				// Update trackers with detections
				if (i % iDetectionRefreshRate == 1){
					imageScaledForTracking.set_size(img.nr() * 2, img.nc() * 2);
					resize_image(img, imageScaledForTracking);
					t1.reset();
					std::vector<rectangle> dets = detector3(imageScaledForTracking);
					t1.print("Detection update took ms: ");					

					// Scale detections for drawing
					for (unsigned int k = 0; k < dets.size(); k++){						
						//win.add_overlay(rectangle(dets[k].left() / 2, dets[k].top() / 2, dets[k].right() / 2, dets[k].bottom() / 2), rgb_pixel(0, 255, 0));
						//cout << "dets " << k << ": x,y,width,height: " << dets[k].left() << ", " << dets[k].top() << ", " << dets[k].width() << ", " << dets[k].height() << endl;
						dets[k] = rectangle(dets[k].left() / 2, dets[k].top() / 2, dets[k].right() / 2, dets[k].bottom() / 2);
					}
					helper.scaleDetectionsToWindow(dets);
					win.add_overlay(dets, rgb_pixel(0, 255, 0));
				}
				
				// Scale and draw updated trackers
				trackerrects.clear();
				for (unsigned int k = 0; k < trackers.size(); k++)
				{
					//cout << "tracker " << k << ": x,y,width,height: " << trackers[k].get_position().left() << ", " << trackers[k].get_position().top() << ", " << trackers[k].get_position().width() << ", " << trackers[k].get_position().height() << endl;
					//win.add_overlay(trackers[k].get_position(), rgb_pixel(255-10*k,10*k,5*k));
					trackerrects.push_back(trackers[k].get_position());
				}
				helper.scaleDetectionsToWindow(trackerrects);
				win.add_overlay(trackerrects, rgb_pixel(255, 0, 0));

				helper.completeToWindowScaling(win);
				cout << "hit enter to process next frame..." << endl;
				char c = cin.get();
			} // tracked through all images
		}

		if (parser.option("video") && !parser.option("test")) // Only run video if asked to
		{
			cout << "\nCommence VIDEO:\n";
			int iDetectionRefreshRate = 2;
			Timer t1 = Timer();

			deserialize(strVideoDirectory + "/" + strVideoDetectorName) >> detector3;
			// Get the list of video frames.  
			std::vector<file> files = get_files_in_directory_tree(strVideoDirectory, match_ending(".jpg"));
			std::sort(files.begin(), files.end());
			if (files.size() == 0)
			{
				cout << "No .jpg images found in " << strVideoDirectory << endl;
				return 1;
			}

			// Load the first video frame  
			array2d<unsigned char> img, image_scaled, imageScaledForTracking;
			load_image(img, files[0]);
			imageScaledForTracking.set_size(img.nr() * 2, img.nc() * 2);
			resize_image(img, imageScaledForTracking);
			std::vector<rectangle> dets = detector3(imageScaledForTracking);
			image_window win;

			// Process next video frame  
			for (unsigned long i = 0; i < files.size(); ++i)
			{
				cout << ".";
				load_image(img, files[i]);

				// Scale detections and image to window size
				helper.prepareToWindowScaling(win, img);
				helper.scaleImageToWindow(img, image_scaled);
				win.set_image(image_scaled);

				// Update trackers with detections
				imageScaledForTracking.set_size(img.nr() * 2, img.nc() * 2);
				resize_image(img, imageScaledForTracking);
				std::vector<rectangle> dets = detector3(imageScaledForTracking);

				// Scale detections for drawing
				for (unsigned int k = 0; k < dets.size(); k++){
					//win.add_overlay(rectangle(dets[k].left() / 2, dets[k].top() / 2, dets[k].right() / 2, dets[k].bottom() / 2), rgb_pixel(0, 255, 0));
					//cout << "dets " << k << ": x,y,width,height: " << dets[k].left() << ", " << dets[k].top() << ", " << dets[k].width() << ", " << dets[k].height() << endl;
					dets[k] = rectangle(dets[k].left() / 2, dets[k].top() / 2, dets[k].right() / 2, dets[k].bottom() / 2);
				}
				helper.scaleDetectionsToWindow(dets);
				//if (i % iDetectionRefreshRate == 1)
					win.clear_overlay();
				win.add_overlay(dets, rgb_pixel(0, 255, 0));				
				helper.completeToWindowScaling(win);
			} // shown all video images
		}

		if (parser.option("cross")) // Only run tracking if asked to
		{
			cout << "\nCommence CROSSVALIDATION:\n";
			int iNumberOfFolds = 2;
			Timer t1 = Timer();
			CrossValidation cv(ulWinSizeWidth, ulWinSizeHeight, strCrossDirectory);
			
			/*cv.addC(0.000001); cv.addC(0.00001); cv.addC(0.0001); cv.addC(0.001);  cv.addC(0.01);
			cv.addC(0.1); */cv.addC(1); cv.addC(10); cv.addC(100); cv.addC(1000); 
			cv.addC(10000); cv.addC(100000); cv.addC(1000000); cv.addC(10000000); cv.addC(100000000); /**/
			cv.addEpsilon(0.00000001); cv.addEpsilon(0.0000001); cv.addEpsilon(0.000001); cv.addEpsilon(0.00001); cv.addEpsilon(0.0001);
			cv.addEpsilon(0.001);  cv.addEpsilon(0.01); cv.addEpsilon(0.1);cv.addEpsilon(1);  cv.addEpsilon(10);  
			cv.addEpsilon(100);  cv.addEpsilon(1000); cv.addEpsilon(10000); cv.addEpsilon(100000);  cv.addEpsilon(1000000);

			// Get the list of xml files in directory.  
			std::vector<file> files = get_files_in_directory_tree(strCrossDirectory, match_ending(".xml"));
			std::sort(files.begin(), files.end());
			if (files.size() == 0)
			{
				cout << "No .xml images found in " << strCrossDirectory << endl;
				return 1;
			}

			// Load all images and boxes into database for cv test
			for (unsigned long i = 0; i < files.size(); ++i)
			{
				cout << "adding: " << strCrossDirectory + "/" + files[i].name() << "\n";
				cv.addImages(strCrossDirectory + "/" + files[i].name());
			}

			// Prepare cross validation by randomly splitting images and boxes into folds that can than be cross-validated against each other
			cv.splitImagesIntoFolds(iNumberOfFolds);
			//cv.splitImagesIntoFolds(4);
			if (parser.option("visualize")) // Only visualize folds if asked to
			{
				//cv.showImages();
				//cv.showFold(0);
				cv.showAllFolds();
			}
			cv.executeCrossValidation(parser.option("visualize"), parser.option("nnr"));

			cout << "\nCross-validation done. Press any key to finish.";
			cin.get();
		}	// cross validation finished

		if (parser.option("ball"))
		{
			cout << "\nCommence BALLTRAINING:\n";
			int iNumberOfFolds = 2;
			Timer t1 = Timer();
			CrossValidation cv(20, 20, strCrossDirectory);
			cv.addC(10);  cv.addC(100);  cv.addC(1000);
			cv.addEpsilon(0.01); cv.addEpsilon(0.1); cv.addEpsilon(1);  cv.addEpsilon(10);

			// Get the list of xml files in directory.  
			std::vector<file> files = get_files_in_directory_tree(strCrossDirectory, match_ending(".xml"));
			std::sort(files.begin(), files.end());
			if (files.size() == 0)
			{
				cout << "No .xml images found in " << strCrossDirectory << endl;
				return 1;
			}

			// Load all images and boxes into database for cv test
			for (unsigned long i = 0; i < files.size(); ++i)
			{
				cout << "adding: " << strCrossDirectory + "/" + files[i].name() << "\n";
				cv.addImages(strCrossDirectory + "/" + files[i].name());
			}

			// Prepare cross validation by randomly splitting images and boxes into folds that can than be cross-validated against each other
			cv.splitImagesIntoFolds(iNumberOfFolds);
			cv.executeCrossValidation(parser.option("visualize"));

			cout << "\nBall training cross-validation done. Press any key to finish.";
			cin.get();
		}	// ball cross validation finished

		if (parser.option("eval"))
		{
			cout << "\nCommence EVALUATION:\n";
			Helper helper("LOGFILE.txt", true);

			if (parser.option("threshold")) // evaluate different thresholds
			{
				helper.Log("\nEvaluation test with:\n\tStartTreshold: "); helper.Log((float)dStartThreshold); helper.Log("\n\tEndThreshold: "); helper.Log((float)dEndThreshold);  helper.Log("\n\tStepsize: "); helper.Log((float)dThresholdStep); helper.Log("\n");
				helper.Log("\nC\t\t\tEpsilon\t\t\tThreshold\t\tTime(ms,real)\tPrecision\tRecall\tAvg. Precision\n");
			} 
			else // regular evaluation
			{
				helper.Log("\nEvaluation test with:\n\tFilterstrength: "); helper.Log(fMaximumFilterStrength); helper.Log("\n\tClassicness: "); helper.Log(fClassicness);  helper.Log("\n");
				helper.Log("\nC\t\t\tEpsilon\t\t\tFold\t\t\tFilters\t\tTime(ms,real)\tPrecision\tRecall\tAvg. Precision\n");
			}

			Timer t1 = Timer();

			// Get the list of . files in directory.  
			std::vector<file> files = get_files_in_directory_tree(strEvaluationDirectory, match_ending(".svm"));
			std::sort(files.begin(), files.end());
			if (files.size() == 0)
			{
				cout << "No .svm detectors found in " << strEvaluationDirectory << endl;
				return 1;
			}

			// Load evaluation images
			dlib::array<array2d<unsigned char> > images_eval;
			std::vector<std::vector<rectangle> > nao_boxes_eval;
			load_image_dataset(images_eval, nao_boxes_eval, strEvaluationDirectory + "/" + strEvaluationXML);
			cout << "  Loaded images: " << images_eval.size();

			// Pre-processing 
			// Double size of images and markup boxes
			if (images_eval[0].nc() < 1000)
				upsample_image_dataset<pyramid_down<2> >(images_eval, nao_boxes_eval);
			if (images_eval.size() < 50){
				add_image_left_right_flips(images_eval, nao_boxes_eval);
				cout << ", flipped images and increased to: " << images_eval.size() << "\n";
			}
			cout << "\n";

			// Iterate through all .svms
			for (unsigned long i = 0; i < files.size(); ++i)
			{
				cout << "Evaluating: " << strEvaluationDirectory + "/" + files[i].name() << "\n";
				helper.Log(files[i].name());
				//helper.Log(std::to_string(vecC[i]) + "\t" + std::to_string(vecEpsilon[j]) + "\t\t" + std::to_string(k) + "\t\t\t");
				object_detector<image_scanner_type> detectorForEvaluation;
				deserialize(strEvaluationDirectory + "/" + files[i].name()) >> detectorForEvaluation;

				if (parser.option("threshold")) // iterate through a number of threshold values to increase precision while decreasing recall (or the other way around)
				{
					dlib::matrix<double, 1, 3> tempevaluationmatrix;
					tempevaluationmatrix = 0, 0, 0;
					int iTimePassed = 0;
					for (double dThreshold = dStartThreshold; dThreshold <= dEndThreshold; dThreshold += dThresholdStep)
					{
						helper.Log("\t\t\t"); helper.Log((float)dThreshold); helper.Log("\t\t\t");						
						t1.reset();
						if (!Helper::matIs100(tempevaluationmatrix)) // already reached upper limit
						{
							tempevaluationmatrix = test_object_detection_function(detectorForEvaluation, images_eval, nao_boxes_eval, dlib::test_box_overlap(), dThreshold);
							iTimePassed = int(t1.getPassedTime() / images_eval.size());
						}
						helper.Log(iTimePassed); helper.Log(": \t");
						helper.Log(Helper::matToString(tempevaluationmatrix));
						helper.Log("\t\t");
						cout << "Test with adjusted threshold = " << dThreshold << ": " << tempevaluationmatrix;
						cout << " in ms per image: " << iTimePassed << "\n";
					}
				}
				else if (parser.option("classic")) // select/use only the pre-dominant HOG filter not the whole cascade
				{
					double d = -10;
					bool bClassicness = true;
					int iNumberOfFilters = num_separable_filters(detectorForEvaluation);
					int iStartNumberOfFilters = iNumberOfFilters;
					object_detector<image_scanner_type> detectorClassicHOG = detectorForEvaluation;
					cout << "Decreasing the number of filters from: " << iNumberOfFilters;
					// Filtercount only! helper.Log("\t\t\t"); helper.Log((int)num_separable_filters(detectorClassicHOG)); helper.Log("\n");
					while (iNumberOfFilters > 1 && d < fMaximumFilterStrength && bClassicness)
					{
						detectorClassicHOG = threshold_filter_singular_values(detectorClassicHOG, d);
						iNumberOfFilters = num_separable_filters(detectorClassicHOG);
						//cout << "New num of filters: " << iNumberOfFilters << endl;
						cout << ".";
						//cout << ".\n" << iNumberOfFilters << " " << iStartNumberOfFilters << "=" << (float)((float)iNumberOfFilters / (float)iStartNumberOfFilters) << " > " << fClassicness;
						d += 0.01;
						if (iNumberOfFilters <= 0 || (float)((float)iNumberOfFilters / (float)iStartNumberOfFilters) <= 1-fClassicness) // if number of separable_filters jumps to 0 or below classicness threshold, use the last value with any filters
						{
							bClassicness = false;
							d -= 0.02;
							deserialize(strEvaluationDirectory + "/" + files[i].name()) >> detectorForEvaluation;
							if (d > -9.99)
								detectorClassicHOG = threshold_filter_singular_values(detectorForEvaluation, d);
							else 
								detectorClassicHOG = detectorForEvaluation;
						}
					}
					cout << "\nFinal num of filters: " << num_separable_filters(detectorClassicHOG) << " with d of: " << d << endl;
					helper.Log("\t\t\t"); helper.Log((int)num_separable_filters(detectorClassicHOG)); helper.Log("\t\t\t");
					dlib::matrix<double, 1, 3> tempevaluationmatrix;
					t1.reset();
					tempevaluationmatrix = test_object_detection_function(detectorClassicHOG, images_eval, nao_boxes_eval);
					helper.Log(int(t1.getPassedTime() / images_eval.size())); helper.Log(": \t");
					helper.Log(Helper::matToString(tempevaluationmatrix));
					helper.Log("\t\t");
					cout << "Test results classicHOG: " << tempevaluationmatrix;
					cout << " in ms per image: " << t1.getPassedTime() / images_eval.size() << "\n";
					serialize(strEvaluationDirectory + "/" + files[i].name()) << detectorClassicHOG;
					//t1.reset();
					//cout << "Test results fHOG: " << test_object_detection_function(detectorForEvaluation, images_eval, nao_boxes_eval);
					//cout << " in ms per image: " << t1.getPassedTime() / images_eval.size() << "\n";
				} // classic HOG evaluation
			}

			cout << "\nEvalutation done. Press any key to finish.";
			cin.get();			
		}	// evaluation finished

		// Optional features
		// Evaluation of multiple HOG detectors with increased testing speed (HOG only computed once)
		// std::vector<object_detector<image_scanner_type>> my_detectors;
		// my_detectors.push_back(detector); my_detectors.push_back(detector2); my_detectors.push_back(detector3);
		// std::vector<rectangle> dets = evaluate_detectors(my_detectors, images_train[0]);
		//
		// Addition of a nuclear norm regularizer to the SVM trainer 
		// => Learned HOG detector to be composed of separable filters and can be executed faster + generalization filter works smoother
		// To do so call the function before creating a the trainer object:
		//    scanner.set_nuclear_norm_regularization_strength(1.0);
		// Argument determines how important it is to have a small nuclear norm, it is analogous to the C value  
		// Smaller strength for NNR => smoother and faster HOG filters
		// Bigger strenght for NNR => better fitting the given data

		// Show how many separable filters are inside your detector:
		// cout << "Num of filters: " << num_separable_filters(detector) << endl;
		
		// You can also control how many filters there are by explicitly thresholding the
		// singular values of the filters. The function removes filter components with singular values less than 0.1.  
		// Bigger number => fewer separable filters => faster detection + lower accuracy
		// detector = threshold_filter_singular_values(detector, 0.1);
		// cout << "New num of filters: " << num_separable_filters(detector) << endl;

	}
	catch (exception& e)
	{
		cout << "\nexception thrown!" << endl;
		cout << e.what() << endl;
	}
}

// ----------------------------------------------------------------------------------------