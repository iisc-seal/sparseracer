#include<syslog.h>
#include<stdio.h>

//#include "prthread.h"

void mopInstrument(int address, int typeSize, char* type, char* debugLoc, char *fName){
  //if(strstr(type, "DOMMediaStream")){
    syslog(LOG_INFO, "access (??, %p) of type %s (typesize %d) at %s\n", address, type, typeSize, debugLoc);
    //syslog(LOG_DEBUG, "access(%lu, %x)", pthread_self(), address);
    //}
}
