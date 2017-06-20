#ifndef __H_QUEUE_H__
#define __H_QUEUE_H__

/***********************************************************************
 *头文件包含                                                           *
 ***********************************************************************/

#include <semaphore.h>
//#include "common.h"

/***********************************************************************
 *宏定义                                                               *
 ***********************************************************************/

/***********************************************************************
 *数据结构定义                                                         *
 ***********************************************************************/
/* 队列结点结构体 */
typedef struct ComQueueNodeTag
{
    struct ComQueueNodeTag *pNext; //队列后向结点指针
    void *pNode;                   //用户自定义结点信息结构体          
} ComQueueNodeT, COM_QUEUE_NODE;

/* 队列操作句柄结构体 */
typedef struct
{
    COM_QUEUE_NODE* pQueueHead; //指向队列的队首
    COM_QUEUE_NODE* pQueueTail; //指向队列的队尾
    pthread_mutex_t QueueLock; //队列锁
    //sem_t           QueueSem;  //队列信号量
    int (*FreeFunc) (void *pNode);  //用户如果在p_node中又分配了空间，需要这里定义释放函数，否则设为NULL
    unsigned int NodeInfoSize;     //用户自定义结点信息结构体大小
    unsigned int QueueNodeCount;   //队列中存在的结点个数
	unsigned int OptDataLen;		//用户自定义表头信息长度,若不定义则设为0
	void *pOptData;				//用户自定义表头信息，若不定义则设为NULL
} com_queue_hdl_t, COM_QUEUE_HDL;

/***********************************************************************
 *外部变量引用                                                         *
 ***********************************************************************/

/***********************************************************************
 *供外部操作宏定义                                                     
 ***********************************************************************/
//通过队列操作句柄可获取的信息
#define COM_HDL_GET_QUEUE_HEAD(x) (((COM_QUEUE_HDL*)(x))->queue_head)
#define COM_HDL_GET_QUEUE_TAIL(x) (((COM_QUEUE_HDL*)(x))->queue_tail)
#define COM_HDL_GET_QUEUE_LOCK(x) (((COM_QUEUE_HDL*)(x))->queue_lock)
#define COM_HDL_GET_QUEUE_SEM(x)  (((COM_QUEUE_HDL*)(x))->queue_sem)
#define COM_HDL_GET_QUEUE_NODE_INFO_SIZE(x) (((COM_QUEUE_HDL*)(x))->node_info_size)
#define COM_HDL_GET_QUEUE_NODE_COUNT(x) (((COM_QUEUE_HDL*)(x))->queue_node_count)
#define COM_HDL_GET_QUEUE_OPT_DATA_LEN(x) (((COM_QUEUE_HDL*)(x))->opt_data_len)
#define COM_HDL_GET_QUEUE_OPT_DATA(x)   (((COM_QUEUE_HDL*)(x))->opt_data)
    
//通过队列结点可获取的信息
#define COM_QUEUE_GET_NEXT(n)    (((COM_QUEUE_NODE*)(n))->next)
#define COM_QUEUE_GET_USER_INFO(n)  (((COM_QUEUE_NODE*)(n))->p_node)

/***********************************************************************
 *函数声明                                                             *
 ***********************************************************************/
extern int ComInitQueue (COM_QUEUE_HDL* pHdlQueue,
                        unsigned int NodeInfoSize,
                        int (*FreeFunc) (void *pNode),
                        unsigned int OptDataLen,
                        void* pOptData );
extern int ComAddQueueNode( COM_QUEUE_HDL* pHdlQueue, void* pNodeInfo );
int ComGetQueueNode( COM_QUEUE_HDL* pHdlQueue, void* pNodeInfo );
int ComDestroyQueue (COM_QUEUE_HDL * pHdlQueue);

extern COM_QUEUE_HDL gQueueTouchInfo;

#endif
