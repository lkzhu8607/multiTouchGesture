#include<stdlib.h>
#include<string.h>
#include <stdio.h>
#include <pthread.h>
#include "common.h"
#include "queue.h"
COM_QUEUE_HDL gQueueTouchInfo;

/*************************************************************************
 * 	函数名  : ComInitQueue                                
 *	功能描述: 初始化队列
 *	输入参数: pHdlQueue 队列操作句柄
 *            NodeInfoSize 用户自定义结点信息结构体的大小
 *            FreeFunc 若用户在自定义的结点信息结构体中又分配了内存空间，则在此处定义
 *                      		释放回调函数
 *            OptDataLen 句柄中用户自定义区大小
 *            OptData  句柄中用户自定义区内容首地址
 *	输出参数: p_hdl_link 创建的队列操作句柄                                        
 *	返回值  : COM_SUCC: 创建队列成功                              
 *	          COM_FAIL_PARM: 参数出错
 *            COM_FAIL_MEM: 内存出错
 *            COM_FAIL_THREAD: 线程出错
**************************************************************************/

int ComInitQueue (COM_QUEUE_HDL *pHdlQueue,
                        unsigned int NodeInfoSize,
                        int (*FreeFunc) (void *pNode),
                        unsigned int OptDataLen,
                        void* OptData )
{
	/* 输入参数正确性检查 */
	if (NULL == pHdlQueue || 0 == NodeInfoSize)
		return COM_FAIL_PARM;

	/* 如果用户自定义了队列头信息则分配空间 */
	if (0 != OptDataLen && NULL != OptData)
	{
		pHdlQueue->pOptData = malloc (OptDataLen);
		if (NULL == pHdlQueue->pOptData)
			return COM_FAIL_MEM;

		pHdlQueue->OptDataLen = OptDataLen;
		memcpy (pHdlQueue->pOptData, OptData, OptDataLen);
	}

    /* 队列互斥锁创建 */
    if ( 0 != pthread_mutex_init( &pHdlQueue->QueueLock, NULL ) )
        return COM_FAIL_THREAD;

    /* 队列信号量创建 */
    //if ( 0 != sem_init( &pHdlQueue->QueueSem, 0, 0 ) )
      //  return COM_FAIL_THREAD;

	/* 初始化队列操作句柄 */
    pHdlQueue->pQueueHead = NULL;
    pHdlQueue->pQueueTail = NULL;
    pHdlQueue->FreeFunc = FreeFunc;
    pHdlQueue->NodeInfoSize = NodeInfoSize;
    pHdlQueue->QueueNodeCount = 0;
    
	return COM_SUCC;

}

/*************************************************************************
 * 	函数名  : com_add_queue_node                                 
 *	功能描述: 在队列尾新增一个结点
 *	输入参数: pHdlQueue 队列操作句柄       
 *            		   pNodeInfo 要加入队列的结点信息首地址
 *	输出参数: 无                                             
 *	返回值  : COM_FAIL_PARM: 参数出错        
 *	          		COM_FAIL_MEM: 内存分配出错
 *            		COM_SUCC: 新增结点成功
**************************************************************************/
int ComAddQueueNode( COM_QUEUE_HDL* pHdlQueue, void* pNodeInfo )
{
    COM_QUEUE_NODE *pTempNode = NULL;

    /* 函数参数正确性检查 */
    if( NULL == pHdlQueue || NULL == pNodeInfo )
        return COM_FAIL_PARM;

    /* 分配队列结点 */
    pTempNode = (COM_QUEUE_NODE*)malloc( sizeof(COM_QUEUE_NODE) );
    if( NULL == pTempNode )
        return COM_FAIL_MEM;
    memset( pTempNode, 0, sizeof( COM_QUEUE_NODE ) );

    /* 分配用户自定义结点空间 */
	pTempNode->pNode = (void *)malloc (pHdlQueue->NodeInfoSize);
	if (NULL == pTempNode->pNode)
	{
		free (pTempNode);
		pTempNode = NULL;
		return COM_FAIL_MEM;
	}
	memset (pTempNode->pNode, 0, pHdlQueue->NodeInfoSize);
    
    /* 填充分配的队列结点 */
    memcpy( pTempNode->pNode, pNodeInfo, pHdlQueue->NodeInfoSize);
    pTempNode->pNext = NULL;

    /* 将队列结点插入队列尾 */
    pthread_mutex_lock( &pHdlQueue->QueueLock);
    //如果是第一个结点
    if( 0 == pHdlQueue->QueueNodeCount )
    {
        pHdlQueue->pQueueTail= pTempNode;
        pHdlQueue->pQueueHead= pTempNode;
    }
    //如果不是第一个结点
    else
    {
        pHdlQueue->pQueueTail->pNext = pTempNode;
        pHdlQueue->pQueueTail = pTempNode;
    }

    pHdlQueue->QueueNodeCount++;
    //sem_post( &pHdlQueue->QueueSem);
    pthread_mutex_unlock( &pHdlQueue->QueueLock);
    
    return COM_SUCC;
}

/*************************************************************************
 * 	函数名  : ComDelQueueHeadNode                             
 *	功能描述: 将首结点从队列中删除
 *	输入参数: pHdlQueue 队列操作句柄                                   
 *	输出参数: 无                          
 *	返回值  : 无
**************************************************************************/
void ComDelQueueHeadNode( COM_QUEUE_HDL* pHdlQueue )
{
    COM_QUEUE_NODE *pTempNode = pHdlQueue->pQueueHead;

    /* 将结点从队列中删除 */
    pHdlQueue->pQueueHead = pHdlQueue->pQueueHead->pNext;
    pHdlQueue->QueueNodeCount--;
    if( 0 == pHdlQueue->QueueNodeCount )
        pHdlQueue->pQueueTail = NULL;
//	DEBUG_LOG("^^^^^^^^^^^^^1111111111111111111111111^^^^^^^^^^^^^^^^^^^^^^");
    /* 如果设定了释放空间函数则先释放 */
    if (NULL != pHdlQueue->FreeFunc)
        pHdlQueue->FreeFunc(pTempNode->pNode);
	//DEBUG_LOG("^^^^^^^^^^^^^2222222222222222222222222^^^^^^^^^^^^^^^^^^^^^^");
    /* 先释放用户数据区域 */
    if (pTempNode->pNode)
    {
        free (pTempNode->pNode);
        pTempNode->pNode = NULL;
    }

    /* 再释放队列结点 */
    free (pTempNode);
    pTempNode = NULL;
	//DEBUG_LOG("^^^^^^^^^^^^^SUCC TO FREE NODE^^^^^^^^^^^^^^^^^^^^^^");
    return;
}

/*************************************************************************
 * 	函数名  : ComGetQueueNode                                 
 *	功能描述: 获取队列首结点中的用户信息，并将首结点从队列中删除
 *	输入参数: pHdlQueue 队列操作句柄                                   
 *            node_info 存储首结点信息的空间地址
 *	输出参数: node_info 返回首结点中的用户信息                                
 *	返回值  : COM_SUCC: 获取成功                 
 *	          COM_FAIL_PARM: 参数出错
 *            COM_FAIL: 获取失败，可能是队列中无结点
**************************************************************************/
int ComGetQueueNode( COM_QUEUE_HDL* pHdlQueue,
                            void* pNodeInfo )
{
    COM_QUEUE_NODE *pTempNode = NULL;

    /* 输入参数正确性检查 */
    if( NULL == pHdlQueue || NULL == pNodeInfo )
        return COM_FAIL_PARM;

    //sem_wait( &pHdlQueue->QueueSem );
    pthread_mutex_lock( &pHdlQueue->QueueLock );

    /* 如果队列中无结点，返回出错 */
    if( 0 == pHdlQueue->QueueNodeCount )
    {
        pthread_mutex_unlock( &pHdlQueue->QueueLock );
        return COM_FAIL;
    }
    
    /* 取队列首结点 */
    pTempNode = pHdlQueue->pQueueHead;
    memcpy( pNodeInfo, pTempNode->pNode, pHdlQueue->NodeInfoSize );

    /* 将首结点从队列中删除 */
    ComDelQueueHeadNode( pHdlQueue );
    pthread_mutex_unlock( &pHdlQueue->QueueLock );

    return COM_SUCC;
}

/*************************************************************************
 * 	函数名  : ComDestroyQueue                                 
 *	功能描述: 释放队列空间
 *	输入参数: pHdlQueue 队列操作句柄                             
 *	输出参数: 无                 
 *	返回值  : COM_SUCC: 释放队列成功                         
 *	          COM_FAIL_PARM: 参数出错
**************************************************************************/
int ComDestroyQueue (COM_QUEUE_HDL * pHdlQueue)
{
	/* 输入参数正确性检查 */
	if (NULL == pHdlQueue)
		return COM_FAIL_PARM;

	/* 如果队列上存在结点，则先释放结点 */
    while( pHdlQueue->pQueueHead != NULL )
    {
        pthread_mutex_lock( &pHdlQueue->QueueLock );
        ComDelQueueHeadNode( pHdlQueue );
        pthread_mutex_unlock( &pHdlQueue->QueueLock );
    }

	/* 如果用户设置了可选区域，则释放可选区域 */
	if (pHdlQueue->OptDataLen != 0 && pHdlQueue->pOptData != NULL)
	{
		free (pHdlQueue->pOptData);
		pHdlQueue->pOptData = NULL;
		pHdlQueue->OptDataLen = 0;
	}

    /* 释放线程锁 */
    pthread_mutex_destroy( &pHdlQueue->QueueLock );

    /* 释放信号量 */
    //sem_destroy( &pHdlQueue->QueueSem );

	return COM_SUCC;
}
