#include<syslog.h>
#include<stdio.h>
#include <pthread.h>
//#include "prthread.h"

void mopInstrument(long long address, int typeSize, char* type, char* debugLoc, char *fName){
    printf("access (??, %p) of type %s (typesize %d) at %s\n", address, type, typeSize, debugLoc);
}

void fInstrument(char *fName, int entering){
  if(entering)
    syslog(LOG_DEBUG, "entering(%lu, %s)", pthread_self(), fName);
  else
    syslog(LOG_DEBUG, "exiting(%lu, %s)", pthread_self(), fName);
}
