// Author: Dominik Krienelke
// Contact: dominik.krienelke@gmail.com
// Code based on dlib fHOG implementation, original program based on dlib fHOG example

/*
	This program executes fastest when compiled with at least SSE2
	instructions enabled. You should create the build project using:
	cmake path_to_dlib_root/examples -DUSE_SSE2_INSTRUCTIONS=ON
	cmake path_to_dlib_root/examples -DUSE_SSE4_INSTRUCTIONS=ON
	cmake path_to_dlib_root/examples -DUSE_AVX_INSTRUCTIONS=ON
	This will set the appropriate compiler options for GCC, clang, Visual
	Studio, or the Intel compiler.
*/

#define DLIB_JPEG_SUPPORT
#include "dlib/all/source.cpp"
#include "dlib/svm_threaded.h"
#include "dlib/gui_widgets.h"
#include "dlib/image_processing.h"
#include "dlib/data_io.h"

#include <iostream>
#include <fstream>


using namespace std;
using namespace dlib;

// ----------------------------------------------------------------------------------------

int main(int argc, char** argv)
{

	try
	{
		// This example loads data from a provided dataset. The default path to the dataset is set here
		// but ofc you simply can overwrite it.
		std::string input_directory = "images";
		if (argc > 1)
		{
			cout << "Found command line argument for input files. Default path overwritten." << endl;
			cout << endl;
			input_directory = argv[1];
		}

		// The default input directory contains a training dataset (half1) and a separate testing dataset (half2). 
		// Positive training examples within the datasets are marked by rectangles. The datasets are loaded into the 
		// following variables.
		dlib::array<array2d<unsigned char> > images_train, images_test;
		std::vector<std::vector<rectangle> > robot_boxes_train, robot_boxes_test;

		// Now we load the data.  These XML files list the images in each
		// dataset and also contain the positions of the face boxes.
		// To create the XML files the dlib imglab tool was used.
		// To see how to use it read dlib's tools/imglab/README.txt
		load_image_dataset(images_train, robot_boxes_train, input_directory + "/half1/half1.xml");
		load_image_dataset(images_test, robot_boxes_test, input_directory + "/half2/half2.xml");

		// Very basic lpre-processing.  
		// Resize images+markups to twice the size. Optional, but greatly improves results.
		upsample_image_dataset<pyramid_down<2> >(images_train, robot_boxes_train);
		upsample_image_dataset<pyramid_down<2> >(images_test, robot_boxes_test);
		// Double training data size by flipping images.		
		add_image_left_right_flips(images_train, robot_boxes_train);
		cout << "Num training images: " << images_train.size() << endl;
		cout << "Num testing images:  " << images_test.size() << endl;

		// Prepare a Felzenszwalb's HOG feature extractor.
		// Configure some basic values (e.g. sliding window size, scale pyramid scaling, C of SVM,
		// epsilon maximum error term, that stops training when achieved)
		// Normally it requires a lot of work to find the best fitting values for a dataset/task
		typedef scan_fhog_pyramid<pyramid_down<6> > image_scanner_type;
		image_scanner_type scanner;
		// The sliding window detector will be 45 pixels wide and 64 pixels tall.
		scanner.set_detection_window_size(45, 64);
		structural_object_detection_trainer<image_scanner_type> trainer(scanner);

		// The trainer trains a SVM classificator
		trainer.set_num_threads(4); // Set this to the number of processing cores on your machine.
		trainer.set_c(100); // Bigger C => encourares (over)fitting to the training data
		trainer.be_verbose(); // Tell about progress

		// The trainer will run until the "error term" is less than epsilon*C.  Smaller values
		// make the trainer solve the SVM optimization problem more accurately but will take longer.
		trainer.set_epsilon(0.1);

		// Now run the trainer.  
		// For this example, it should take about a couple of minutes to train WITH SSE2 instructions
		object_detector<image_scanner_type> detector = trainer.train(images_train, robot_boxes_train);

		// Once the classificator is trained it can be bulk tested
		cout << "training results: " << test_object_detection_function(detector, images_train, robot_boxes_train) << endl;
		// and for comparison tests on the "unknown" data.
		cout << "testing results:  " << test_object_detection_function(detector, images_test, robot_boxes_test) << endl;

		// One can/should save the detector to disk using the serialize() function.
		serialize("basic_robot_detector.svm") << detector;
		// And to recall it using the deserialize() function.
		object_detector<image_scanner_type> detector2;
		deserialize("basic_robot_detector.svm") >> detector2;

		// Simple way to visualize the detector
		image_window hogwin(draw_fhog(detector), "Learned fHOG detector");

		// Finally a simple way on how to showcase the detections on the images
		image_window win;
		for (unsigned long i = 0; i < images_test.size(); ++i)
		{
			// Run the detector and get the robot detections.
			std::vector<rectangle> dets = detector2(images_test[i]);
			win.clear_overlay();
			win.set_image(images_test[i]);
			win.add_overlay(dets, rgb_pixel(255, 0, 0));
			cout << "Hit enter to process the next image..." << endl;
			cin.get();
		}

		cout << "Example program finished... Press any key" << endl;
		cin.get();
	}
	catch (exception& e)
	{
		cout << "\nexception thrown!" << endl;
		cout << e.what() << endl;
	}
}

// ----------------------------------------------------------------------------------------

