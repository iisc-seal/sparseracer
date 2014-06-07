/*
 * Logger.h
 *
 *  Created on: 07-Jun-2014
 *      Author: shalini
 */

#include <fstream>

using namespace std;

#ifndef LOGGER_H_
#define LOGGER_H_

class Logger {
public:
	Logger(string fileName);
	virtual ~Logger();

	void writeLog(string message);
	void initLog();

private:
	ofstream logFile;
	string logFileName;
};

#endif /* LOGGER_H_ */
