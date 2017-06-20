#ifndef __H_QUEUE_H__
#define __H_QUEUE_H__

/***********************************************************************
 *ͷ�ļ�����                                                           *
 ***********************************************************************/

#include <semaphore.h>
//#include "common.h"

/***********************************************************************
 *�궨��                                                               *
 ***********************************************************************/

/***********************************************************************
 *���ݽṹ����                                                         *
 ***********************************************************************/
/* ���н��ṹ�� */
typedef struct ComQueueNodeTag
{
    struct ComQueueNodeTag *pNext; //���к�����ָ��
    void *pNode;                   //�û��Զ�������Ϣ�ṹ��          
} ComQueueNodeT, COM_QUEUE_NODE;

/* ���в�������ṹ�� */
typedef struct
{
    COM_QUEUE_NODE* pQueueHead; //ָ����еĶ���
    COM_QUEUE_NODE* pQueueTail; //ָ����еĶ�β
    pthread_mutex_t QueueLock; //������
    //sem_t           QueueSem;  //�����ź���
    int (*FreeFunc) (void *pNode);  //�û������p_node���ַ����˿ռ䣬��Ҫ���ﶨ���ͷź�����������ΪNULL
    unsigned int NodeInfoSize;     //�û��Զ�������Ϣ�ṹ���С
    unsigned int QueueNodeCount;   //�����д��ڵĽ�����
	unsigned int OptDataLen;		//�û��Զ����ͷ��Ϣ����,������������Ϊ0
	void *pOptData;				//�û��Զ����ͷ��Ϣ��������������ΪNULL
} com_queue_hdl_t, COM_QUEUE_HDL;

/***********************************************************************
 *�ⲿ��������                                                         *
 ***********************************************************************/

/***********************************************************************
 *���ⲿ�����궨��                                                     
 ***********************************************************************/
//ͨ�����в�������ɻ�ȡ����Ϣ
#define COM_HDL_GET_QUEUE_HEAD(x) (((COM_QUEUE_HDL*)(x))->queue_head)
#define COM_HDL_GET_QUEUE_TAIL(x) (((COM_QUEUE_HDL*)(x))->queue_tail)
#define COM_HDL_GET_QUEUE_LOCK(x) (((COM_QUEUE_HDL*)(x))->queue_lock)
#define COM_HDL_GET_QUEUE_SEM(x)  (((COM_QUEUE_HDL*)(x))->queue_sem)
#define COM_HDL_GET_QUEUE_NODE_INFO_SIZE(x) (((COM_QUEUE_HDL*)(x))->node_info_size)
#define COM_HDL_GET_QUEUE_NODE_COUNT(x) (((COM_QUEUE_HDL*)(x))->queue_node_count)
#define COM_HDL_GET_QUEUE_OPT_DATA_LEN(x) (((COM_QUEUE_HDL*)(x))->opt_data_len)
#define COM_HDL_GET_QUEUE_OPT_DATA(x)   (((COM_QUEUE_HDL*)(x))->opt_data)
    
//ͨ�����н��ɻ�ȡ����Ϣ
#define COM_QUEUE_GET_NEXT(n)    (((COM_QUEUE_NODE*)(n))->next)
#define COM_QUEUE_GET_USER_INFO(n)  (((COM_QUEUE_NODE*)(n))->p_node)

/***********************************************************************
 *��������                                                             *
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
