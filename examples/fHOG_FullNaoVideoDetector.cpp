// Author: Dominik Krienelke
// Contact: dominik.krienelke@gmail.com
// Code based on dlib fHOG implementation, original program based on dlib face_landmark_detection_ex example.

/*
	This example is essentially just a version of the face_landmark_detection_ex.cpp
    example modified to use OpenCV's VideoCapture object to read from a camera instead 
    of files.
	
	Note that this program executes fastest when compiled with at least SSE2
	instructions enabled. 
*/

#include "Global.h"

using namespace dlib;
using namespace std;
#include <conio.h>

int main()
{
    try
    {
        // Inits		
		cv::VideoCapture cap;
		Helper::threshold = -0.5;
		unsigned long ulFrames = 0;
		Timer t1 = Timer();
		const int FLIMITER = 2000;
		//const int FLIMITER = 0;
		cap.open("half1.mp4");
        image_window win;
		win.set_size(960, 540);
		typedef scan_fhog_pyramid<pyramid_down<6>> image_scanner_type;
		Helper helper = Helper("LOGFILE_Video.txt", false);
		helper.Log("Starting threshold Sk = "); helper.Log((float)Helper::threshold); helper.Log("\n");
		helper.Log("Starting green filter = "); helper.Log((int)Helper::bGreenFilter); helper.Log("\n");

		//image_scanner_type scanner;

		// Prepare tracker
		/*std::vector<rectangle> trackerrects;
		std::vector<correlation_tracker> trackers;
		//trackers.resize(dets.size());

		// Init trackers
		trackers.push_back(correlation_tracker());
		trackers.push_back(correlation_tracker());
		trackers.push_back(correlation_tracker());
		trackers.push_back(correlation_tracker());
		trackers.push_back(correlation_tracker());
		trackers.push_back(correlation_tracker());
		trackers.push_back(correlation_tracker());
		trackers.push_back(correlation_tracker());*/

		// Recall a detector by using the deserialize() function.
		object_detector<image_scanner_type> detector;
		object_detector<image_scanner_type> detectorStanding, detectorSitting, detectorLying, detectorGoalie; // Pose

		//deserialize("best.svm") >> detector;
		deserialize("Standing.svm") >> detectorStanding;
		deserialize("Sitting.svm") >> detectorSitting;
		deserialize("Lying.svm") >> detectorLying;
		deserialize("Goalie.svm") >> detectorGoalie;

        // Grab and process frames until the main window is closed by the user.
        while(!win.is_closed())
        {
            // Grab a frame
            cv::Mat temp;
            cap >> temp;
			ulFrames++;
            cv_image<bgr_pixel> cimg(temp);  // Turn OpenCV's Mat into something dlib can deal with. Wraps the Mat object, it doesn't copy anything.  So cimg is only valid as
			array2d<unsigned char> image_scaled;

			//if (ulFrames < 300) continue; // 1x sitting
			//if (ulFrames < 2000) continue; // 1x falling
			//if (ulFrames < 2700) continue; // 1x sitting


			// Detect robots 
			std::vector<std::pair<double, rectangle>> detspairs, detspairstemp;
			std::vector<std::pair<double, rectangle>> detspairsSitting, detspairstempSitting;
			std::vector<std::pair<double, rectangle>> detspairsLying, detspairstempLying;
			std::vector<std::pair<double, rectangle>> detspairsGoalie, detspairstempGoalie;
			/*std::vector<rectangle> dets = detector(cimg);
			detector(cimg, detspairstemp, Helper::threshold); // run detector and save confidence values*/

			// Run the detector and get the nao detections.
			std::vector<rectangle> detsStanding, detsSitting, detsLying, detsGoalie, detsAll;

			t1.reset();
			detectorStanding(cimg, detspairstemp, Helper::threshold); // run detector and save confidence values
			detectorSitting(cimg, detspairstempSitting, Helper::threshold); // run detector and save confidence values
			detectorLying(cimg, detspairstempLying, Helper::threshold); // run detector and save confidence values
			detectorGoalie(cimg, detspairstempGoalie, Helper::threshold); // run detector and save confidence values

			detspairs = Helper::filterPoseDetections(detspairstemp, ulFrames, 1);
			detspairsSitting = Helper::filterPoseDetections(detspairstempSitting, ulFrames, 2);
			detspairsLying = Helper::filterPoseDetections(detspairstempLying, ulFrames, 3);
			detspairsGoalie = Helper::filterPoseDetections(detspairstempGoalie, ulFrames, 4);

			detsStanding = Helper::pairsToRectangles(detspairs);
			detsSitting = Helper::pairsToRectangles(detspairsSitting);
			detsLying = Helper::pairsToRectangles(detspairsLying);
			detsGoalie = Helper::pairsToRectangles(detspairsGoalie);

			//dets = Helper::pairsToRectangles(detspairs);
			helper.Log("Frame: "); helper.Log(ulFrames); helper.Log("\n");
			//helper.Log("\t#Detections: "); helper.Log((int)dets.size()); helper.Log("\n");

			// Scale detections and image to window size
			helper.prepareToWindowScaling(win, cimg);
			// helper.scaleDetectionsToWindow(dets);
			helper.scaleDetectionsToWindow(detsStanding);
			helper.scaleDetectionsToWindow(detsSitting);
			helper.scaleDetectionsToWindow(detsLying);
			helper.scaleDetectionsToWindow(detsGoalie);
			helper.scaleImageToWindow(cimg, image_scaled);

			// Draw image and detections
			win.clear_overlay();
			win.set_image(image_scaled);
			//win.add_overlay(dets, rgb_pixel(255, 0, 0));
			win.add_overlay(detsGoalie, rgb_pixel(0, 0, 255));

			win.add_overlay(dlib::point(-1, 1), rgb_pixel(255, 0, 0), "Frame: " + Helper::ulongToString(ulFrames));
			win.add_overlay(dlib::point(-1, 15), rgb_pixel(255, 0, 0), "Threshold Sk: " + Helper::doubleToString(Helper::threshold));
			if(Helper::bGreenFilter)
				win.add_overlay(dlib::point(-1, 30), rgb_pixel(255, 0, 0), "Green filter: On");
			else
				win.add_overlay(dlib::point(-1, 30), rgb_pixel(255, 0, 0), "Green filter: Off");

			// Colors
			/*if (dets.size() > 0)
			{
				win.add_overlay(dets[0], rgb_pixel(0, 255, 0), std::to_string(detspairs[0].first) + "= HIGHEST");
				for (unsigned long l = 1; l < dets.size() - 1; l++)
					win.add_overlay(dets[l], rgb_pixel(255, 0, 0), std::to_string(detspairs[l].first));
				if (dets.size() > 1)
					win.add_overlay(dets[dets.size() - 1], rgb_pixel(0, 255, 255), std::to_string(detspairs[dets.size() - 1].first) + "= LOWEST");
			}*/

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
			
			// Red detections only
			/*for (unsigned long l = 0; l < dets.size(); l++)
				win.add_overlay(dets[l], rgb_pixel(255, 0, 0), std::to_string(detspairs[l].first));*/


			helper.completeToWindowScaling(win);
			
/*
			// Load trackers for first frame
			if (ulFrames == 1)
			{
				cout << "\nPredefined Trackers:\n";
				trackers[0].start_track(cimg, rectangle(206, 356, 282, 452));
				trackers[1].start_track(cimg, rectangle(236, 514, 352, 640));
				trackers[2].start_track(cimg, rectangle(1136, 244, 1210, 368));
				trackers[3].start_track(cimg, rectangle(708, 514, 768, 608));

				trackers[4].start_track(cimg, rectangle(1536, 296, 1608, 416));
				trackers[5].start_track(cimg, rectangle(1354, 260, 1400, 338));
				trackers[6].start_track(cimg, rectangle(392, 552, 502, 726));				
				trackers[7].start_track(cimg, rectangle(622, 216, 682, 312));

			}
			if (ulFrames == 2) // Set manual trackers
			{
				point p1, p2; 
				//win.get_next_double_click(p1); win.get_next_double_click(p2);
				//trackers[7].start_track(cimg, rectangle(2 * p1.x(), 2 * p1.y(), 2 * p2.x(), 2 * p2.y()));
				cout << "tracker " << 0 << ": x,y,width,height: " << trackers[0].get_position().left() << ", " << trackers[0].get_position().top() << ", " << trackers[0].get_position().right() << ", " << trackers[0].get_position().bottom() << endl;
				cout << "tracker " << 1 << ": x,y,width,height: " << trackers[1].get_position().left() << ", " << trackers[1].get_position().top() << ", " << trackers[1].get_position().right() << ", " << trackers[1].get_position().bottom() << endl;
				cout << "tracker " << 2 << ": x,y,width,height: " << trackers[2].get_position().left() << ", " << trackers[2].get_position().top() << ", " << trackers[2].get_position().right() << ", " << trackers[2].get_position().bottom() << endl;
				cout << "tracker " << 3 << ": x,y,width,height: " << trackers[3].get_position().left() << ", " << trackers[3].get_position().top() << ", " << trackers[3].get_position().right() << ", " << trackers[3].get_position().bottom() << endl;
				cout << "tracker " << 4 << ": x,y,width,height: " << trackers[4].get_position().left() << ", " << trackers[4].get_position().top() << ", " << trackers[4].get_position().right() << ", " << trackers[4].get_position().bottom() << endl;
				cout << "tracker " << 5 << ": x,y,width,height: " << trackers[5].get_position().left() << ", " << trackers[5].get_position().top() << ", " << trackers[5].get_position().right() << ", " << trackers[5].get_position().bottom() << endl;
				cout << "tracker " << 6 << ": x,y,width,height: " << trackers[6].get_position().left() << ", " << trackers[6].get_position().top() << ", " << trackers[6].get_position().right() << ", " << trackers[6].get_position().bottom() << endl;
				cout << "tracker " << 7 << ": x,y,width,height: " << trackers[7].get_position().left() << ", " << trackers[7].get_position().top() << ", " << trackers[7].get_position().right() << ", " << trackers[7].get_position().bottom() << endl;
			}

			for (unsigned int k = 0; k < trackers.size(); k++)
				trackers[k].update(cimg);

			// Scale detections and image to window size	
			helper.prepareToWindowScaling(win, cimg);
			helper.scaleImageToWindow(cimg, image_scaled);
			win.set_image(image_scaled);
			win.clear_overlay();

			// Scale and draw updated trackers
			trackerrects.clear();
			for (unsigned int k = 0; k < trackers.size(); k++)
			{
				//cout << "tracker " << k << ": x,y,width,height: " << trackers[k].get_position().left() << ", " << trackers[k].get_position().top() << ", " << trackers[k].get_position().width() << ", " << trackers[k].get_position().height() << endl;					//win.add_overlay(trackers[k].get_position(), rgb_pixel(255-10*k,10*k,5*k));
				trackerrects.push_back(trackers[k].get_position());
			}
			helper.scaleDetectionsToWindow(trackerrects);
			//win.add_overlay(trackerrects, rgb_pixel(255, 0, 0));
			win.add_overlay(dlib::point(-1, 1), rgb_pixel(255, 0, 0), "Frame: " + Helper::ulongToString(ulFrames));
			for (unsigned long l = 0; l < 4; l++)
				win.add_overlay(trackerrects[l], rgb_pixel(255, 0, 0), "Red Robot " + helper.ulongToString(l+1));
			for (unsigned long l = 4; l < 8; l++)
				win.add_overlay(trackerrects[l], rgb_pixel(0, 255, 0), "Green Robot " + helper.ulongToString(l-3));

			helper.completeToWindowScaling(win);*/ // Tracking

			// frame limiter
			while (t1.getPassedTime() < FLIMITER);
			t1.print("PassedTime: "); cout << " last passedTime: "; cout << t1.getLastPassedTime();
			t1.reset();

			// Read from input stream, if there was a keystroke
			if (_kbhit())
			{
				char c = _getch();
				if (c == '+' || c == '-')
				{
					Helper::adjustThreshold(c);
					cout << "Set threshold Sk = " << Helper::threshold << "\n";
					helper.Log("\tSet Threshold Sk =  "); helper.Log((float)Helper::threshold); helper.Log("\n");
				}
				if (c == 'g')
				{
					Helper::toogleGreenFilter();
					cout << "Set green filter Sk = " << Helper::bGreenFilter << "\n";
					helper.Log("\tSet green filter =  "); helper.Log((int)Helper::bGreenFilter); helper.Log("\n");
				}
				if (c == 'c')
				{
					point p;
					cout << "\nWating for click..";
					if (win.get_next_double_click(p))
						cout << "user double clicked on pixel (b, l) : (" << 2*p.y() << ", " << 2*p.x() << ")" << endl;
				}
			}
        }
    }
    catch(serialization_error& e)
    {
        cout << "Error while loading detector" << endl;
        cout << endl << e.what() << endl;
    }
    catch(exception& e)
    {
        cout << e.what() << endl;
    }
}

