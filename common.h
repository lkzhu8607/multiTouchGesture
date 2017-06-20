#ifndef __H_COMMON_H__
#define __H_COMMON_H__

/*#include "queue.h"
#include "checkGesture.h"
#include "touch.h"
#include "log.h"
#include <pthread.h>*/
//#include <time.h>
//#include <math.h>

/* 函数返回值定义 */
#define     COM_FAIL    0
#define     COM_SUCC    1
#define     COM_FAIL_MEM    2   //内存错误
#define     COM_FAIL_PARM   3   //参数出错
#define     COM_FAIL_FILE   4   //文件操作出错
#define     COM_FAIL_POPEN  5   //popen出错
#define     COM_FAIL_FORK   6   //fork出错
#define     COM_FAIL_SOCKET 7   //socket出错
#define     COM_FAIL_THREAD 8   //pthread出错

#define SCREENWIDTH 1920 //屏幕像素宽度分辨率
#define SCREENHEIGTH 1080 //屏幕像素高度分辨率

/*常量宏定义*/
#define MAX_QUEUE_COUNT 5000


/* * RESULT变量宏定义 */
#define NORMAL	0
#define EXCEPTION	-1
#define INVALID	-2

/* * General function macros */
#ifndef min
#define min(A, B)		((A) < (B) ? (A) : (B))
#endif

#ifndef max
#define max(A, B)		((A) > (B) ? (A) : (B))
#endif

#define minsize(A, B)	(sizeof(A) < sizeof(B) ? sizeof(A) : sizeof(B))
#define maxsize(A, B)	(sizeof(A) > sizeof(B) ? sizeof(A) : sizeof(B))

#ifndef isblank
#define isblank(c)		((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')
#endif

#ifndef isdigit
#define isdigit(c)		((c) <= '9' && (c) >= '0')
#endif

#ifndef isalpha
#define isalpha(c)		((c) <= 'z' && (c) >= 'a' || (c) <= 'Z' && (c) >= 'A')
#endif

#ifndef upper
#define upper(c)		((c) <= 'z' && (c) >= 'a' ? (c) - 'a' + 'A' : (c))
#endif

#ifndef lower
#define lower(c)		((c) <= 'Z' && (c) >= 'A' ? (c) - 'A' + 'a' : (c))
#endif

#define SQUARE(a)   ((a)*(a)) //计算平方


typedef struct SETTINGTAG{
	int mode;// 1:表示直接从触摸屏上取信号源信息; 2:表示从文件中获取信号源信息
	char fileName[128];//当mode = 1时，保存的是设备文件; 当mode = 2时，保存的是已保存触屏信号源信息的文件名
}SETTING;
extern SETTING gSetting;
#endif