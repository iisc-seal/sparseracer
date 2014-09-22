/*
 * Logger.cpp
 *
 *  Created on: 07-Jun-2014
 *      Author: shalini
 */

#include <logging/Logger.h>
#include <cstdio>
#include <iostream>

void Logger::init (string fileName) {
	logFileName = fileName;
	logFile.open(logFileName.c_str(), std::ios::trunc);

	if (!logFile.is_open()) {
		cout << "ERROR: Cannot open log file " << logFileName << endl;
	}
}

Logger::Logger() {

}

Logger::~Logger() {
	logFile.close();
}

//void Logger::writeLog(string message) {
void Logger::writeLog() {
	string message = streamObject.str();
	logFile << message;
	streamObject.str("");
	streamObject.clear();
}

void Logger::initLog() {
	if (remove(logFileName.c_str()) != 0) {
		logFile.close();
		cout << "ERROR: Deleting log file : " << logFileName << endl;
	}
	else {
		logFile.open(logFileName.c_str(), std::ios::out);
		if (!logFile.is_open())
			cout << "ERROR: Cannot open log file " << logFileName << endl;
	}
}
