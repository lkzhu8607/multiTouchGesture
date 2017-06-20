/***************************************************************
 * 名   称 : 日志库
 * 作   者 : 朱连凯
 * 创建日期: 2016-9-24
 * 修改记录: 
 ****************************************************************/

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>


#define BUFFER_SIZE          (4 * 1024)
#define MAX_LINE			 100000
#define MAX_BYTE			 (1024*1024*50)

typedef struct {
	int current_fd;
	int last_fd;
	int log_line;
	int log_byte;
	int log_count;
	time_t switch_time; 
	pthread_mutex_t mutex;
} log_info_t;

static char g_log_dir[1024] = {0};
static char g_log_old_dir[1024] = {0};
static char g_log_pre[1024] = {0};
static char g_log_suf[1024] = {0};
static log_lvl_t g_log_lvl = log_lvl_max; 
static unsigned long g_max_line = 0;
static unsigned long g_max_byte = 0;
static log_info_t g_log_info[log_lvl_max] = {{0}};

static pthread_key_t key = 0;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;

static char log_lvl_name[log_lvl_max][16] = {
		"fatal", 
	    "error",
		"warn",
        "info",
        "debug",
        "trace",
        "free",
        "send"
	};

static int isSameDay(time_t time1, time_t time2);
static void threadInit(void);
static const char * getLogCurrentPathName(log_lvl_t log_lvl, char *buffer, int buffer_size);
static const char * getLogSwitchPathName(log_lvl_t log_lvl, char *buffer, int buffer_size);
static int logSwitch(log_lvl_t log_lvl);

int logInit(const char *log_dir,const char *log_old_dir, const char *log_pre, const char *log_suf)
{
	memset(&g_log_info, 0, sizeof(g_log_info));
	if (log_dir != NULL) {
		strncpy(g_log_dir, log_dir, sizeof(g_log_dir) - 1);
	} else {
		strncpy(g_log_dir, ".", sizeof(g_log_dir) - 1);
	}
	if (log_old_dir != NULL) {
		strncpy(g_log_old_dir, log_old_dir, sizeof(g_log_old_dir) - 1);
	} else {
		strncpy(g_log_old_dir, ".", sizeof(g_log_old_dir) - 1);
	}
	if (log_pre != NULL) {
		strncpy(g_log_pre, log_pre, sizeof(g_log_pre) - 1);
	} else {
		strncpy(g_log_pre, "program", sizeof(g_log_pre) - 1);
	}
	if (log_suf != NULL) {
		strncpy(g_log_suf, log_suf, sizeof(g_log_suf) - 1);
	} else {
		strncpy(g_log_suf, "log", sizeof(g_log_suf) - 1);
	}

	//g_log_lvl = log_lvl_debug;
	g_max_line = MAX_LINE;
	g_max_byte = MAX_BYTE;

	if (access(g_log_dir, W_OK) != 0) {
		return -1;
	}
	if (access(g_log_old_dir, W_OK) != 0) {
		return -1;
	}

	int i = 0;
	for (i = 0; i < log_lvl_max; ++i) {
		g_log_info[i].current_fd = -1;
		g_log_info[i].last_fd = -1;
		g_log_info[i].log_line = 0;
		g_log_info[i].log_byte = 0;
		if (pthread_mutex_init(&g_log_info[i].mutex, NULL) != 0) {
			return -1;
		}
	}

	pthread_once(&init_done, threadInit);//保证thread_init函数只执行一次

	return 0;
}

int logPrintf(log_lvl_t log_lvl, const char *fmt, ...)
{
	if (log_lvl > g_log_lvl) {
		return 0;
	}

	char *buffer = NULL;
	int size = 0;
	buffer = (char *)pthread_getspecific(key);
	if (buffer == NULL) {
		buffer = (char *)malloc(BUFFER_SIZE);
		if (buffer == NULL) {
			return -1;
		}
		pthread_setspecific(key, buffer);
	}

	memset(buffer, 0, BUFFER_SIZE);

	time_t current_time = time(NULL);         
	struct tm current_tm = {0};
	int time_len = 0;
	time_len = strftime(buffer, BUFFER_SIZE - 1, 
						"[%Y-%m-%d %H:%M:%S]", localtime_r(&current_time, &current_tm));       
	va_list ap;	
	va_start(ap, fmt);	
	size = vsnprintf(buffer + time_len, BUFFER_SIZE - time_len - 1, fmt, ap) + 
			time_len;	
	va_end(ap);

	if ((g_max_line > 0 && g_log_info[log_lvl].log_line >= g_max_line) ||
	   (g_max_byte > 0 && g_log_info[log_lvl].log_byte >= g_max_byte) ||
	   !isSameDay(g_log_info[log_lvl].switch_time, time(NULL))) {
		logSwitch(log_lvl);
	}
	write(g_log_info[log_lvl].current_fd, buffer, size);
	++g_log_info[log_lvl].log_line;
	g_log_info[log_lvl].log_byte += size;

	return 0;
	
}

static int isSameDay(time_t time1, time_t time2)
{
	struct tm tm1 = {0};
	struct tm tm2 = {0};

	localtime_r(&time1, &tm1);
	localtime_r(&time2, &tm2);

	return (tm1.tm_year == tm2.tm_year && 
			tm1.tm_mon == tm2.tm_mon && 
			tm1.tm_mday == tm2.tm_mday);
}

static void threadInit(void)
{
	pthread_key_create(&key, free);
}

static const char * getLogCurrentPathName(log_lvl_t log_lvl, char *buffer, int buffer_size)
{
	if (buffer == NULL) {
		return NULL;
	}
	
	memset(buffer, 0, buffer_size);
	snprintf(buffer, buffer_size - 1, "%s/%s_%s_%s.%s", 
				g_log_dir, g_log_pre, log_lvl_name[log_lvl], "current", g_log_suf);
	
	return buffer;
}

static const char * getLogSwitchPathName(log_lvl_t log_lvl, char *buffer, int buffer_size)
{
	if (buffer == NULL) {
		return NULL;
	}

	struct stat file_stat = {0};
	stat(getLogCurrentPathName(log_lvl, buffer, buffer_size), &file_stat);
	time_t file_time = file_stat.st_ctime;

	char time_buffer[24] = {0};
	struct tm tm_buffer = {0};
	strftime(time_buffer, sizeof(time_buffer) - 1, 
						"%Y%m%d", localtime_r(&file_time, &tm_buffer));       

	memset(buffer, 0, buffer_size);
	snprintf(buffer, buffer_size - 1, "%s/%s_%s_%s_%03d.%s",
				g_log_old_dir, g_log_pre, log_lvl_name[log_lvl], time_buffer, 
				g_log_info[log_lvl].log_count, g_log_suf);

	return buffer;
}

static int logSwitch(log_lvl_t log_lvl)
{
	if (pthread_mutex_trylock(&g_log_info[log_lvl].mutex) == 0) {
		char current_path_name[1024] = {0};
		char switch_path_name[1024] = {0};

		int old_last_fd = g_log_info[log_lvl].last_fd;
		g_log_info[log_lvl].last_fd = g_log_info[log_lvl].current_fd;
		rename(getLogCurrentPathName(log_lvl, current_path_name, sizeof(current_path_name)), 
			   getLogSwitchPathName(log_lvl, switch_path_name, sizeof(switch_path_name)));
		g_log_info[log_lvl].current_fd = 
			/*open(current_path_name, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 0666);*/
			open(current_path_name, O_WRONLY | O_APPEND | O_CREAT, 0666);
		if (g_log_info[log_lvl].current_fd == -1) {
			g_log_info[log_lvl].current_fd = g_log_info[log_lvl].last_fd;
			g_log_info[log_lvl].last_fd = old_last_fd;
			pthread_mutex_unlock(&g_log_info[log_lvl].mutex);
			return -1;
		}
	    g_log_info[log_lvl].log_line = 0;
		g_log_info[log_lvl].log_byte = 0;
		g_log_info[log_lvl].switch_time = time(NULL);
		++g_log_info[log_lvl].log_count;
		if (old_last_fd != -1) {
			close(old_last_fd);
		}
		pthread_mutex_unlock(&g_log_info[log_lvl].mutex);
	} else {
		//pthread_mutex_lock(&g_log_info[log_lvl].mutex);
		pthread_mutex_unlock(&g_log_info[log_lvl].mutex);
	}
	
	return 0;
}