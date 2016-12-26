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
 * TraceParser.h
 *
 *  Created on: 29-May-2014
 *      Author: shalini
 */

#include <fstream>
#include <racedetector/UAFDetector.h>
#include <logging/Logger.h>

#include <config.h>

#ifndef TRACEPARSER_H_
#define TRACEPARSER_H_

/*
 * TraceParser class reads the trace file and checks whether each line in the
 * file is a valid operation.
 */
class TraceParser {
public:
//	TraceParser(string traceFileName, Logger *logger);
	TraceParser(string traceFileName, bool withPriority, bool isHexTaskID);
	virtual ~TraceParser();

	std::string traceName;

	// checks whether each line from the trace file is a valid operation
//	int parse(UAFDetector &detector, Logger *logger);
	int parse(UAFDetector &detector);


private:
	ifstream traceFile;

	std::string opRegEx;	 // regex for all valid operations
	std::string posIntRegEx;
	std::string intRegEx; 	 // regex for integers (used as threadIDs)
	std::string hexRegEx;	 // regex for hexadecimal numbers (used as taskID, memory address, etc)
	std::string prefixRegEx; // regex for prefix (if any) of each line in the trace file
	std::string suffixRegEx; // regex for suffix (if any) of each line in the trace file
	std::string finalRegEx;  // regex for a valid line in the trace file

	long long opCount;		// no of operations in the trace
	long long nodeCount;	// no of nodes in the trace/graph
	long long blockCount;	// no of blocks in the trace
};

#endif /* TRACEPARSER_H_ */
