#ifndef _BSG_MANYCORE_PRINTING_H
#define _BSG_MANYCORE_PRINTING_H

#if defined(COSIM)
#include "bsg_manycore_features.h"
#else
#include <bsg_manycore_features.h>
#endif
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#if defined(__cplusplus)
extern "C" {
#endif // #if defined(__cplusplus)

#if defined(COSIM)
#define BSG_PRINT_PREFIX_DEBUG "DEBUG @(%llu/%llu):   "
#define BSG_PRINT_PREFIX_ERROR "ERROR:   "
#define BSG_PRINT_PREFIX_WARN  "WARNING: "
#define BSG_PRINT_PREFIX_INFO  "INFO:    "
#else // !defined(COSIM)
#define BSG_PRINT_PREFIX_DEBUG "DEBUG @(%llu):   "
#define BSG_PRINT_PREFIX_ERROR "ERROR:   "
#define BSG_PRINT_PREFIX_WARN  "WARNING: "
#define BSG_PRINT_PREFIX_INFO  "INFO:    "
#endif

#define BSG_PRINT_STREAM_DEBUG stderr
#define BSG_PRINT_STREAM_ERROR stderr
#define BSG_PRINT_STREAM_WARN  stderr
#define BSG_PRINT_STREAM_INFO  stderr               

#ifdef COSIM
extern void sv_bsg_realtime(uint64_t*);
uint64_t bsg_realtime();
#endif

static inline uint64_t bsg_utc(){
	struct timeval tv;
	gettimeofday(&tv, NULL);

	uint64_t ms =
		(uint64_t)(tv.tv_sec) * 1000 +
		(uint64_t)(tv.tv_usec) / 1000;

	return ms;
}

__attribute__((format(printf, 2, 3)))
int bsg_pr_prefix(const char *prefix, const char *fmt, ...);
        
#if defined(DEBUG) && defined(COSIM)
#define bsg_pr_dbg(fmt, ...)                    \
        bsg_pr_prefix(BSG_PRINT_PREFIX_DEBUG, fmt, bsg_realtime(), bsg_utc(), ##__VA_ARGS__)
#elif defined(DEBUG)
#define bsg_pr_dbg(fmt, ...)                    \
        bsg_pr_prefix(BSG_PRINT_PREFIX_DEBUG, fmt, bsg_utc(), ##__VA_ARGS__)
#else 
#define bsg_pr_dbg(...)
#endif

#define bsg_pr_err(fmt, ...)                                            \
        bsg_pr_prefix(BSG_PRINT_PREFIX_ERROR, fmt, ##__VA_ARGS__)
        
#define bsg_pr_warn(fmt, ...)                                           \
        bsg_pr_prefix(BSG_PRINT_PREFIX_WARN, fmt, ##__VA_ARGS__)
        
#define bsg_pr_info(fmt, ...)                                           \
        bsg_pr_prefix(BSG_PRINT_PREFIX_INFO, fmt, ##__VA_ARGS__)


#if defined(__cplusplus)
}
#endif // #if defined(__cplusplus)
        
#endif
