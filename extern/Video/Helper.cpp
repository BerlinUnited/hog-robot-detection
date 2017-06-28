#include "Helper.h"

double Helper::threshold = 0;
bool Helper::bGreenFilter = false;

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


std::vector<std::pair<double, dlib::rectangle>> Helper::filterDetections(std::vector < std::pair<double, dlib::rectangle >> detectionpairs)
{
	std::vector<std::pair<double, dlib::rectangle>> temp;
	//// RoboCup2017 ////
	/*for (int i = 0; i < detectionpairs.size(); i++)
	{
		//cout << "H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		if (detectionpairs[i].second.height() > 400 // very basic size filter
			|| (detectionpairs[i].second.height() > 280 && detectionpairs[i].second.bottom() < 760)
			|| detectionpairs[i].second.height() < 60 // size
			|| (detectionpairs[i].second.height() < 80 && detectionpairs[i].second.bottom() > 730)
			|| detectionpairs[i].second.bottom() < 260 // above field
			|| detectionpairs[i].second.bottom() > 866) // below field
		{
			//cout << "size removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		}
		// Greenfilter
		else if (bGreenFilter && (detectionpairs[i].second.bottom() < 304 // above field
			|| detectionpairs[i].second.bottom() < 602
			|| detectionpairs[i].second.bottom() > 930 // below field
			|| detectionpairs[i].second.right() < 52 // left of field
			|| (detectionpairs[i].second.right() < 98 && detectionpairs[i].second.bottom() < 676)
			|| (detectionpairs[i].second.right() < 146 && detectionpairs[i].second.bottom() < 668)
			|| (detectionpairs[i].second.right() < 238 && detectionpairs[i].second.bottom() < 640)
			|| (detectionpairs[i].second.right() < 318 && detectionpairs[i].second.bottom() < 634)
			|| (detectionpairs[i].second.right() < 346 && detectionpairs[i].second.bottom() < 630)
			|| (detectionpairs[i].second.right() < 422 && detectionpairs[i].second.bottom() < 612)
			|| (detectionpairs[i].second.right() < 490 && detectionpairs[i].second.bottom() < 602)

			|| (detectionpairs[i].second.left() > 1826 && detectionpairs[i].second.bottom() < 628) // right of field
			|| (detectionpairs[i].second.left() > 1760 && detectionpairs[i].second.bottom() < 646)
			|| (detectionpairs[i].second.left() > 1670 && detectionpairs[i].second.bottom() < 638)
			|| (detectionpairs[i].second.left() > 1638 && detectionpairs[i].second.bottom() < 638)
			|| (detectionpairs[i].second.left() > 1604 && detectionpairs[i].second.bottom() < 624)
			|| (detectionpairs[i].second.left() > 1546 && detectionpairs[i].second.bottom() < 614)
			|| (detectionpairs[i].second.left() > 1490 && detectionpairs[i].second.bottom() < 602)
			))  // very basic ground filter
		{
			//cout << "green removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		}
		else
			temp.push_back(detectionpairs[i]);
	}*/

	//// EuropeanOpen, Test, Half1 ////
	for (int i = 0; i < detectionpairs.size(); i++)
	{
		//cout << "H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		if (detectionpairs[i].second.height() > 230 // very basic size filter
			|| detectionpairs[i].second.height() < 60
			|| detectionpairs[i].second.bottom() < 280
			|| detectionpairs[i].second.top() < 224)
		{
			//cout << "size removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		}
		// Greenfilter
		else if (bGreenFilter && (detectionpairs[i].second.bottom() < 304 // above field
			|| detectionpairs[i].second.top() < 244
			|| detectionpairs[i].second.bottom() > 930 // below field
			|| detectionpairs[i].second.right() < 52 // left of field
			|| (detectionpairs[i].second.right() < 974 && detectionpairs[i].second.bottom() < 308)
			|| (detectionpairs[i].second.right() < 646 && detectionpairs[i].second.bottom() < 326)
			|| (detectionpairs[i].second.right() < 648 && detectionpairs[i].second.bottom() < 344)
			|| (detectionpairs[i].second.right() < 64 && detectionpairs[i].second.bottom() < 648)
			|| (detectionpairs[i].second.right() < 98 && detectionpairs[i].second.bottom() < 614)
			|| (detectionpairs[i].second.right() < 108 && detectionpairs[i].second.bottom() < 390)
			|| (detectionpairs[i].second.right() < 144 && detectionpairs[i].second.bottom() < 590)
			|| (detectionpairs[i].second.right() < 174 && detectionpairs[i].second.bottom() < 570)
			|| (detectionpairs[i].second.right() < 200 && detectionpairs[i].second.bottom() < 544)
			|| (detectionpairs[i].second.right() < 224 && detectionpairs[i].second.bottom() < 526)
			|| (detectionpairs[i].second.right() < 264 && detectionpairs[i].second.bottom() < 502)
			|| (detectionpairs[i].second.right() < 282 && detectionpairs[i].second.bottom() < 472)
			|| (detectionpairs[i].second.right() < 322 && detectionpairs[i].second.bottom() < 444)
			|| (detectionpairs[i].second.right() < 356 && detectionpairs[i].second.bottom() < 430)
			|| (detectionpairs[i].second.right() < 380 && detectionpairs[i].second.bottom() < 410)
			|| (detectionpairs[i].second.right() < 414 && detectionpairs[i].second.bottom() < 378)
			|| (detectionpairs[i].second.right() < 458 && detectionpairs[i].second.bottom() < 354)
			|| (detectionpairs[i].second.right() < 490 && detectionpairs[i].second.bottom() < 336)

			|| (detectionpairs[i].second.left() > 1872 && detectionpairs[i].second.bottom() < 556) // right of field
			|| (detectionpairs[i].second.left() > 1840 && detectionpairs[i].second.bottom() < 536)
			|| (detectionpairs[i].second.left() > 1814 && detectionpairs[i].second.bottom() < 510)
			|| (detectionpairs[i].second.left() > 1776 && detectionpairs[i].second.bottom() < 482)
			|| (detectionpairs[i].second.left() > 1744 && detectionpairs[i].second.bottom() < 458)
			|| (detectionpairs[i].second.left() > 1700 && detectionpairs[i].second.bottom() < 432)
			|| (detectionpairs[i].second.left() > 1646 && detectionpairs[i].second.bottom() < 402)
			|| (detectionpairs[i].second.left() > 1608 && detectionpairs[i].second.bottom() < 382)
			|| (detectionpairs[i].second.left() > 1582 && detectionpairs[i].second.bottom() < 364)
			|| (detectionpairs[i].second.left() > 1520 && detectionpairs[i].second.bottom() < 338)
			|| (detectionpairs[i].second.left() > 1478 && detectionpairs[i].second.bottom() < 306)
			|| (detectionpairs[i].second.left() > 1460 && detectionpairs[i].second.bottom() < 298)
			))  // very basic ground filter
		{
			//cout << "green removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		}
		else
			temp.push_back(detectionpairs[i]);
	}

	//// RoboCanes, GOPRO4409 ////
	/*for (int i = 0; i < detectionpairs.size(); i++)
	{
		//cout << "H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		if (detectionpairs[i].second.height() > 230 // very basic size filter
			|| detectionpairs[i].second.height() < 65
			|| detectionpairs[i].second.bottom() < 270)
		{
			//cout << "size removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		}
		// Greenfilter 
		else if (bGreenFilter && (detectionpairs[i].second.bottom() < 350 // above field
			//|| detectionpairs[i].second.bottom() > 1700 // below field
			|| (detectionpairs[i].second.right() < 66 && detectionpairs[i].second.bottom() < 543) // left of field
			|| (detectionpairs[i].second.right() < 190 && detectionpairs[i].second.bottom() < 446)
			|| (detectionpairs[i].second.right() < 274 && detectionpairs[i].second.bottom() < 392)
			|| (detectionpairs[i].second.right() < 400 && detectionpairs[i].second.bottom() < 325)
			|| (detectionpairs[i].second.left() > 1885 && detectionpairs[i].second.bottom() < 662)
			|| (detectionpairs[i].second.left() > 1842 && detectionpairs[i].second.bottom() < 622)
			|| (detectionpairs[i].second.left() > 1800 && detectionpairs[i].second.bottom() < 582)
			|| (detectionpairs[i].second.left() > 1724 && detectionpairs[i].second.bottom() < 526)
			|| (detectionpairs[i].second.left() > 1640 && detectionpairs[i].second.bottom() < 454)
			|| (detectionpairs[i].second.left() > 1640 && detectionpairs[i].second.bottom() < 454)
			))  // very basic ground filter
		{
			//cout << "green removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		}
		else
			temp.push_back(detectionpairs[i]);
	}
	//// MakeMunich NaoDevils Final Half 1 ////
	for (int i = 0; i < detectionpairs.size(); i++){
		//cout << "H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		if (detectionpairs[i].second.height() > 230 // very basic size filter
		|| detectionpairs[i].second.height() < 75
		|| detectionpairs[i].second.bottom() < 150)
		{
			//cout << "size removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		}
		// Greenfilter 
		else if (bGreenFilter && (detectionpairs[i].second.bottom() < 218 // above field
		|| detectionpairs[i].second.top() < 155 // above field top
		|| detectionpairs[i].second.bottom() > 866 // below field
		|| (detectionpairs[i].second.right() < 16 && detectionpairs[i].second.bottom() < 652) // left of field
		|| (detectionpairs[i].second.right() < 90 && detectionpairs[i].second.bottom() < 562)
		|| (detectionpairs[i].second.right() < 152 && detectionpairs[i].second.bottom() < 494)
		|| (detectionpairs[i].second.right() < 204 && detectionpairs[i].second.bottom() < 452)
		|| (detectionpairs[i].second.right() < 210 && detectionpairs[i].second.bottom() < 468)
		|| (detectionpairs[i].second.right() < 214 && detectionpairs[i].second.bottom() < 405)
		|| (detectionpairs[i].second.right() < 264 && detectionpairs[i].second.bottom() < 396)
		|| (detectionpairs[i].second.right() < 304 && detectionpairs[i].second.bottom() < 362)
		|| (detectionpairs[i].second.right() < 350 && detectionpairs[i].second.bottom() < 326)
		|| (detectionpairs[i].second.right() < 406 && detectionpairs[i].second.bottom() < 286)
		|| (detectionpairs[i].second.left() > 1444 && detectionpairs[i].second.bottom() < 234)
		|| (detectionpairs[i].second.left() > 1512 && detectionpairs[i].second.bottom() < 280)
		|| (detectionpairs[i].second.left() > 1590 && detectionpairs[i].second.bottom() < 336)
		|| (detectionpairs[i].second.left() > 1628 && detectionpairs[i].second.bottom() < 368)
		|| (detectionpairs[i].second.left() > 1684 && detectionpairs[i].second.bottom() < 414)
		|| (detectionpairs[i].second.left() > 1748 && detectionpairs[i].second.bottom() < 456)
		|| (detectionpairs[i].second.left() > 1822 && detectionpairs[i].second.bottom() < 534)
		))  // very basic ground filter
		{
			//cout << "green removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		}
		else
			temp.push_back(detectionpairs[i]);
	}*/
	cout << endl << endl;
	return temp;
}

std::vector<std::pair<double, dlib::rectangle>> Helper::filterPoseDetections(std::vector < std::pair<double, dlib::rectangle >> detectionpairs, unsigned long ulFrames, int iPose)
{
	std::vector<std::pair<double, dlib::rectangle>> temp;

	//// EuropeanOpen, Test, Half1 ////
	for (int i = 0; i < detectionpairs.size(); i++)
	{
		//cout << "H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		if (detectionpairs[i].second.height() > 230 // very basic size filter
			|| detectionpairs[i].second.height() < 60
			|| detectionpairs[i].second.bottom() < 280
			|| detectionpairs[i].second.top() < 224)
		{
			//cout << "size removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		}
		// Greenfilter
		else if (bGreenFilter && (detectionpairs[i].second.bottom() < 304 // above field
			|| detectionpairs[i].second.top() < 244
			|| detectionpairs[i].second.bottom() > 930 // below field
			|| detectionpairs[i].second.right() < 52 // left of field
			|| (detectionpairs[i].second.right() < 974 && detectionpairs[i].second.bottom() < 308)
			|| (detectionpairs[i].second.right() < 646 && detectionpairs[i].second.bottom() < 326)
			|| (detectionpairs[i].second.right() < 648 && detectionpairs[i].second.bottom() < 344)
			|| (detectionpairs[i].second.right() < 64 && detectionpairs[i].second.bottom() < 648)
			|| (detectionpairs[i].second.right() < 98 && detectionpairs[i].second.bottom() < 614)
			|| (detectionpairs[i].second.right() < 108 && detectionpairs[i].second.bottom() < 390)
			|| (detectionpairs[i].second.right() < 144 && detectionpairs[i].second.bottom() < 590)
			|| (detectionpairs[i].second.right() < 174 && detectionpairs[i].second.bottom() < 570)
			|| (detectionpairs[i].second.right() < 200 && detectionpairs[i].second.bottom() < 544)
			|| (detectionpairs[i].second.right() < 224 && detectionpairs[i].second.bottom() < 526)
			|| (detectionpairs[i].second.right() < 264 && detectionpairs[i].second.bottom() < 502)
			|| (detectionpairs[i].second.right() < 282 && detectionpairs[i].second.bottom() < 472)
			|| (detectionpairs[i].second.right() < 322 && detectionpairs[i].second.bottom() < 444)
			|| (detectionpairs[i].second.right() < 356 && detectionpairs[i].second.bottom() < 430)
			|| (detectionpairs[i].second.right() < 380 && detectionpairs[i].second.bottom() < 410)
			|| (detectionpairs[i].second.right() < 414 && detectionpairs[i].second.bottom() < 378)
			|| (detectionpairs[i].second.right() < 458 && detectionpairs[i].second.bottom() < 354)
			|| (detectionpairs[i].second.right() < 490 && detectionpairs[i].second.bottom() < 336)

			|| (detectionpairs[i].second.left() > 1872 && detectionpairs[i].second.bottom() < 556) // right of field
			|| (detectionpairs[i].second.left() > 1840 && detectionpairs[i].second.bottom() < 536)
			|| (detectionpairs[i].second.left() > 1814 && detectionpairs[i].second.bottom() < 510)
			|| (detectionpairs[i].second.left() > 1776 && detectionpairs[i].second.bottom() < 482)
			|| (detectionpairs[i].second.left() > 1744 && detectionpairs[i].second.bottom() < 458)
			|| (detectionpairs[i].second.left() > 1700 && detectionpairs[i].second.bottom() < 432)
			|| (detectionpairs[i].second.left() > 1646 && detectionpairs[i].second.bottom() < 402)
			|| (detectionpairs[i].second.left() > 1608 && detectionpairs[i].second.bottom() < 382)
			|| (detectionpairs[i].second.left() > 1582 && detectionpairs[i].second.bottom() < 364)
			|| (detectionpairs[i].second.left() > 1520 && detectionpairs[i].second.bottom() < 338)
			|| (detectionpairs[i].second.left() > 1478 && detectionpairs[i].second.bottom() < 306)
			|| (detectionpairs[i].second.left() > 1460 && detectionpairs[i].second.bottom() < 298)
			))  // very basic ground filter
		{
			//cout << "green removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
		}
		else if (iPose == 1 && ((detectionpairs[i].second.right() < 390 && detectionpairs[i].second.bottom() < 572) // links oben kein Standing
			|| (detectionpairs[i].second.left() > 1632 && detectionpairs[i].second.bottom() < 480)
			))
		{

		}
		else if (iPose == 3 && ( // kein Lying
			(ulFrames > 2860 && ulFrames < 2900)
			|| (ulFrames > 3010 && ulFrames < 3150)
			|| (ulFrames > 3350)
			))
		{

		}
		else if (iPose ==4  && ulFrames > 2800 && detectionpairs[i].second.right() < 386 // links kein Goalie
			)
		{

		}
		else
			temp.push_back(detectionpairs[i]);
	}

	// Posenfilter
	// if (ulFrames > 2860 && ulFrames < 2900) links kein Lying
	// if (ulFrames > 3010 && ulFrames < 3150) links kein Lying
	// if (ulFrames > 3350 && ulFrames < 3150) links kein Lying

	//// RoboCanes, GOPRO4409 ////
	/*for (int i = 0; i < detectionpairs.size(); i++)
	{
	//cout << "H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
	if (detectionpairs[i].second.height() > 230 // very basic size filter
	|| detectionpairs[i].second.height() < 65
	|| detectionpairs[i].second.bottom() < 270)
	{
	//cout << "size removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
	}
	// Greenfilter
	else if (bGreenFilter && (detectionpairs[i].second.bottom() < 350 // above field
	//|| detectionpairs[i].second.bottom() > 1700 // below field
	|| (detectionpairs[i].second.right() < 66 && detectionpairs[i].second.bottom() < 543) // left of field
	|| (detectionpairs[i].second.right() < 190 && detectionpairs[i].second.bottom() < 446)
	|| (detectionpairs[i].second.right() < 274 && detectionpairs[i].second.bottom() < 392)
	|| (detectionpairs[i].second.right() < 400 && detectionpairs[i].second.bottom() < 325)
	|| (detectionpairs[i].second.left() > 1885 && detectionpairs[i].second.bottom() < 662)
	|| (detectionpairs[i].second.left() > 1842 && detectionpairs[i].second.bottom() < 622)
	|| (detectionpairs[i].second.left() > 1800 && detectionpairs[i].second.bottom() < 582)
	|| (detectionpairs[i].second.left() > 1724 && detectionpairs[i].second.bottom() < 526)
	|| (detectionpairs[i].second.left() > 1640 && detectionpairs[i].second.bottom() < 454)
	|| (detectionpairs[i].second.left() > 1640 && detectionpairs[i].second.bottom() < 454)
	))  // very basic ground filter
	{
	//cout << "green removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
	}
	else
	temp.push_back(detectionpairs[i]);
	}
	//// MakeMunich NaoDevils Final Half 1 ////
	for (int i = 0; i < detectionpairs.size(); i++){
	//cout << "H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
	if (detectionpairs[i].second.height() > 230 // very basic size filter
	|| detectionpairs[i].second.height() < 75
	|| detectionpairs[i].second.bottom() < 150)
	{
	//cout << "size removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
	}
	// Greenfilter
	else if (bGreenFilter && (detectionpairs[i].second.bottom() < 218 // above field
	|| detectionpairs[i].second.top() < 155 // above field top
	|| detectionpairs[i].second.bottom() > 866 // below field
	|| (detectionpairs[i].second.right() < 16 && detectionpairs[i].second.bottom() < 652) // left of field
	|| (detectionpairs[i].second.right() < 90 && detectionpairs[i].second.bottom() < 562)
	|| (detectionpairs[i].second.right() < 152 && detectionpairs[i].second.bottom() < 494)
	|| (detectionpairs[i].second.right() < 204 && detectionpairs[i].second.bottom() < 452)
	|| (detectionpairs[i].second.right() < 210 && detectionpairs[i].second.bottom() < 468)
	|| (detectionpairs[i].second.right() < 214 && detectionpairs[i].second.bottom() < 405)
	|| (detectionpairs[i].second.right() < 264 && detectionpairs[i].second.bottom() < 396)
	|| (detectionpairs[i].second.right() < 304 && detectionpairs[i].second.bottom() < 362)
	|| (detectionpairs[i].second.right() < 350 && detectionpairs[i].second.bottom() < 326)
	|| (detectionpairs[i].second.right() < 406 && detectionpairs[i].second.bottom() < 286)
	|| (detectionpairs[i].second.left() > 1444 && detectionpairs[i].second.bottom() < 234)
	|| (detectionpairs[i].second.left() > 1512 && detectionpairs[i].second.bottom() < 280)
	|| (detectionpairs[i].second.left() > 1590 && detectionpairs[i].second.bottom() < 336)
	|| (detectionpairs[i].second.left() > 1628 && detectionpairs[i].second.bottom() < 368)
	|| (detectionpairs[i].second.left() > 1684 && detectionpairs[i].second.bottom() < 414)
	|| (detectionpairs[i].second.left() > 1748 && detectionpairs[i].second.bottom() < 456)
	|| (detectionpairs[i].second.left() > 1822 && detectionpairs[i].second.bottom() < 534)
	))  // very basic ground filter
	{
	//cout << "green removed -> H: " << detectionpairs[i].second.height() << "; (b,l) :(" << detectionpairs[i].second.bottom() << "," << detectionpairs[i].second.left() << ")" << endl;
	}
	else
	temp.push_back(detectionpairs[i]);
	}*/
	cout << endl << endl;
	return temp;
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

void Helper::prepareToWindowScaling(dlib::image_window& win, const dlib::cv_image<dlib::bgr_pixel>& imageToScale)
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
}

void Helper::scaleImageToWindow(const dlib::cv_image<dlib::bgr_pixel>& imgageIn, dlib::array2d<unsigned char>& imageOut)
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

bool Helper::Log(unsigned long WriteToLog)
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

void Helper::toogleGreenFilter()
{
	Helper::bGreenFilter = !Helper::bGreenFilter;
}


// Converts
std::string Helper::ulongToString(unsigned long ul)
{
	std::stringstream ss;
	ss << ul;
	return ss.str();
}

std::string Helper::doubleToString(double d)
{
	std::stringstream ss;
	ss << d;
	return ss.str();
}

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
