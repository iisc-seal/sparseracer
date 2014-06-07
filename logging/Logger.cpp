/*
 * Logger.cpp
 *
 *  Created on: 07-Jun-2014
 *      Author: shalini
 */

#include <logging/Logger.h>
#include <cstdio>
#include <iostream>

Logger::Logger(string fileName) {
	logFileName = fileName;
	logFile.open(logFileName.c_str(), std::ofstream::app);

	if (!logFile.is_open()) {
		cout << "ERROR: Cannot open log file " << logFileName << endl;
	}
}

Logger::~Logger() {
	logFile.close();
}

void Logger::writeLog(string message) {
	logFile << message << endl;
}

void Logger::initLog() {
	if (remove(logFileName.c_str()) != 0) {
		logFile.close();
		cout << "ERROR: Deleting log file : " << logFileName << endl;
	}
	else {
		logFile.open(logFileName.c_str(), std::ofstream::out);
		if (!logFile.is_open())
			cout << "ERROR: Cannot open log file " << logFileName << endl;
	}
}
