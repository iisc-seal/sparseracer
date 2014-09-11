#include <syslog.h>

void mopDealloc(int address, int typeSize, char* type, char* debugLoc, char *fName){
  //if(show_backtrace("DOMMediaStream")){
  //printStackTrace(stderr, 0);
    //syslog(LOG_INFO, "%p of type %s (typesize %d) bytes freed at %s\n", address, type, typeSize, debugLoc);
    //if(print_stacktrace(stderr, 63, interesting))
  //syslog(LOG_DEBUG, "free(%lu, %p, %d):%s", pthread_self(), address, typeSize, fName);
  printf("free(%lu, %p, %d):%s\n", pthread_self(), address, typeSize, type);
  //}
  //printf("=====================================================\n");
    //}
}

void mopAlloc(int address, int memsize, char* type, char* debugLoc, char *fName){
  //if(strstr(type, "DOMMediaStream")){
    //syslog(LOG_INFO, "allocated %d bytes of type %s at address %p (%s)\n", memsize, type, address, debugLoc);
  //show_backtrace();
  /* if(show_backtrace("DOMMediaStream")){ */
  //syslog(LOG_DEBUG, "alloc(%lu, %p, %d):%s", pthread_self(), address, memsize, fName); 
     printf("alloc(%lu, %p, %d:%s)\n", pthread_self(), address, memsize, type); 
  /* } */
}
