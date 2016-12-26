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
 * main.cpp
 *
 *  Created on: 29-May-2014
 *      Author: shalini
 */

#include <iostream>
#include <cstdlib>
#include <parser/TraceParser.h>
#include <racedetector/UAFDetector.h>
#include <logging/Logger.h>
#include <time.h>
#include <cstring>

#include <debugconfig.h>

std::string convertTime(clock_t startTime, clock_t endTime) {
	std::string time;
	std::ostringstream stream;
	unsigned long long elapsed_secs = (endTime - startTime)/CLOCKS_PER_SEC;

	if (elapsed_secs == 0) {
		double secs = (double) (endTime - startTime)/CLOCKS_PER_SEC;

		double milliseconds = secs * 1000;

		stream << milliseconds << "ms";
		time = stream.str();
	} else {
		double remaining = (double) (endTime - startTime)/CLOCKS_PER_SEC;
		stream << "total seconds: " << remaining << " ";
		int hrs = remaining / 3600;
		//unsigned long long remaining = elapsed_secs % 3600;
		remaining = (remaining/3600 - hrs) * 3600;

		int mins = remaining / 60;
		remaining = (remaining/60 - mins) * 60;

		int secs = remaining;
		int milliseconds = (remaining - secs) * 1000;

		stream << hrs << "h" << mins << "m" << secs << "s" << milliseconds << "ms";
		time = stream.str();
	}

	return time;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "ERROR: Missing input\n";
		cout << "Input trace-file name as argument 1\n";
		return -1;
	}

	string traceFileName = argv[1];


	bool outputAllConflictingOps = false;
	bool filterUAFInput = false, filterRaceInput = false;
	bool runDetectorOnTrace = false;
	bool multithreadedHB = false;
	bool richHB = false;
	bool withPriority = true, hexTaskID = false;;
	string outputUAFAllOpsFileName, outputUAFAllOpsUniqueFileName,
		   outputRacesAllOpsFileName, outputRacesAllOpsUniqueFileName;
	string filterUAFInputFileName, filterUAFOutputFileName,
		   filterRaceInputFileName, filterRaceOutputFileName;
	for (int i = 2; i < argc; i++) {
		if (strcmp(argv[i], "-no-priority") == 0) {
			withPriority = false;
		} else if (strcmp(argv[i], "-hex") == 0) {
			hexTaskID = true;
		} else if (strcmp(argv[i], "-a") == 0) {
			outputAllConflictingOps = true;
			outputUAFAllOpsFileName = traceFileName + ".uaf.allconflictingops";
			outputUAFAllOpsUniqueFileName = traceFileName + ".uaf.allconflictingops.unique";
			outputRacesAllOpsFileName = traceFileName + ".race.allconflictingops";
			outputRacesAllOpsUniqueFileName = traceFileName + ".race.allconflictingops.unique";
		} else if (strcmp(argv[i], "-fu") == 0) {
			filterUAFInput = true;
			filterUAFInputFileName = argv[i+1];
			filterUAFOutputFileName = filterUAFInputFileName + ".filtered";
			i++;
		} else if (strcmp(argv[i], "-fr") == 0) {
			filterRaceInput = true;
			filterRaceInputFileName = argv[i+1];
			filterRaceOutputFileName = filterRaceInputFileName + ".filtered";
		} else if (strcmp(argv[i], "-rm") == 0) {
			runDetectorOnTrace = true;
			multithreadedHB = true;
		} else if (strcmp(argv[i], "-rr") == 0) {
			runDetectorOnTrace = true;
			richHB = true;
		} else {
			cout << "ERROR: Invalid argument " << argv[i] << "\n";
			exit(0);
		}
	}

	TraceParser parser(traceFileName, withPriority, hexTaskID);
	UAFDetector detectorObj;

	clock_t totalStart, totalEnd, tStart, tEnd, transitiveStart, transitiveEnd;
	totalStart = clock();
	tStart = totalStart;
	transitiveStart = totalStart;
	int retParse = parser.parse(detectorObj);
	if (retParse == -2) {
		cout << "Giving up! More than 15k nodes\n";
		return -1;
	} else if (retParse < 0) {
		cout << "ERROR while parsing the trace\n";
		return -1;
	}

#ifdef TRACEDEBUG
	cout << "map size: " << detectorObj.opIDMap.size() << endl;
#endif
	tEnd = clock();
	cout << "Time taken for parsing: " << convertTime(tStart, tEnd) << "\n";

	if (outputAllConflictingOps) {
		detectorObj.outputAllConflictingOps(outputUAFAllOpsFileName, outputUAFAllOpsUniqueFileName,
				outputRacesAllOpsFileName, outputRacesAllOpsUniqueFileName);
	}

	if (!filterUAFInput  && !filterRaceInput && !runDetectorOnTrace)
		return 0;

	tStart = clock();
	if (detectorObj.addEdges() < 0) {
		cout << "ERROR while constructing HB Graph\n";
		return -1;
	}
	tEnd = clock();
	transitiveEnd = tEnd;
	cout << "Time taken for transitive closure: " << convertTime(tStart, tEnd) << "\n";
	cout << "Time - parsing + transitive closure: " << convertTime(transitiveStart, transitiveEnd) << "\n";

	detectorObj.initLog(traceFileName);
	if (runDetectorOnTrace) {
		tStart = clock();
		cout << "\nFinding UAF\n";
#ifdef NODERACES
		int retfindUAF = detectorObj.findUAFUsingNodes();
#else
		int retfindUAF = detectorObj.findUAF();
#endif

		if (retfindUAF == -1) {
			cout << "ERROR: While finding UAF\n";
			return -1;
		} else if (retfindUAF == 0) {
			cout << "No UAF in the trace\n";
		} else {
			cout << "OUTPUT: Found " << retfindUAF << " UAFs\n";
		}
		tEnd = clock();
		cout << "Time taken for finding UAF: " << convertTime(tStart, tEnd) << "\n";

#ifdef DATARACE
		tStart = clock();
		cout << "\nFinding data races\n";
#ifdef NODERACES
		int retfindRace = detectorObj.findDataRacesUsingNodes();
#else
		int retfindRace = detectorObj.findDataRaces();
#endif

		if (retfindRace == -1) {
			cout << "ERROR: While finding Data races\n";
			return -1;
		} else if (retfindRace == 0) {
			cout << "No data races in the trace\n";
		} else {
			cout << "OUTPUT: Found " << retfindRace << " races\n";
		}
		tEnd = clock();
		cout << "Time taken for finding races: " << convertTime(tStart, tEnd) << "\n";
#endif

		if (multithreadedHB && !richHB)
			detectorObj.log(true);
		else if (!multithreadedHB && richHB)
			detectorObj.log(false);
	}

	if (filterUAFInput) {
		if (detectorObj.filterInput(filterUAFInputFileName, filterUAFOutputFileName) < 0) {
			cout << "ERROR: While filtering uafs\n";
			return -1;
		}
	}
	if (filterRaceInput) {
		if (detectorObj.filterInput(filterRaceInputFileName, filterRaceOutputFileName) < 0) {
			cout << "ERROR: While filtering races\n";
			return -1;
		}
	}

	totalEnd = clock();
	cout << "Total time taken: " << convertTime(totalStart, totalEnd) << "\n";
	return 0;
}
