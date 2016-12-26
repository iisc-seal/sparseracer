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
 * Logger.h
 *
 *  Created on: 07-Jun-2014
 *      Author: shalini
 */

#include <fstream>
#include <sstream>

using namespace std;

#ifndef LOGGER_H_
#define LOGGER_H_

class Logger {
public:
	void init(string fileName);
	Logger();
	virtual ~Logger();

	void writeLog(string message);
	void writeLog();
	void initLog();

	std::stringstream streamObject;
private:
	ofstream logFile;
	string logFileName;
};

#endif /* LOGGER_H_ */
