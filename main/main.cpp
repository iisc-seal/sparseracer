/*
 * main.cpp
 *
 *  Created on: 29-May-2014
 *      Author: shalini
 */

#include <iostream>
#include <parser/TraceParser.h>
#include <racedetector/UAFDetector.h>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "ERROR: Missing input:- trace-file name\n";
		return -1;
	}

	TraceParser parser(argv[1]);
	UAFDetector detectorObj;

	if (parser.parse(detectorObj) < 0) {
		cout << "ERROR\n";
		return -1;
	}

	cout << "map size: " << detectorObj.opIDMap.size() << endl;
}
