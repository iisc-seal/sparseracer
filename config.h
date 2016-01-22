// To limit the number of nodes
 #define NODELIMIT 15000

// Flag to give up when reaching nodelimit
// #define RUNOVERNODELIMIT

// To use access ops rather than read/write ops
// #define ACCESS

// To use heuristics that prevent false positives
 #define ADDITIONS

// To process locks in the trace
// #define LOCKS

// To enable sanity checks
 #define SANITYCHECK

// To set the datatype used for thread-id, op-id and block-id
// To use int datatype
 #define SMALL
// To use long datatype
// #define LARGE
// To use long long datatype
// #define LARGEST

// To find data races also
// #define DATARACE

// To find unique races
// #define UNIQUERACE

// To use nodes for finding races
 #define NODERACES

// To enable reporting of only single threaded races
// #define SINGLETHREADEDRACES

// To use extra rules
// #define EXTRARULES

// To enable MT and Cascaded loop rules
// #define ADVANCEDRULES

#ifdef SMALL
typedef int IDType;
#endif
#ifdef LARGE
typedef long IDType;
#endif
#ifdef LARGEST
typedef long long IDType;
#endif
