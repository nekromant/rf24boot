#ifndef DEBUG_H
#define DEBUG_H


#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif
 
#ifndef COMPONENT
#define COMPONENT "???"
#endif


#define do_log(level, ...) __dbg_ # level (__VA_ARGS__)

#define err(...)       fprintf(stderr, COMPONENT ": " __VA_ARGS__)
#define warn(...)      __dbg_0(__VA_ARGS__)
#define info(...)      __dbg_1(__VA_ARGS__)
#define dbg(...)       __dbg_2(__VA_ARGS__)
#define trace(...)     __dbg_3(__VA_ARGS__)



#if DEBUG_LEVEL > 0 
#define __dbg_0(...) fprintf(stderr, COMPONENT ": " __VA_ARGS__)
#define D_WARN_ENABLED
#else
#define __dbg_0(...) { }
#endif

#if DEBUG_LEVEL > 1 
#define __dbg_1(...) fprintf(stderr, COMPONENT ": " __VA_ARGS__)
#define D_INFO_ENABLED
#else
#define __dbg_1(...) { }
#endif

#if DEBUG_LEVEL > 2 
#define __dbg_2(...) fprintf(stderr, COMPONENT ": " __VA_ARGS__)
#define D_DBG_ENABLED
#else
#define __dbg_2(...) { }
#endif

#if DEBUG_LEVEL > 3 
#define __dbg_3(...) fprintf(stderr, COMPONENT ": " __VA_ARGS__)
#else
#define __dbg_3(...) { }
#endif


#endif
