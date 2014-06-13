#include<syslog.h>
#include<stdio.h>

//#include "prthread.h"

void mopInstrument(int address, int typeSize, char* type, char* debugLoc, char *fName){
    printf("access (??, %p) of type %s (typesize %d) at %s\n", address, type, typeSize, debugLoc);
}
