#ifndef _H_LOG_H_
#define _H_LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/types.h> 

typedef enum {	
	log_lvl_fatal,	
	log_lvl_error,		
	log_lvl_warn,		
	log_lvl_info,		
	log_lvl_debug,	
	log_lvl_trace,
	log_lvl_free,
	log_lvl_send,
	log_lvl_max
} log_lvl_t;

#define FATAL_LOG(fmt, ...)  \
	logPrintf(log_lvl_fatal, "[%d][0x%x][%s:%d] "fmt"\n",getpid(),pthread_self(),__FILE__, __LINE__, \
			 ##__VA_ARGS__)

#define ERROR_LOG(fmt, ...)  \
	logPrintf(log_lvl_error, "[%d][0x%x][%s:%d] "fmt"\n",getpid(),pthread_self(), __FILE__, __LINE__, \
			##__VA_ARGS__)

#define WARN_LOG(fmt, ...)  \
	logPrintf(log_lvl_warn, "[%d][0x%x][%s:%d] "fmt"\n",getpid(),pthread_self(),__FILE__, __LINE__, \
			##__VA_ARGS__)

#define INFO_LOG(fmt, ...)  \
	logPrintf(log_lvl_info, "[%d][0x%x][%s:%d] "fmt"\n",getpid(),pthread_self(), __FILE__, __LINE__, \
			##__VA_ARGS__)

#define DEBUG_LOG(fmt, ...)  \
	logPrintf(log_lvl_debug, "[%d][0x%x][%s:%d] "fmt"\n",getpid(),pthread_self(),__FILE__, __LINE__,\
		  ##__VA_ARGS__)

#define TRACE_LOG(fmt, ...)  \
	logPrintf(log_lvl_trace, "[%d][0x%x][%s:%d] "fmt"\n",getpid(),pthread_self(),__FILE__, __LINE__, \
			##__VA_ARGS__)
			
#define FREE_LOG(fmt, ...) \
	logPrintf(log_lvl_free, fmt"\n", ##__VA_ARGS__)
	
#define SEND_LOG(fmt, ...) \
	logPrintf(log_lvl_send, fmt"\n", ##__VA_ARGS__)
	
int logInit(const char *log_dir, 
			 const char *log_old_dir,
			 const char *log_pre, 
			 const char *log_suf);

int logPrintf(log_lvl_t log_lvl, const char *fmt, ...);

/*
 * 宏定义。用来减少经常判断一个条件是否成立的代码，同时返回一个值
 */
#define DBG_RETURN_VAL_IF_FAIL(expr,val)	do{ \
	if(expr) {} else { \
		ERROR_LOG("判断条件( %s )失败",#expr); \
		if(1) return(val); \
	} \
}while(0)

#endif
