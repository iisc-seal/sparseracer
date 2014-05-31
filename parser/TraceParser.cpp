/*
 * TraceParser.cpp
 *
 *  Created on: 29-May-2014
 *      Author: shalini
 */

#include "TraceParser.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <boost/regex.hpp>
#include <racedetector/UAFDetector.h>

/*
 * Constructor for TraceParser class.
 * Takes as argument name of the tracefile
 * Creates regex for a valid operation.
 */
TraceParser::TraceParser(char* traceFileName) {
	traceFile.open(traceFileName, ios_base::in);
	if (!traceFile.is_open())
		cout << "Cannot open trace file\n";

	// The prefix regular expression
	prefixRegEx = "[0-9]+ *:";

	intRegEx = "[0-9]+";
	hexRegEx = "0[xX][0-9a-fA-F]+";

	// The operation regular expression.
	opRegEx = " *(threadinit) *\\( *(" + intRegEx + ") *\\) *" + "|" +
			  " *(threadexit) *\\( *(" + intRegEx + ") *\\) *" + "|" +
			  " *(fork) *\\( *("       + intRegEx + ") *, *(" 	   + intRegEx + ") *\\) *" + "|" +
			  " *(join) *\\( *("       + intRegEx + ") *, *(" 	   + intRegEx + ") *\\) *" + "|" +
			  " *(enterloop) *\\( *("  + intRegEx + ") *\\) *" + "|" +
			  " *(exitloop) *\\( *("   + intRegEx + ") *\\) *" + "|" +
			  " *(enq) *\\( *("        + intRegEx + ") *, *(" 	   + hexRegEx + ") *, *("
			  	  	  	  	  	  	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(deq) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(end) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(pause) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(resume) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(acquire) *\\( *("    + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(release) *\\( *("    + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(alloc) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	    		  	   + hexRegEx + ") *\\) *" + "|" +
			  " *(free) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	   	   	   	  	   + hexRegEx + ") *\\) *" + "|" +
			  " *(inc) *\\( *(" 	   + intRegEx + ") *, *\\( *(" + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	  	   	   	  	   + hexRegEx + ") *\\) *\\) *" + "|" +
			  " *(dec) *\\( *(" 	   + intRegEx + ") *, *\\( *(" + hexRegEx + ") *, *("
			  	  	  	  	  	  	  	  	  	  	   	   	   	   + hexRegEx + ") *\\) *\\) *" + "|" +
			  " *(read) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *" + "|" +
			  " *(write) *\\( *(" 	   + intRegEx + ") *, *(" 	   + hexRegEx + ") *\\) *";

	// Regular expression for the entire line
	finalRegEx = "^" + prefixRegEx + "(" + opRegEx + ")" + "$";
}

TraceParser::~TraceParser() {
}

int TraceParser::parse(UAFDetector detector) {
//int TraceParser::parse() {
	string line;
	boost::regex reg;
	boost::cmatch matches;

//	cout << finalRegEx << endl;
	try {
		// create boost regex from finalRegEx, ignoring case
		reg.assign(finalRegEx, boost::regex_constants::icase);
	}
	catch (boost::regex_error& e) {
		cout << finalRegEx << " is not a valid regular expression: \""
			 << e.what() << "\"" << endl;
		return -1;
	}

	opCount = 0;
	bool flag;
	int typePos, threadPos;
	while (traceFile >> line) {
		// Check whether the line is a valid line according to finalRegEx
		if (!boost::regex_match(line.c_str(), matches, reg))
			cout << line << endl;
		else {
			opCount++;
			cout << line << endl;
			flag = false;
			for (int i=1; i < matches.size(); i++) {
				string match(matches[i].first, matches[i].second);
				if (!match.empty() && match.compare(" ") != 0) {
					cout << "\tmatches[" << i << "] = " << match << endl;
					if (i != 1 && !flag) {
						flag = true;
						typePos = i;

						UAFDetector::opDetails opdetails;
						opdetails.opType = match;
						for (int j = typePos; j < matches.size(); j++) {
							string m1(matches[j].first, matches[j].second);
							if (!m1.empty() && m1.compare(" ") != 0) {
								opdetails.threadID = atoi(m1.c_str());
								break;
							}
						}
						if (match.compare("threadinit") == 0) {
							opdetails.opType = "threadinit";
							for (int j = typePos; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									opdetails.threadID = atoi(m1.c_str());
									threadPos = j;
									break;
								}
							}
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("threadexit") == 0) {
							opdetails.opType = "threadexit";
							for (int j = typePos; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									opdetails.threadID = atoi(m1.c_str());
									threadPos = j;
									break;
								}
							}
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("enterloop") == 0) {
							opdetails.opType = "enterloop";
							for (int j = typePos; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									opdetails.threadID = atoi(m1.c_str());
									threadPos = j;
									break;
								}
							}
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("exitloop") == 0) {
							opdetails.opType = "exitloop";
							for (int j = typePos; j < matches.size(); j++) {
								string m1(matches[j].first, matches[j].second);
								if (!m1.empty() && m1.compare(" ") != 0) {
									opdetails.threadID = atoi(m1.c_str());
									threadPos = j;
									break;
								}
							}
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("fork") == 0) {
							opdetails.opType = "fork";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("join") == 0) {
							opdetails.opType = "join";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("enq") == 0) {
							opdetails.opType = "enq";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("deq") == 0) {
							opdetails.opType = "deq";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("end") == 0) {
							opdetails.opType = "end";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("pause") == 0) {
							opdetails.opType = "pause";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("resume") == 0) {
							opdetails.opType = "resume";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("acquire") == 0) {
							opdetails.opType = "acquire";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("release") == 0) {
							opdetails.opType = "release";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("inc") == 0) {
							opdetails.opType = "inc";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("dec") == 0) {
							opdetails.opType = "dec";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("alloc") == 0) {
							opdetails.opType = "alloc";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("free") == 0) {
							opdetails.opType = "free";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("read") == 0) {
							opdetails.opType = "read";
							detector.opIDMap[opCount] = opdetails;
						} else if (match.compare("write") == 0) {
							opdetails.opType = "write";
							detector.opIDMap[opCount] = opdetails;
						}
					}
				}
			}
		}
	}

	cout << "No of ops: " << opCount << endl;
	return 0;
}
