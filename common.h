#ifndef __H_COMMON_H__
#define __H_COMMON_H__

/*#include "queue.h"
#include "checkGesture.h"
#include "touch.h"
#include "log.h"
#include <pthread.h>*/
//#include <time.h>
//#include <math.h>

/* ��������ֵ���� */
#define     COM_FAIL    0
#define     COM_SUCC    1
#define     COM_FAIL_MEM    2   //�ڴ����
#define     COM_FAIL_PARM   3   //��������
#define     COM_FAIL_FILE   4   //�ļ���������
#define     COM_FAIL_POPEN  5   //popen����
#define     COM_FAIL_FORK   6   //fork����
#define     COM_FAIL_SOCKET 7   //socket����
#define     COM_FAIL_THREAD 8   //pthread����

#define SCREENWIDTH 1920 //��Ļ���ؿ�ȷֱ���
#define SCREENHEIGTH 1080 //��Ļ���ظ߶ȷֱ���

/*�����궨��*/
#define MAX_QUEUE_COUNT 5000


/* * RESULT�����궨�� */
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

#define SQUARE(a)   ((a)*(a)) //����ƽ��


typedef struct SETTINGTAG{
	int mode;// 1:��ʾֱ�ӴӴ�������ȡ�ź�Դ��Ϣ; 2:��ʾ���ļ��л�ȡ�ź�Դ��Ϣ
	char fileName[128];//��mode = 1ʱ����������豸�ļ�; ��mode = 2ʱ����������ѱ��津���ź�Դ��Ϣ���ļ���
}SETTING;
extern SETTING gSetting;
#endif