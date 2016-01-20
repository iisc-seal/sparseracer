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
	TraceParser(string traceFileName);
	virtual ~TraceParser();

	std::string traceName;

	// checks whether each line from the trace file is a valid operation
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
};

#endif /* TRACEPARSER_H_ */
