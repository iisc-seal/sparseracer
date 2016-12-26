/*
Copyright 2014-2016 Anirudh Santhiar, Shalini Kaleeswaran and Aditya
Kanade from the Software Engineering and Analysis Lab, Department of
Computer Science and Automation, Indian Institute of Science.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
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

void Logger::writeLog(string message) {
	logFile << message;
}
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
