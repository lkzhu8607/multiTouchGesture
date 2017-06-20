#include<stdlib.h>
#include<string.h>
#include <stdio.h>
#include <pthread.h>
#include "common.h"
#include "queue.h"
COM_QUEUE_HDL gQueueTouchInfo;

/*************************************************************************
 * 	������  : ComInitQueue                                
 *	��������: ��ʼ������
 *	�������: pHdlQueue ���в������
 *            NodeInfoSize �û��Զ�������Ϣ�ṹ��Ĵ�С
 *            FreeFunc ���û����Զ���Ľ����Ϣ�ṹ�����ַ������ڴ�ռ䣬���ڴ˴�����
 *                      		�ͷŻص�����
 *            OptDataLen ������û��Զ�������С
 *            OptData  ������û��Զ����������׵�ַ
 *	�������: p_hdl_link �����Ķ��в������                                        
 *	����ֵ  : COM_SUCC: �������гɹ�                              
 *	          COM_FAIL_PARM: ��������
 *            COM_FAIL_MEM: �ڴ����
 *            COM_FAIL_THREAD: �̳߳���
**************************************************************************/

int ComInitQueue (COM_QUEUE_HDL *pHdlQueue,
                        unsigned int NodeInfoSize,
                        int (*FreeFunc) (void *pNode),
                        unsigned int OptDataLen,
                        void* OptData )
{
	/* ���������ȷ�Լ�� */
	if (NULL == pHdlQueue || 0 == NodeInfoSize)
		return COM_FAIL_PARM;

	/* ����û��Զ����˶���ͷ��Ϣ�����ռ� */
	if (0 != OptDataLen && NULL != OptData)
	{
		pHdlQueue->pOptData = malloc (OptDataLen);
		if (NULL == pHdlQueue->pOptData)
			return COM_FAIL_MEM;

		pHdlQueue->OptDataLen = OptDataLen;
		memcpy (pHdlQueue->pOptData, OptData, OptDataLen);
	}

    /* ���л��������� */
    if ( 0 != pthread_mutex_init( &pHdlQueue->QueueLock, NULL ) )
        return COM_FAIL_THREAD;

    /* �����ź������� */
    //if ( 0 != sem_init( &pHdlQueue->QueueSem, 0, 0 ) )
      //  return COM_FAIL_THREAD;

	/* ��ʼ�����в������ */
    pHdlQueue->pQueueHead = NULL;
    pHdlQueue->pQueueTail = NULL;
    pHdlQueue->FreeFunc = FreeFunc;
    pHdlQueue->NodeInfoSize = NodeInfoSize;
    pHdlQueue->QueueNodeCount = 0;
    
	return COM_SUCC;

}

/*************************************************************************
 * 	������  : com_add_queue_node                                 
 *	��������: �ڶ���β����һ�����
 *	�������: pHdlQueue ���в������       
 *            		   pNodeInfo Ҫ������еĽ����Ϣ�׵�ַ
 *	�������: ��                                             
 *	����ֵ  : COM_FAIL_PARM: ��������        
 *	          		COM_FAIL_MEM: �ڴ�������
 *            		COM_SUCC: �������ɹ�
**************************************************************************/
int ComAddQueueNode( COM_QUEUE_HDL* pHdlQueue, void* pNodeInfo )
{
    COM_QUEUE_NODE *pTempNode = NULL;

    /* ����������ȷ�Լ�� */
    if( NULL == pHdlQueue || NULL == pNodeInfo )
        return COM_FAIL_PARM;

    /* ������н�� */
    pTempNode = (COM_QUEUE_NODE*)malloc( sizeof(COM_QUEUE_NODE) );
    if( NULL == pTempNode )
        return COM_FAIL_MEM;
    memset( pTempNode, 0, sizeof( COM_QUEUE_NODE ) );

    /* �����û��Զ�����ռ� */
	pTempNode->pNode = (void *)malloc (pHdlQueue->NodeInfoSize);
	if (NULL == pTempNode->pNode)
	{
		free (pTempNode);
		pTempNode = NULL;
		return COM_FAIL_MEM;
	}
	memset (pTempNode->pNode, 0, pHdlQueue->NodeInfoSize);
    
    /* ������Ķ��н�� */
    memcpy( pTempNode->pNode, pNodeInfo, pHdlQueue->NodeInfoSize);
    pTempNode->pNext = NULL;

    /* �����н��������β */
    pthread_mutex_lock( &pHdlQueue->QueueLock);
    //����ǵ�һ�����
    if( 0 == pHdlQueue->QueueNodeCount )
    {
        pHdlQueue->pQueueTail= pTempNode;
        pHdlQueue->pQueueHead= pTempNode;
    }
    //������ǵ�һ�����
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
 * 	������  : ComDelQueueHeadNode                             
 *	��������: ���׽��Ӷ�����ɾ��
 *	�������: pHdlQueue ���в������                                   
 *	�������: ��                          
 *	����ֵ  : ��
**************************************************************************/
void ComDelQueueHeadNode( COM_QUEUE_HDL* pHdlQueue )
{
    COM_QUEUE_NODE *pTempNode = pHdlQueue->pQueueHead;

    /* �����Ӷ�����ɾ�� */
    pHdlQueue->pQueueHead = pHdlQueue->pQueueHead->pNext;
    pHdlQueue->QueueNodeCount--;
    if( 0 == pHdlQueue->QueueNodeCount )
        pHdlQueue->pQueueTail = NULL;
//	DEBUG_LOG("^^^^^^^^^^^^^1111111111111111111111111^^^^^^^^^^^^^^^^^^^^^^");
    /* ����趨���ͷſռ亯�������ͷ� */
    if (NULL != pHdlQueue->FreeFunc)
        pHdlQueue->FreeFunc(pTempNode->pNode);
	//DEBUG_LOG("^^^^^^^^^^^^^2222222222222222222222222^^^^^^^^^^^^^^^^^^^^^^");
    /* ���ͷ��û��������� */
    if (pTempNode->pNode)
    {
        free (pTempNode->pNode);
        pTempNode->pNode = NULL;
    }

    /* ���ͷŶ��н�� */
    free (pTempNode);
    pTempNode = NULL;
	//DEBUG_LOG("^^^^^^^^^^^^^SUCC TO FREE NODE^^^^^^^^^^^^^^^^^^^^^^");
    return;
}

/*************************************************************************
 * 	������  : ComGetQueueNode                                 
 *	��������: ��ȡ�����׽���е��û���Ϣ�������׽��Ӷ�����ɾ��
 *	�������: pHdlQueue ���в������                                   
 *            node_info �洢�׽����Ϣ�Ŀռ��ַ
 *	�������: node_info �����׽���е��û���Ϣ                                
 *	����ֵ  : COM_SUCC: ��ȡ�ɹ�                 
 *	          COM_FAIL_PARM: ��������
 *            COM_FAIL: ��ȡʧ�ܣ������Ƕ������޽��
**************************************************************************/
int ComGetQueueNode( COM_QUEUE_HDL* pHdlQueue,
                            void* pNodeInfo )
{
    COM_QUEUE_NODE *pTempNode = NULL;

    /* ���������ȷ�Լ�� */
    if( NULL == pHdlQueue || NULL == pNodeInfo )
        return COM_FAIL_PARM;

    //sem_wait( &pHdlQueue->QueueSem );
    pthread_mutex_lock( &pHdlQueue->QueueLock );

    /* ����������޽�㣬���س��� */
    if( 0 == pHdlQueue->QueueNodeCount )
    {
        pthread_mutex_unlock( &pHdlQueue->QueueLock );
        return COM_FAIL;
    }
    
    /* ȡ�����׽�� */
    pTempNode = pHdlQueue->pQueueHead;
    memcpy( pNodeInfo, pTempNode->pNode, pHdlQueue->NodeInfoSize );

    /* ���׽��Ӷ�����ɾ�� */
    ComDelQueueHeadNode( pHdlQueue );
    pthread_mutex_unlock( &pHdlQueue->QueueLock );

    return COM_SUCC;
}

/*************************************************************************
 * 	������  : ComDestroyQueue                                 
 *	��������: �ͷŶ��пռ�
 *	�������: pHdlQueue ���в������                             
 *	�������: ��                 
 *	����ֵ  : COM_SUCC: �ͷŶ��гɹ�                         
 *	          COM_FAIL_PARM: ��������
**************************************************************************/
int ComDestroyQueue (COM_QUEUE_HDL * pHdlQueue)
{
	/* ���������ȷ�Լ�� */
	if (NULL == pHdlQueue)
		return COM_FAIL_PARM;

	/* ��������ϴ��ڽ�㣬�����ͷŽ�� */
    while( pHdlQueue->pQueueHead != NULL )
    {
        pthread_mutex_lock( &pHdlQueue->QueueLock );
        ComDelQueueHeadNode( pHdlQueue );
        pthread_mutex_unlock( &pHdlQueue->QueueLock );
    }

	/* ����û������˿�ѡ�������ͷſ�ѡ���� */
	if (pHdlQueue->OptDataLen != 0 && pHdlQueue->pOptData != NULL)
	{
		free (pHdlQueue->pOptData);
		pHdlQueue->pOptData = NULL;
		pHdlQueue->OptDataLen = 0;
	}

    /* �ͷ��߳��� */
    pthread_mutex_destroy( &pHdlQueue->QueueLock );

    /* �ͷ��ź��� */
    //sem_destroy( &pHdlQueue->QueueSem );

	return COM_SUCC;
}
