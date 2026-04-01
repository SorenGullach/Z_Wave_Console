// Z_Wave_Stack_PI.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <ostream>
#include <istream>

#include "Z_Wave_Stack_PI.h"

#include "Logging.h"
#include "ZWaveAPI.h"

using namespace std;

Logging Log; // a global var used for logging
ZWaveAPI ZW;

int main()
{
#if defined(__linux__) 
	if (!ZW.OpenPort("/dev/ttyUSB0")) // or /dev/serialIO /dev/ttyUSB0
#elif defined(_WIN32)
	if (!ZW.OpenPort("COM3"))
#else
	#pragma message("Platform not supported")
#endif
	{
		cout << "Comport not opened. press to exit" << endl;
		cin.get();
		return 1;
	}
//	ZW.Test();
	ZW.Start(); // start the Z-Wave stack.

	cout << "Hello CMake. press to exit" << endl;
	cin.get();
	return 0;
}


