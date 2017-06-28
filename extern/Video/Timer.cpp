#include "Timer.h"

Timer::Timer()
{
	t1 = std::chrono::high_resolution_clock::now();
	t2 = std::chrono::high_resolution_clock::now();
	lastPassedTime = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
}

void Timer::reset()
{
	t1 = std::chrono::high_resolution_clock::now();
	t2 = std::chrono::high_resolution_clock::now();
}

void Timer::print(std::string strMessage)
{	
	std::cout << strMessage.c_str() << " " << getPassedTime() << "\n";
}

long Timer::getPassedTime()
{
	t2 = std::chrono::high_resolution_clock::now();
	lastPassedTime = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
	return static_cast<long>(lastPassedTime.count());
}

long Timer::getLastPassedTime()
{
	return static_cast<long>(lastPassedTime.count());
}

Timer::~Timer()
{
}
