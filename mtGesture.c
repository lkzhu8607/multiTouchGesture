#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "touch.h"
#include "checkGesture.h"
#include "queue.h"
#include "common.h"
#include "log.h"
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>

#define LOG_PATH "../log/"
#define LOG_HISTORY_PATH "../historyLog/"
#define PORT 3721
//#define PORT 3722
//#define SCREENWIDTH 1920
//#define SCREENHEIGTH 1080

#define CHECKSOCKTIMEOUT 2000000 //̽����ĳ�ʱʱ��,��ǰֵ�ĵ�λΪus

static int releaseFlag = 0;//0,��ʾ�������ϴβ���������ָ��Ϣ��1����ʾ�ϴβ���������ָ�ڴ�������

#define BASESCALE 10000 //�������ʣ���ԭͼ��ʱ��Ϊ10000
#define POWERMAX 1.07   //�������ķŴ���
#define POWERMIN 0.8    //������С����С����

#define MAJOR_VERSION 2   //���汾��
#define MINOR_VERSION 00  //�ΰ汾��



#define FILTER_DRAG_GESTURE 4 //�������Ź���������ӵ���ק���������Ķ���

int singleDragTimeFlag = 0;
pthread_t gTouchInfoID = 0;
pthread_t gGestureInfoID = 0;
pthread_attr_t  gTouchInfoIDAttr; 
pthread_attr_t  gGestureInfoIDAttr;

//COM_QUEUE_HDL gQueueTouchInfo;
char gInputDeviceFile[128];
int gSocket = 0;

/*typedef struct SETTINGTAG{
	int mode;// 1:��ʾֱ�ӴӴ�������ȡ�ź�Դ��Ϣ; 2:��ʾ���ļ��л�ȡ�ź�Դ��Ϣ
	char fileName[128];//��mode = 1ʱ����������豸�ļ�; ��mode = 2ʱ����������ѱ��津���ź�Դ��Ϣ���ļ���
}SETTING;*/
SETTING gSetting;

int dragEnableFlag = 0;//���������ǣ�ֻ�������һ�������Ժ���ק�Ķ���������Ч
int singleEnableFlag = 0;//��ָ��־
int invalidCountFlag = 0;//�Ϸ�����ָ������־λ������ָ�ĸ���С��0���ߴ���10��ʱ�򣬱��Ϊ1
int invalidSlotValueFlag = 0;//�Ϸ���Slotֵ��־λ��0,��ʾ�Ϸ���slot;1,��ʾ�Ƿ���slotֵ;С��-1���ߴ���9��slotֵΪ�Ƿ�ֵ

SAMPLETOUCHINFO singleFingerBase[2];//��Ե�����ָ�Ĳ������ݻ�׼
SAMPLETOUCHINFO multiFingersBase[2][10];//��Զ����ָ�Ĳ������ݻ�׼

AXIS scaleCenterPoint;//��ק�Ժ����ŵ����ĵ��¼

//̽���
unsigned char probePacket[28] = {0x00,0x00,0x27,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

int setSocketNonblock(int sockfd)  
{ 
	int block_flag = fcntl(sockfd, F_GETFL, 0); 
	if(block_flag < 0){
		ERROR_LOG("get socket fd flag error:%s\n", strerror(errno));  
        return -1;  
    }  
	else{ 
		if(fcntl(sockfd, F_SETFL, block_flag | O_NONBLOCK) < 0){
			ERROR_LOG("set socket fd non block error:%s\n", strerror(errno));  
            return -1;  
        }  
    } 
	return 0;  
 }

/*********************
return��-1,����socket�����ӷ�����ʧ��
*********************/
int socketConnect()
{
    gSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(gSocket < 0)
	{
        perror("create socket");
		ERROR_LOG("Fail to create socket, sock_cli = %d",gSocket);
        return -1;		
	}
    ///����sockaddr_in
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);  ///�������˿�
    //servaddr.sin_addr.s_addr = inet_addr("192.168.0.131");  ///������ip
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//servaddr.sin_addr.s_addr = inet_addr("192.168.3.69");
	//if(set_socket_nonblock(gSocket) == -1)

    //���ӷ��������ɹ�����0�����󷵻�-1
    if (connect(gSocket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
       perror("connect");
	   ERROR_LOG("Fail to connect server!\n");
       return -2;
    }
	if(setSocketNonblock(gSocket) == 0)//����gSocketΪ������
		printf("Succ to set gSocket nonblock!\n");	
	return gSocket;	
}

int reSocketConnect()
{
	close(gSocket);
	socketConnect();
	sleep(2);
	return 0;
}

/** 
  *@return  
  * 0 success�� 
  *-1 failure,errno==EPIPE, �Զ˽���fd���ص� 
  *-2 failure,����Ͽ����߶Զ˳�ʱδ�ٷ������� 
  *-3 failure,other errno 
**/  
int socketNonblockSend(int fd, unsigned char* buffer,  unsigned int length, unsigned long timeout)  
{  
    unsigned int bytes_left;  //�޷���  
    long long written_bytes;  //�з���  
    unsigned char* ptr;  
    ptr = buffer;  
    bytes_left = length;  
    fd_set writefds;  
    struct timeval tv;  
    int ret = 0; 
    if(length == 0 || buffer == NULL){  
        ERROR_LOG("buffer point is NULL or length is zero\n");  
        return 0;  
    }  	
    while(bytes_left > 0){  
        //written_bytes = send(fd, ptr, bytes_left, MSG_NOSIGNAL);  
		written_bytes = send(fd, ptr, bytes_left, MSG_NOSIGNAL);  
        if(written_bytes < 0){  
            if(errno == EINTR )     //�����ź��жϣ�ûд�ɹ��κ�����  
                written_bytes = 0;  
            else if(errno == EWOULDBLOCK){    //��EAGAIN��socket�ں˻�������,�������߶Ͽ�  
                FD_ZERO(&writefds);  
                FD_SET(fd, &writefds);  
                tv.tv_sec = timeout/1000000;  
                tv.tv_usec = timeout%1000000;  
                ret = select(fd+1, NULL, &writefds, NULL, &tv); //����,err:0 timeout err:-1 �����errno  
                if(ret == 0){    //��ʱ���ж�Ϊ���߶Ͽ�  
                    ERROR_LOG("select error:%s\n", strerror(errno));  
                    return -2;  
                }  
                else if(ret < 0 && errno != EINTR) {  
                    ERROR_LOG("select error:%s\n", strerror(errno));  
                    return -2;
                }  
                written_bytes = 0;  //δ��ʱ���ж�Ϊsocket�����������µ���������  
            }  
            else if(errno == EPIPE){     //�����׽��ֵı��ض��ѹر�.(��Զ˱�kill��)  
                ERROR_LOG("write socket error %d:%s\n", errno, strerror(errno));  
                return -1;  
            }  
            else{       //��������  
                ERROR_LOG("write socket error %d:%s\n", errno, strerror(errno));  
                return -3;  
            }  
        }  
        bytes_left -= written_bytes;  
        ptr += written_bytes;  
    }  
    return 0;  
}  


int checkConn(int sock_cli)
{
	int ret = 0;
	ret = socketNonblockSend(sock_cli, probePacket, 28, CHECKSOCKTIMEOUT);
	if(ret < 0)
	{
		ERROR_LOG("Server exception, please check the socket of server!");
	}
    return ret;	
}

int setDragTransData(AXIS CoordinateS, AXIS CoordinateE, TRANSINFO *transInfo)
{
	transInfo->scale = htonl(10000);
	transInfo->x = 0;
	transInfo->y = 0;
	if(CoordinateS.x < 0 || CoordinateS.y < 0 || CoordinateE.x < 0 || CoordinateE.y < 0)//����
		return -1;

	if(gSetting.mode == 1){
		transInfo->dragStart_x = htonl(CoordinateS.x * SCREENWIDTH / gScreenInfo.x.max);
		transInfo->dragStart_y = htonl(CoordinateS.y * SCREENHEIGTH / gScreenInfo.y.max);
		transInfo->dragEnd_x = htonl(CoordinateE.x * SCREENWIDTH / gScreenInfo.x.max);
		transInfo->dragEnd_y = htonl(CoordinateE.y * SCREENHEIGTH / gScreenInfo.y.max);
	}
	
	if(gSetting.mode == 2){
		transInfo->dragStart_x = htonl(CoordinateS.x * SCREENWIDTH / 5801);
		transInfo->dragStart_y = htonl(CoordinateS.y * SCREENHEIGTH / 4095);
		transInfo->dragEnd_x = htonl(CoordinateE.x * SCREENWIDTH / 5801);
		transInfo->dragEnd_y = htonl(CoordinateE.y * SCREENHEIGTH / 4095);
	}

	return 0;

}

//static int flag = 0;

int setSingleDragSendData(TOUCHPOINT point0, TOUCHPOINT point1, TRANSINFO *transInfo)
{

	transInfo->scale = htonl(10000);
	transInfo->x = 0;
	transInfo->y = 0;
	if(point0.code_53 < 0 || point0.code_54 < 0 || point1.code_53 < 0 || point1.code_54 < 0)//����
		return -1;

	if(gSetting.mode == 1){
		transInfo->dragStart_x = htonl(point0.code_53 * SCREENWIDTH / gScreenInfo.x.max);
		transInfo->dragStart_y = htonl(point0.code_54 * SCREENHEIGTH / gScreenInfo.y.max);
		transInfo->dragEnd_x = htonl(point1.code_53 * SCREENWIDTH / gScreenInfo.x.max);
		transInfo->dragEnd_y = htonl(point1.code_54 * SCREENHEIGTH / gScreenInfo.y.max);
	}
	
	if(gSetting.mode == 2){
		transInfo->dragStart_x = htonl(point0.code_53 * SCREENWIDTH / 5801);
		transInfo->dragStart_y = htonl(point0.code_54 * SCREENHEIGTH / 4095);
		transInfo->dragEnd_x = htonl(point1.code_53 * SCREENWIDTH / 5801);
		transInfo->dragEnd_y = htonl(point1.code_54 * SCREENHEIGTH / 4095);
	}

	return 0;
}

int setDoubleDragSendData(TWOPOINTDRAG doubleMoveCenter, TRANSINFO *transInfo)
{
	if(doubleMoveCenter.startX < 0 || doubleMoveCenter.startY < 0 || doubleMoveCenter.endX < 0 || doubleMoveCenter.endY)
		return -1;
	transInfo->scale = htonl(10000);
	transInfo->x = 0;
	transInfo->y = 0;	
	
	if(gSetting.mode == 1){
		transInfo->dragStart_x = htonl(doubleMoveCenter.startX * SCREENWIDTH / gScreenInfo.x.max);
		transInfo->dragStart_y = htonl(doubleMoveCenter.startY * SCREENHEIGTH / gScreenInfo.y.max);
		transInfo->dragEnd_x = htonl(doubleMoveCenter.endX * SCREENWIDTH / gScreenInfo.x.max);
		transInfo->dragEnd_y = htonl(doubleMoveCenter.endY * SCREENHEIGTH / gScreenInfo.y.max);
	}
	
	if(gSetting.mode == 2){
		transInfo->dragStart_x = htonl(doubleMoveCenter.startX * SCREENWIDTH / 5801);
		transInfo->dragStart_y = htonl(doubleMoveCenter.startY * SCREENHEIGTH / 4095);
		transInfo->dragEnd_x = htonl(doubleMoveCenter.endX * SCREENWIDTH / 5801);
		transInfo->dragEnd_y = htonl(doubleMoveCenter.endY * SCREENHEIGTH / 4095);
	}	
	return 0;
}


int setScaleSendData(COORDINATEINFO coordinateInfo,int scale,TRANSINFO *transInfo)
{
	if(scale > (BASESCALE * POWERMAX) || scale < (BASESCALE * POWERMIN)){
		return -1;
	}
	//INFO_LOG("Send Data, Center of X:%d, Center of Y:%d, Scale:%d \n",coordinateInfo.center_x, coordinateInfo.center_y, scale);
	if(scale == 10000 || NULL == transInfo)
		return -1;

	if(gSetting.mode == 1){
		if(coordinateInfo.center_x > gScreenInfo.x.max || coordinateInfo.center_y > gScreenInfo.y.max)
			return -1;		
		transInfo->x = htonl(coordinateInfo.center_x * SCREENWIDTH / gScreenInfo.x.max);
		transInfo->y = htonl(coordinateInfo.center_y * SCREENHEIGTH / gScreenInfo.y.max);
	}
	else if(gSetting.mode == 2){
		if(coordinateInfo.center_x > 5801 || coordinateInfo.center_y > 4095)
			return -1;		
		transInfo->x = htonl(coordinateInfo.center_x * SCREENWIDTH / 5801);
		transInfo->y = htonl(coordinateInfo.center_y * SCREENHEIGTH / 4095);
	}
	transInfo->scale = htonl(scale);
	transInfo->dragStart_x = 0;
	transInfo->dragStart_y = 0;
	transInfo->dragEnd_x = 0;
	transInfo->dragEnd_y = 0;	
	return 0;
}

int sendData(int sock_cli, char* data, int lenData)
{
	int nByte = 0;
	nByte = send(sock_cli, data, lenData, 0);
	if(nByte == lenData){
		INFO_LOG("send data len =%d", nByte);
		return 0;
	}
	else if (nByte < 0){
		return -1;		
	}
		
	/*
    unsigned int bytes_left;  //�޷���  
    long long written_bytes;  //�з��� 
    char* ptr; 
    ptr = data;
    if(lenData == 0 || data == NULL){  
        ERROR_LOG("data point is NULL or length is zero\n");  
        return 0;  
    }  
    while(bytes_left > 0){
		written_bytes = send(fd, ptr, bytes_left, 0);
		if(written_bytes < 0)
			return -1;
		bytes_left -= written_bytes;  
		ptr += written_bytes;
	}
	*/
	return 0;
}

int initLog()
{
	if (logInit(LOG_PATH,LOG_HISTORY_PATH,"mtGesture", NULL) != 0) {
		fprintf(stderr,"ERROR: log_init.");
		ERROR_LOG("Fail to init log");
		exit(1);
	}
	return 0;
}

void touchInfoGather(void)
{
	getTouchInfo(gSetting.fileName, gSetting.mode);	
	//return;
}

int initTouchInfoGatherThread()
{
    /* ��ʼ���߳����Խṹ�� */
    pthread_attr_init( &gTouchInfoIDAttr);
	
    if ( 0 != pthread_create(&gTouchInfoID, &gTouchInfoIDAttr, (void *)touchInfoGather, NULL))
    {
		ERROR_LOG("RUN Touch Info Gather THREAD ERROR!\n");
        return -1;
    }
	return 0;
}

void gestureInfoTrans(void)
{
	TOUCHPOINT touchInfo; 
	
	COORDINATEINFO coordinate0Base;
	COORDINATEINFO coordinate1Base;	

	int scale = 0;
	TRANSINFO transInfo;
	TOUCHPOINT point;

	SAMPLETOUCHINFO mapMousePoint;//���е��㴥����Ϣ�ĵ�
	
	AXIS D_value[2];
	AXIS dragS_to_Epoint[2] = {{0,0},{0,0}};

	AXIS DValue[10];
	TOUCHPOINT tmpPointsInfo[2][10];
	TOUCHPOINT ScalePoint[2][2];
	int GestureFlag = 0;// 1,drag; 2,scale
	struct timeval tv;
	int k = 0;
	int i = 0;
	int j = 0;
	int index[MAX_FINGERS_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	
	char data[28] = {0};
	int ret = 0;

	memset(D_value, 0, sizeof(AXIS));
		
	if(gSetting.mode == 1){
		while(1){
			ret = socketConnect();
			if(ret == -1 || ret == -2)
			{
				if(ret == -2)
					close(gSocket);
				ERROR_LOG("Fail to connect server!");
				//exit(1);
				sleep(2);
				continue;
			}	
			else
				break;
		}
	}
	SEND_LOG("time(s)\ttime(us)\tscale\tcenter_x\tcenter_y\tdrag_start_x\tdrag_start_y\tdrag_end_x\tdrag_end_y");
	while(1)
	{
		memset(&touchInfo,-2,sizeof(TOUCHPOINT));
		memset(&point, 0,sizeof(point));
		memset(tmpPointsInfo, 0, sizeof(TOUCHPOINT)*20);
		memset(DValue, 0, sizeof(AXIS)*2);
		memset(dragS_to_Epoint, 0, sizeof(AXIS)*2);
		memset(ScalePoint, 0, sizeof(TOUCHPOINT)*4);
		memset(&coordinate0Base, 0, sizeof(COORDINATEINFO));
		memset(&coordinate1Base, 0, sizeof(COORDINATEINFO));
		if(gQueueTouchInfo.QueueNodeCount == 0)
		{
			usleep(200);
			continue;
		}
		ret = ComGetQueueNode(&gQueueTouchInfo, (void *)&touchInfo);
		if(ret == 1)
		{
			//��������
			if(!samplePointInfo(touchInfo, &point)){				
				if(point.code_0 > 0 || point.code_1 > 0){//��Ӧ���Ĵ�������Ϣ�������
					if(point.code_57 == -1){
						if(existFingerCount > 0){//���ӳ��㽫ת�Ƶ��������ָ
							//mapMousePoint.pointInfo.code_0 = point.code_0;
							//mapMousePoint.pointInfo.code_1 = point.code_1;
							memcpy(&mapMousePoint.pointInfo, &touchInfo, sizeof(TOUCHPOINT));
						}
						else if(existFingerCount == 0){//��Ļ���޴��������
							memset(&mapMousePoint, 0, sizeof(SAMPLETOUCHINFO));
						}
					}
					else{
						if(mapMousePoint.savedFlag == 0){//ʹ��mapMousePoint�ݴ��Ӧ������Ϣ���Թ�������ʹ��
							memcpy(&mapMousePoint.pointInfo, &point, sizeof(TOUCHPOINT));
							mapMousePoint.savedFlag = 1;
						}
						else if(mapMousePoint.savedFlag == 1){							
							if(mapMousePoint.pointInfo.code_47 != point.code_47){
								for(i = 0; i < MAX_FINGERS_COUNT; i++){
									if(multiFingersBase[0][i].pointInfo.code_47 == point.code_47){
										if(multiFingersBase[1][i].savedFlag ==1){
											complementSampleInfo(multiFingersBase[1][i].pointInfo, &point);
										}
										else{
											complementSampleInfo(multiFingersBase[0][i].pointInfo, &point);
										}
										//memcpy(&mapMousePoint.pointInfo, &point, sizeof(TOUCHPOINT));
										//complementSampleInfo(point, &mapMousePoint.pointInfo);
										updateSampleInfo(point, &mapMousePoint.pointInfo);
										//mapMousePoint.savedFlag = 1;
										break;
									}
								}
							}
							else{
								updateSampleInfo(point, &mapMousePoint.pointInfo);
								memcpy(&point, &mapMousePoint.pointInfo, sizeof(TOUCHPOINT));
							}
						}							
					}
				}
					
				if(point.code_47 < 0){//ȷ����ָ����
					if(point.code_57 == -1){
						memset(singleFingerBase, 0  ,  sizeof(SAMPLETOUCHINFO) *2 );
						if(detectFingerCount > 0)
							detectFingerCount--;
						continue;
					}		
					singleEnableFlag = 1;
					if(singleFingerBase[0].savedFlag == 0){
						memcpy(&singleFingerBase[0].pointInfo, &point, sizeof(TOUCHPOINT));
						singleFingerBase[0].savedFlag = 1;
					}
					else if(singleFingerBase[0].savedFlag == 1){
						if(singleFingerBase[1].savedFlag == 0){
							memcpy(&singleFingerBase[1].pointInfo, &point, sizeof(TOUCHPOINT));
							singleFingerBase[1].savedFlag = 1;
							detectFingerCount++;
						}
						else if(singleFingerBase[1].savedFlag == 1){
							complementSampleInfo(singleFingerBase[0].pointInfo, &singleFingerBase[1].pointInfo);
							memcpy(&singleFingerBase[0].pointInfo, &singleFingerBase[1].pointInfo, sizeof(TOUCHPOINT));
							memcpy(&singleFingerBase[1].pointInfo, &point, sizeof(TOUCHPOINT));
						}
					}
				}
				else{//��ָ���ָ����
					singleEnableFlag = 0;
					if(point.code_47 > 9){
						invalidSlotValueFlag = 1;
					}
					else{
						for(k = 0; k < MAX_FINGERS_COUNT; k++){
							if(point.code_47 == multiFingersBase[0][k].pointInfo.code_47){//multiFingersBase
								if(point.code_57 == -1){
									memset(&multiFingersBase[0][k], 0, sizeof(SAMPLETOUCHINFO));
									memset(&multiFingersBase[1][k], 0, sizeof(SAMPLETOUCHINFO));
									if(point.code_47 == mapMousePoint.pointInfo.code_47){
										memset(&mapMousePoint, 0, sizeof(SAMPLETOUCHINFO));
									}
									if(detectFingerCount > 0)
										detectFingerCount--;									
									break;
								}
								if(multiFingersBase[0][k].savedFlag == 0){
									memcpy(&multiFingersBase[0][k].pointInfo, &point, sizeof(TOUCHPOINT));
									multiFingersBase[0][k].savedFlag = 1;
								}
								else if(multiFingersBase[0][k].savedFlag == 1){
									if(multiFingersBase[1][k].savedFlag == 0){
										//complementSampleInfo(multiFingersBase[0][index[0]].pointInfo, &multiFingersBase[1][index[0]].pointInfo);
										memcpy(&multiFingersBase[1][k].pointInfo, &point, sizeof(TOUCHPOINT));
										multiFingersBase[1][k].savedFlag = 1;
										detectFingerCount++;
									}									
									else if(multiFingersBase[1][k].savedFlag == 1){
										complementSampleInfo(multiFingersBase[0][k].pointInfo, &multiFingersBase[1][k].pointInfo);
										memcpy(&multiFingersBase[0][k].pointInfo, &multiFingersBase[1][k].pointInfo, sizeof(TOUCHPOINT));
										memcpy(&multiFingersBase[1][k].pointInfo, &point, sizeof(TOUCHPOINT));
									}
								}
								break;
							}
						}
					}
				}
			}
			
			//����ʶ��
			if(existFingerCount <= 0){
				for(i = 0; i < MAX_FINGERS_COUNT; i++){//�ò�������Ҫ������޴˲������ܵ���ĳ�������Ժ���������������޷���Ч
					multiFingersBase[0][i].pointInfo.code_47 = i;
					multiFingersBase[1][i].pointInfo.code_47 = i;
				}				
				releaseFlag = 1;
				continue;
			}
			
			if(detectFingerCount != existFingerCount){
				for(i = 0; i < MAX_FINGERS_COUNT; i++){//�ò�������Ҫ������޴˲������ܵ���ĳ�������Ժ���������������޷���Ч
					multiFingersBase[0][i].pointInfo.code_47 = i;
					multiFingersBase[1][i].pointInfo.code_47 = i;
				}				
				continue;
			}
			if(detectFingerCount >= 1){
				if(singleEnableFlag == 1){
					ret = calSinglePointMoveDistance(singleFingerBase[0].pointInfo, singleFingerBase[1].pointInfo);
					if(ret == 0){
						setSingleDragSendData(singleFingerBase[0].pointInfo, singleFingerBase[1].pointInfo, &transInfo);
						GestureFlag = 1;
						INFO_LOG("single dragging ret = %d\n",ret);
						singleDragTimeFlag++;
					}	
					detectFingerCount = 0;
					memcpy(&singleFingerBase[0].pointInfo, &singleFingerBase[1].pointInfo, sizeof(TOUCHPOINT));
					memset(&singleFingerBase[1], 0, sizeof(SAMPLETOUCHINFO));
				}
				else if(singleEnableFlag == 0){
					memset(singleFingerBase, 0  ,  sizeof(SAMPLETOUCHINFO) *2 );
					for(i = 0; i < MAX_FINGERS_COUNT; i++){
						if(multiFingersBase[1][i].savedFlag == 1){
							index[j++] = i;
						}
						else
							continue;
					}
					if(detectFingerCount == 1){
						complementSampleInfo(multiFingersBase[0][index[0]].pointInfo, &multiFingersBase[1][index[0]].pointInfo);
						ret = calSinglePointMoveDistance(multiFingersBase[0][index[0]].pointInfo, multiFingersBase[1][index[0]].pointInfo);
						if(ret == 0){
							setSingleDragSendData(multiFingersBase[0][index[0]].pointInfo, multiFingersBase[1][index[0]].pointInfo, &transInfo);
							GestureFlag = 1;
							INFO_LOG("single--m dragging ret = %d\n",ret);
							singleDragTimeFlag++;
						}
						memcpy(&multiFingersBase[0][index[0]].pointInfo, &multiFingersBase[1][index[0]].pointInfo, sizeof(TOUCHPOINT));
						memset(&multiFingersBase[1][index[0]], 0, sizeof(SAMPLETOUCHINFO));
						//setDragTransData(dragS_to_Epoint[0], dragS_to_Epoint[1], &transInfo);
						
					}
					else if(detectFingerCount == 2){
						
						complementSampleInfo(multiFingersBase[0][index[0]].pointInfo, &multiFingersBase[1][index[0]].pointInfo);
						complementSampleInfo(multiFingersBase[0][index[1]].pointInfo, &multiFingersBase[1][index[1]].pointInfo);

						calCenterCoordinate(multiFingersBase[0][index[0]].pointInfo,  multiFingersBase[0][index[1]].pointInfo, &coordinate0Base);
						calCenterCoordinate(multiFingersBase[1][index[0]].pointInfo,  multiFingersBase[1][index[1]].pointInfo, &coordinate1Base);
						if((scale = calScaling(coordinate0Base.variance, coordinate1Base.variance)) != -1)
						{
							INFO_LOG("two figners scaling...");
							setScaleSendData(coordinate0Base, scale, &transInfo);
							dragEnableFlag = 1;
							singleDragTimeFlag = 0;
							GestureFlag = 2;
						}	

						calCoordinateDeviation(multiFingersBase[0][index[0]].pointInfo, multiFingersBase[0][index[1]].pointInfo, &D_value[0].x, &D_value[0].y);
						calCoordinateDeviation(multiFingersBase[1][index[0]].pointInfo, multiFingersBase[1][index[1]].pointInfo, &D_value[1].x, &D_value[1].y);
						if(calD_value(D_value) == 0){//������ק������Ч
							singleDragTimeFlag++;
							INFO_LOG("two figners dragging...");
							dragS_to_Epoint[0].x = coordinate0Base.center_x;
							dragS_to_Epoint[0].y = coordinate0Base.center_y;
							dragS_to_Epoint[1].x = coordinate1Base.center_x;
							dragS_to_Epoint[1].y = coordinate1Base.center_y;
							GestureFlag = 1;
							setDragTransData(dragS_to_Epoint[0], dragS_to_Epoint[1], &transInfo);
						}

						memcpy(&multiFingersBase[0][index[0]].pointInfo, &multiFingersBase[1][index[0]].pointInfo, sizeof(TOUCHPOINT));
						memcpy(&multiFingersBase[0][index[1]].pointInfo, &multiFingersBase[1][index[1]].pointInfo, sizeof(TOUCHPOINT));
						memset(&multiFingersBase[1][index[0]], 0, sizeof(SAMPLETOUCHINFO));
						memset(&multiFingersBase[1][index[1]], 0, sizeof(SAMPLETOUCHINFO));
					}
					else if(detectFingerCount >= 2 && detectFingerCount <= 10){
						//��ȫ���Ĵ�����Ϣ
						for(i = 0; i < detectFingerCount; i++){
							complementSampleInfo(multiFingersBase[0][index[i]].pointInfo, &multiFingersBase[1][index[i]].pointInfo);
							memcpy(&tmpPointsInfo[0][i], &multiFingersBase[0][index[i]].pointInfo, sizeof(TOUCHPOINT));
							memcpy(&tmpPointsInfo[1][i], &multiFingersBase[1][index[i]].pointInfo, sizeof(TOUCHPOINT));
						}
						multiPointCoordinateDeviation(tmpPointsInfo, detectFingerCount, &GestureFlag, dragS_to_Epoint, ScalePoint);
						if(GestureFlag == 1){
							singleDragTimeFlag++;
							setDragTransData(dragS_to_Epoint[0], dragS_to_Epoint[1], &transInfo);
						}
						else if(GestureFlag == 2){
							dragEnableFlag = 1;
							singleDragTimeFlag = 0;
							calCenterCoordinate(ScalePoint[0][0],  ScalePoint[0][1], &coordinate0Base);
							calCenterCoordinate(ScalePoint[1][0],  ScalePoint[1][1], &coordinate1Base);
							if((scale = calScaling(coordinate0Base.variance, coordinate1Base.variance)) != -1)
							{
								INFO_LOG("multi figners scaling...");
								setScaleSendData(coordinate0Base, scale, &transInfo);
							}							
						}
						for(i = 0; i < detectFingerCount; i++){
							memcpy(&multiFingersBase[0][index[i]].pointInfo, &multiFingersBase[1][index[i]].pointInfo, sizeof(TOUCHPOINT));
							memset(&multiFingersBase[1][index[i]], 0, sizeof(SAMPLETOUCHINFO));						
						}
					}


					detectFingerCount = 0;
					j = 0;
					memset(index, 0, sizeof(int));
					for(i = 0; i < MAX_FINGERS_COUNT; i++){//�ò�������Ҫ������޴˲������ܵ���ĳ�������Ժ���������������޷���Ч
						multiFingersBase[0][i].pointInfo.code_47 = i;
						multiFingersBase[1][i].pointInfo.code_47 = i;
					}
				}
			}
			else
				continue;

			//�������ݴ���
			//���˲��Ϸ���transInfo
			if(ntohl(transInfo.scale) > (BASESCALE * POWERMAX) || ntohl(transInfo.scale) < (BASESCALE * POWERMIN)){
				continue;
			}
			if(ntohl(transInfo.x) > SCREENWIDTH || ntohl(transInfo.y) > SCREENHEIGTH || ntohl(transInfo.dragStart_x) > SCREENWIDTH ||  ntohl(transInfo.dragStart_y) > SCREENHEIGTH || ntohl(transInfo.dragEnd_x) > SCREENWIDTH ||  ntohl(transInfo.dragEnd_y) > SCREENHEIGTH){
				continue;
			}
			if(ntohl(transInfo.x) < 0 || ntohl(transInfo.y) < 0 || ntohl(transInfo.dragStart_x) < 0 ||  ntohl(transInfo.dragStart_y) < 0 || ntohl(transInfo.dragEnd_x) < 0 ||  ntohl(transInfo.dragEnd_y) < 0){
				continue;
			}
			if((ntohl(transInfo.dragStart_x) == ntohl(transInfo.dragEnd_x)) &&(ntohl(transInfo.dragStart_y) == ntohl(transInfo.dragEnd_y))){
				continue;
			}
			
			if(GestureFlag == 2){
				if(GestureFlag == 2){
					if(gSetting.mode == 2){
						gettimeofday(&tv, NULL);
						SEND_LOG("%ld\t%06ld\t%d\t%d\t%d\t%d\t%d\t%d\t%d", tv.tv_sec, tv.tv_usec, ntohl(transInfo.scale), ntohl(transInfo.x), ntohl(transInfo.y), 0, 0, 0, 0);
						continue;
					}		
					else if(gSetting.mode == 1){
						memcpy(data, &transInfo, 28);
						while(1){
							if(checkConn(gSocket) == 0){
								gettimeofday(&tv, NULL);
								if(sendData(gSocket, data, 28)==0)//��������
									SEND_LOG("%ld\t%06ld\t%d\t%d\t%d\t%d\t%d\t%d\t%d", tv.tv_sec, tv.tv_usec, ntohl(transInfo.scale), ntohl(transInfo.x), ntohl(transInfo.y), 0, 0, 0, 0);
								break;							
							}
							else{
								reSocketConnect();
								continue;
							}
						}
					}
				}
			}
			else if(GestureFlag == 1){
				if(gSetting.mode == 2){
					dragEnableFlag = 1;
				}
				if(dragEnableFlag == 1){
					if(gSetting.mode == 2){
						gettimeofday(&tv, NULL);
						SEND_LOG("%ld\t%06ld\t%d\t%d\t%d\t%d\t%d\t%d\t%d", tv.tv_sec, tv.tv_usec, 10000, 0, 0, ntohl(transInfo.dragStart_x), ntohl(transInfo.dragStart_y), ntohl(transInfo.dragEnd_x), ntohl(transInfo.dragEnd_y));
						//continue;
					}
					else if(gSetting.mode == 1){
						memcpy(data, &transInfo, 28);
						while(1){
							if(checkConn(gSocket) == 0){
								gettimeofday(&tv, NULL);
								if(releaseFlag == 1){
									if(sendData(gSocket, data, 28) == 0)//��������
										SEND_LOG("%ld\t%06ld\t%d\t%d\t%d\t%d\t%d\t%d\t%d", tv.tv_sec, tv.tv_usec, 10000, 0, 0, ntohl(transInfo.dragStart_x), ntohl(transInfo.dragStart_y), ntohl(transInfo.dragEnd_x), ntohl(transInfo.dragEnd_y));
									releaseFlag = 0;
								}
								else if(releaseFlag == 0){
									if(singleDragTimeFlag > FILTER_DRAG_GESTURE){
										if(sendData(gSocket, data, 28) == 0)//��������
											SEND_LOG("%ld\t%06ld\t%d\t%d\t%d\t%d\t%d\t%d\t%d", tv.tv_sec, tv.tv_usec, 10000, 0, 0, ntohl(transInfo.dragStart_x), ntohl(transInfo.dragStart_y), ntohl(transInfo.dragEnd_x), ntohl(transInfo.dragEnd_y));
									}
								}

								break;
							}
							else{
								reSocketConnect();
								continue;
							}
						}	
					}
				}
			}
		}
		else
		{
			continue;
			ERROR_LOG("Fail to get node from queue\n");
		}
		GestureFlag = 0;
		usleep(200);		
	}
}

int initGestureThread()
{
    /* ��ʼ���߳����Խṹ�� */
    pthread_attr_init( &gGestureInfoIDAttr);
	
    if ( 0 != pthread_create(&gGestureInfoID, &gGestureInfoIDAttr, (void *)gestureInfoTrans, NULL))
    {
        //printf( "RUN Touch Info Gather THREAD ERROR!\n");
		ERROR_LOG("RUN Touch Info Gather THREAD ERROR!\n");
        return -1;
    }
	return 0;
}

void usge()
{
	printf("\t* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
	printf("\t* need more arguements, format like ./Gesture -r /dev/input/event16       *\n");
	printf("\t*                                                                         *\n");
	printf("\t* Usge:                                                                   *\n");
	printf("\t*\t -r [device file name] ;like /dev/input/event16                   *\n");
	printf("\t*\t -f [saved file name]  ;like mtGesture_free_current.log           *\n");
	printf("\t* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
}

int getVersion()
{
	FILE *fp = NULL;
	if ((fp = fopen("mtGestureVersion.txt", "w")) < 0) {
		perror("mtGestureVersion:");
		ERROR_LOG("Fail to open mtGestureVersion");
		return -1;
	}	
	fprintf(fp,"%d.%02d",MAJOR_VERSION, MINOR_VERSION);
	INFO_LOG("software version:%d.%02d\n",MAJOR_VERSION, MINOR_VERSION);
	fclose(fp);
	return 0;
	
}

int main(int argc, char **argv)
{
	int ret = 0;
	char ch;
	int i = 0;
	int j = 0;
	
	memset(singleFingerBase, 0, sizeof(SAMPLETOUCHINFO)*2);
	memset(multiFingersBase, 0, sizeof(SAMPLETOUCHINFO)*20);

	//��ʼ����������Ϣ�Ļ�����
	for(j = 0; j< 2; j++){
		for(i = 0; i < MAX_FINGERS_COUNT; i++){
			multiFingersBase[j][i].pointInfo.code_47 = i;
		}
	}

	memset(gInputDeviceFile, '0', 128);
	ret = getVersion();
	if(ret == -1){
		ERROR_LOG("Fail to write version info to mtGestureVersion.txt\n");
	}
	if (argc < 3) {
		usge();
		return 1;
	}
	while((ch = getopt(argc,argv,"r:f:"))!=-1){
		switch(ch){
			case 'r':
				gSetting.mode = 1;
				strcpy(gSetting.fileName, optarg);
				break;
			case 'f':
				gSetting.mode = 2;
				strcpy(gSetting.fileName, optarg);
				break;
			default:
				break;
		}
	}
	INFO_LOG("touch screen device file : %s\n",gSetting.fileName);
	//��ʼ����־��Ϣ
	initLog();

	//�������У��ݴ津������Ϣ
	ret = ComInitQueue(&gQueueTouchInfo,sizeof(TOUCHPOINT), NULL, 0, NULL);
	if(ret == COM_SUCC)
	{
		INFO_LOG("Succ to create touching info queue\n");
	}
	else {
		ERROR_LOG("Fail to Create Touch Info Queue!\n");
		exit(-1);
	}		
	
	//���������̣߳�һ����ȡ��������Ϣ����һ��������������
	ret = initTouchInfoGatherThread();
	if(ret < 0){
		//printf("Fail to start initTouchInfoGatherThread Thread\n");
		ERROR_LOG("Fail to start initTouchInfoGatherThread Thread\n");
		exit(1);
	}
	ret = initGestureThread();
	if(ret < 0){
		//printf("Fail to start initGestureThread Thread\n");
		ERROR_LOG("Fail to start initGestureThread Thread\n");
		exit(1);
	}

	pthread_join(gTouchInfoID,NULL);
	pthread_join(gGestureInfoID,NULL);
	close(gSocket);
	ComDestroyQueue (&gQueueTouchInfo);
	return 0;
}
