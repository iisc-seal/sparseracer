// To use access ops rather than read/write ops
// #define ACCESS

// To use heuristics that prevent false positives
 #define ADDITIONS

// To process locks in the trace
// #define LOCKS

// To set the datatype used for thread-id, op-id and block-id
// To use int datatype
 #define SMALL
// To use long datatype
// #define LARGE
// To use long long datatype
// #define LARGEST

// To process inc() and dec() ops in the trace
// #define REFCOUNT


#ifdef SMALL
typedef int IDType;
#endif
#ifdef LARGE
typedef long IDType;
#endif
#ifdef LARGEST
typedef long long IDType;
#endif

