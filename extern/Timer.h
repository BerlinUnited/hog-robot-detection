#pragma once

#include "Global.h"


class Timer
{
public:
	Timer();
	~Timer();

	void reset();
	void print(std::string strMessage);
	long getPassedTime();
	long getLastPassedTime();	

private:
	std::chrono::system_clock::time_point t1;
	std::chrono::system_clock::time_point t2;
	std::chrono::milliseconds lastPassedTime;
};

