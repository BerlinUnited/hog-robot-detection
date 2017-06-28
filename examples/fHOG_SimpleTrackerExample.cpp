// Author: Dominik Krienelke
// Contact: dominik.krienelke@gmail.com
// Code based on dlib fHOG implementation, original program based on dlib video_tracking_ex.cpp example
// Required input is a directory with a close succession of video frames (or a video as in fHOG_FullNaoVideoDetector.cpp)

/*

    This example shows how to use the correlation_tracker from the dlib C++ library.  This
    object lets you track the position of an object as it moves from frame to frame in a
    video sequence.  To use it, you give the correlation_tracker the bounding box of the
    object you want to track in the current video frame.  Then it will identify the
    location of the object in subsequent frames.

    In this particular example, we are going to run on the video sequence that comes with
    dlib, which can be found in the examples/video_frames folder.  This video shows a juice
    box sitting on a table and someone is waving the camera around.  The task is to track the
    position of the juice box as the camera moves around.
*/

#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_io.h>
#include <dlib/dir_nav.h>


using namespace dlib;
using namespace std;

int main(int argc, char** argv) try
{
	if (argc != 2)
	{
		cout << "Call this program like this: " << endl;
		cout << "./video_tracking_ex ../video_frames" << endl;
		return 1;
	}

	// Get the list of video frames.  
	std::vector<file> files = get_files_in_directory_tree(argv[1], match_ending(".jpg"));
	std::sort(files.begin(), files.end());
	if (files.size() == 0)
	{
		cout << "No images found in " << argv[1] << endl;
		return 1;
	}

	// Load the first frame.  
	array2d<unsigned char> img;
	load_image(img, files[0]);
	// Now create a tracker and start a track on the juice box.  If you look at the first
	// frame you will see that the juice box is centered at pixel point(92,110) and 38
	// pixels wide and 86 pixels tall.
	correlation_tracker tracker, tracker2, tracker3;

	tracker.start_track(img, rectangle(598, 363, 684, 469));
	tracker2.start_track(img, rectangle(322, 121, 343, 162));

    // Now run the tracker.  All we have to do is call tracker.update() and it will keep
    // track of the juice box!
    image_window win;
    for (unsigned long i = 0; i < files.size(); ++i)
    {
        load_image(img, files[i]);
		win.set_image(img);

		if (i > 0)
		{
			tracker.update(img);
			tracker2.update(img);
			tracker3.update(img);
		}
		else
		{
			win.add_overlay(tracker.get_position(), rgb_pixel(255, 0, 0));
			win.add_overlay(tracker2.get_position(), rgb_pixel(0, 0, 255));
			cout << "please double click topleft and bottomright corner for additional tracker before tracking is started... ";
			point p1,p2;
			win.get_next_double_click(p1); cout << "topleft set... ";
			win.get_next_double_click(p2); cout << "bottomright set.\n";
			tracker3.start_track(img, rectangle(p1, p2));
		}

        win.clear_overlay(); 
		win.add_overlay(tracker.get_position(), rgb_pixel(255, 0, 0));
		win.add_overlay(tracker2.get_position(), rgb_pixel(0, 0, 255));
		win.add_overlay(tracker3.get_position(), rgb_pixel(0, 255, 0));

		cout << "tracker 1: x,y,width,height: " << tracker.get_position().left() << ", " << tracker.get_position().top() << ", " << tracker.get_position().width() << ", " << tracker.get_position().height() << endl;
		cout << "tracker 2: x,y,width,height: " << tracker2.get_position().left() << ", " << tracker2.get_position().top() << ", " << tracker2.get_position().width() << ", " << tracker2.get_position().height() << endl;
		cout << "tracker 3: x,y,width,height: " << tracker3.get_position().left() << ", " << tracker3.get_position().top() << ", " << tracker3.get_position().width() << ", " << tracker3.get_position().height() << endl;
		cout << "hit enter to process next frame (or 'r,g,b' and enter to set a new tracker)" << endl;		
		
		char c = cin.get();
		if (c == 'r' || c == 'g' || c == 'b'){
			point p1, p2;
			cout << "Please set new tracker before tracking is continued... ";
			win.get_next_double_click(p1); cout << "new topleft set... ";
			win.get_next_double_click(p2); cout << "new bottomright set.\n";
			switch (c)
			{
				case 'r': tracker.start_track(img, rectangle(p1, p2)); break;
				case 'b': tracker2.start_track(img, rectangle(p1, p2)); break;
				case 'g': tracker3.start_track(img, rectangle(p1, p2)); break;
			}
		}
    }
}
catch (std::exception& e)
{
    cout << e.what() << endl;
}

