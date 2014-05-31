/*
 * UAFDetector.h
 *
 *  Created on: 30-May-2014
 *      Author: shalini
 */

#include <map>
#include <string>

using namespace std;

#ifndef UAFDETECTOR_H_
#define UAFDETECTOR_H_

class UAFDetector {
public:
	UAFDetector();
	virtual ~UAFDetector();

	// Stores threadID, taskID and opType of an operation.
	struct opDetails {
		unsigned long int threadID;
		string taskID;
		string opType;
	};

	map<unsigned long int, struct opDetails> opIDMap;
};

#endif /* UAFDETECTOR_H_ */
