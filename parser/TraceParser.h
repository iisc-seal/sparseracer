/*
 * TraceParser.h
 *
 *  Created on: 29-May-2014
 *      Author: shalini
 */

#include <fstream>
#include <racedetector/UAFDetector.h>
using namespace std;

#ifndef TRACEPARSER_H_
#define TRACEPARSER_H_

/*
 * TraceParser class reads the trace file and checks whether each line in the
 * file is a valid operation.
 */
class TraceParser {
public:
	TraceParser(char* traceFileName);
	virtual ~TraceParser();

	// checks whether each line from the trace file is a valid operation
	int parse(UAFDetector detector);
//	int parse();


private:
	ifstream traceFile;

	string opRegEx,		// regex for all valid operations
		   intRegEx, 	// regex for integers (used as threadIDs)
		   hexRegEx,	// regex for hexadecimal numbers (used as taskID, memory address, etc)
		   prefixRegEx, // regex for prefix (if any) of each line in the trace file
		   finalRegEx;  // regex for a valid line in the trace file

	int opCount;		// no of operations in the trace
};

#endif /* TRACEPARSER_H_ */
