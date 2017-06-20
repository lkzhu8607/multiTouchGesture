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

#define CHECKSOCKTIMEOUT 2000000 //探测包的超时时间


#define BASESCALE 10000 //基础倍率，如原图的时候为10000
#define POWERMAX 1.07  //单次最大的放大倍率
#define POWERMIN 0.8  //单次最小的缩小倍率

#define MAJOR_VERSION 1   //主版本号
#define MINOR_VERSION 00  //次版本号

int singleDragTimeFlag = 0;
pthread_t gTouchInfoID = 0;
pthread_t gScaleInfoID = 0;
pthread_attr_t  gTouchInfoIDAttr; 
pthread_attr_t  gScaleInfoIDAttr;

//COM_QUEUE_HDL gQueueTouchInfo;
char gInputDeviceFile[128];
int gSocket = 0;

/*typedef struct SETTINGTAG{
	int mode;// 1:表示直接从触摸屏上取信号源信息; 2:表示从文件中获取信号源信息
	char fileName[128];//当mode = 1时，保存的是设备文件; 当mode = 2时，保存的是已保存触屏信号源信息的文件名
}SETTING;*/
SETTING gSetting;

AXIS scaleCenterPoint;//拖拽以后缩放的中心点记录

//探测包
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
return：-1,创建socket或连接服务器失败
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
    ///定义sockaddr_in
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);  ///服务器端口
    //servaddr.sin_addr.s_addr = inet_addr("192.168.0.131");  ///服务器ip
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//servaddr.sin_addr.s_addr = inet_addr("192.168.3.69");
	//if(set_socket_nonblock(gSocket) == -1)

    //连接服务器，成功返回0，错误返回-1
    if (connect(gSocket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
       perror("connect");
	   ERROR_LOG("Fail to connect server!\n");
       return -2;
    }
	if(setSocketNonblock(gSocket) == 0)//设置gSocket为非阻塞
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
  * 0 success， 
  *-1 failure,errno==EPIPE, 对端进程fd被关掉 
  *-2 failure,网络断开或者对端超时未再发送数据 
  *-3 failure,other errno 
**/  
int socketNonblockSend(int fd, unsigned char* buffer,  unsigned int length, unsigned long timeout)  
{  
    unsigned int bytes_left;  //无符号  
    long long written_bytes;  //有符号  
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
            if(errno == EINTR )     //由于信号中断，没写成功任何数据  
                written_bytes = 0;  
            else if(errno == EWOULDBLOCK){    //即EAGAIN，socket内核缓存区满,或者网线断开  
                FD_ZERO(&writefds);  
                FD_SET(fd, &writefds);  
                tv.tv_sec = timeout/1000000;  
                tv.tv_usec = timeout%1000000;  
                ret = select(fd+1, NULL, &writefds, NULL, &tv); //阻塞,err:0 timeout err:-1 错误见errno  
                if(ret == 0){    //超时，判定为网线断开  
                    ERROR_LOG("select error:%s\n", strerror(errno));  
                    return -2;  
                }  
                else if(ret < 0 && errno != EINTR) {  
                    ERROR_LOG("select error:%s\n", strerror(errno));  
                    return -2;
                }  
                written_bytes = 0;  //未超时，判定为socket缓冲区满导致的网络阻塞  
            }  
            else if(errno == EPIPE){     //连接套接字的本地端已关闭.(如对端被kill掉)  
                ERROR_LOG("write socket error %d:%s\n", errno, strerror(errno));  
                return -1;  
            }  
            else{       //其他错误  
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

static TWOPOINTDRAG lastBase[2];
static int flag = 0;

int setSingleDragSendData(TOUCHPOINT point0, TOUCHPOINT point1, TRANSINFO *transInfo)
{

	transInfo->scale = htonl(10000);
	transInfo->x = 0;
	transInfo->y = 0;
	if(point0.code_53 < 0 || point0.code_54 < 0 || point1.code_53 < 0 || point1.code_54 < 0)//过滤
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


int setDoubleScaleSendData(COORDINATEINFO coordinateInfo,int scale,TRANSINFO *transInfo)
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
    unsigned int bytes_left;  //无符号  
    long long written_bytes;  //有符号 
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
    /* 初始化线程属性结构体 */
    pthread_attr_init( &gTouchInfoIDAttr);
	
    if ( 0 != pthread_create(&gTouchInfoID, &gTouchInfoIDAttr, (void *)touchInfoGather, NULL))
    {
		ERROR_LOG("RUN Touch Info Gather THREAD ERROR!\n");
        return -1;
    }
	return 0;
}

void scaleInfoTrans(void)
{
	TOUCHPOINT touchInfo; 
	
	COORDINATEINFO coordinate0Base;
	COORDINATEINFO coordinate1Base;	

	int scale = 0;
	TRANSINFO transInfo;
	//TOUCHPOINT point;
	TOUCHPOINT dragBase[2];
	static int singleDrag0Flag = 0;
	static int singleDrag1Flag = 0;
	
	TOUCHPOINT touchBase0[2];//第0组坐标采样
	TOUCHPOINT touchBase1[2];//第1组坐标采样
	static int base0Flag = 0;
	static int base1Flag = 0;

	struct timeval tv;
	
	char data[28] = {0};
	int ret = 0;
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
		memset(&touchInfo,-2,sizeof(touchInfo));
		if(gQueueTouchInfo.QueueNodeCount == 0)
		{
			usleep(200);
			continue;
		}
		ret = ComGetQueueNode(&gQueueTouchInfo, (void *)&touchInfo);
		if(ret == 1)
		{
			//TODO
			//INFO_LOG("succ to get a touch info\n");
			if(singleDrag0Flag == 0){
				ret = sampleBaseDragsinglePointInfo(touchInfo, &dragBase[0]);
				if(ret == 0){
					INFO_LOG("Succ to get the first dragging point");
					singleDrag0Flag = 1;
				}
			}
			else{
				ret = sampleBaseDragsinglePointInfo(touchInfo, &dragBase[1]);
				if(ret == 0){
					INFO_LOG("Succ to get the second dragging point\n");
					singleDrag1Flag = 1;
				}
				else if(ret == -2){
					INFO_LOG("Dragging point info disappear\n");
					memcpy(&dragBase[0], &dragBase[1], sizeof(TOUCHPOINT));
					memset(&dragBase[1], 0, sizeof(TOUCHPOINT));
					singleDrag0Flag = 1;
				}
				else if(ret == -3){
					//memcpy(&dragBase[0], &dragBase[1], sizeof(TOUCHPOINT)*2);
					memset(dragBase, 0, sizeof(TOUCHPOINT)*2);
					singleDrag0Flag = 0;
					singleDrag1Flag = 0;
				}
			}
			INFO_LOG("singleDrag0Flag = %d, singleDrag1Flag = %d\n",singleDrag0Flag, singleDrag1Flag);
			if(singleDrag0Flag == 1 && singleDrag1Flag == 1){
				singleDragTimeFlag++;
				ret = calSinglePointMoveDistance(dragBase[0], dragBase[1]);
				INFO_LOG("Dragging ret = %d\n",ret);
				if(ret == 0){
					ret = setSingleDragSendData(dragBase[0], dragBase[1], &transInfo);
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
								if(singleDragTimeFlag > 4 ){
									if(sendData(gSocket, data, 28) == 0)//发送数据
										SEND_LOG("%ld\t%06ld\t%d\t%d\t%d\t%d\t%d\t%d\t%d", tv.tv_sec, tv.tv_usec, 10000, 0, 0, ntohl(transInfo.dragStart_x), ntohl(transInfo.dragStart_y), ntohl(transInfo.dragEnd_x), ntohl(transInfo.dragEnd_y));
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
				singleDrag1Flag = 0;
				if(dragBase[1].code_53 > 0)
					dragBase[0].code_53 = dragBase[1].code_53;
				if(dragBase[1].code_54 > 0)
					dragBase[0].code_54 = dragBase[1].code_54;
				scaleCenterPoint.x = dragBase[0].code_53;
				scaleCenterPoint.y = dragBase[0].code_54;
				memset(&dragBase[1], 0, sizeof(TOUCHPOINT));
			}
			
			//INFO_LOG("base0Flag = %d, base1Flag = %d\n", base0Flag, base1Flag);
			if(base0Flag == 0)//获取第一组点
			{
				ret = sampleBasePointInfo(touchInfo, &touchBase0[0], &touchBase0[1]);
				if(ret == 0)
					base0Flag = 1;
				else if(ret == -2)
				{
					//INFO_LOG("The first group's the first point info disappear\n");
					memset(&touchBase0[0],0,sizeof(TOUCHPOINT));
					base0Flag = 0;
					continue;
				}
				else if(ret == -3)
				{
					//INFO_LOG("The first group's the second point info disappear\n");
					memset(&touchBase0[1],0,sizeof(TOUCHPOINT));
					base0Flag = 0;
					continue;
				}
				else
					continue;				
			}
			else//获取第二组点
			{
				ret = sampleBasePointInfo(touchInfo, &touchBase1[0], &touchBase1[1]);
				if(ret == 0){
					base1Flag = 1;
					//INFO_LOG("touchBase1[0].code_53 = %d, touchBase1[0].code_54 = %d\n", touchBase1[0].code_53, touchBase1[0].code_54);
					//INFO_LOG("touchBase1[1].code_53 = %d, touchBase1[1].code_54 = %d\n", touchBase1[1].code_53, touchBase1[1].code_54);
				}
				else if(ret == -2 || ret == -3)
				{
					if(ret == -2){
						freshValue(touchBase0[1], &touchBase1[1]);
						memset(&touchBase0,0,sizeof(TOUCHPOINT)*2);
						memcpy(&touchBase0[1],&touchBase1[1],sizeof(TOUCHPOINT));						
					}
					else if(ret == -3){
						freshValue(touchBase0[0], &touchBase1[0]);
						memset(&touchBase0,0,sizeof(TOUCHPOINT)*2);						
						memcpy(&touchBase0[0],&touchBase1[0],sizeof(TOUCHPOINT));
						//INFO_LOG("The second group's the second point info disappear\n");
					}
					memset(&touchBase1,0,sizeof(TOUCHPOINT)*2);
					base0Flag = 0;
					base1Flag = 0;
					continue;
				}
				else
					continue;
			}
			if(base1Flag == 1 && base0Flag == 1)//一次采样完毕
			{
				singleDragTimeFlag = 0;
				calCenterCoordinate(touchBase0[0],  touchBase0[1], &coordinate0Base);
				calCenterCoordinate(touchBase1[0],  touchBase1[1], &coordinate1Base);
				if((scale = calScaling(coordinate0Base.variance, coordinate1Base.variance)) != -1)
				{
					ret = setDoubleScaleSendData(coordinate1Base, scale, &transInfo);
					if(ret == -1){
						base0Flag = 0;
						base1Flag = 0;
						memset(touchBase0,0,sizeof(TOUCHPOINT)*2);
						memset(touchBase1,0,sizeof(TOUCHPOINT)*2);
						continue;
					}
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
								if(sendData(gSocket, data, 28)==0)//发送数据
									SEND_LOG("%ld\t%06ld\t%d\t%d\t%d\t%d\t%d\t%d\t%d", tv.tv_sec, tv.tv_usec, ntohl(transInfo.scale), ntohl(transInfo.x), ntohl(transInfo.y), 0, 0, 0, 0);
								break;							
							}
							else{
								reSocketConnect();
								continue;
							}
						}
					}
					memset(touchBase0,0,sizeof(TOUCHPOINT)*2);
					memcpy(touchBase0,touchBase1,sizeof(TOUCHPOINT)*2);
					memset(touchBase1,0,sizeof(TOUCHPOINT)*2);
					base1Flag = 0;
				}
				else{
					base1Flag = 0;
				}
			}
			else
				continue;
		}
		else
		{
			continue;
			ERROR_LOG("Fail to get node from queue\n");
		}
		usleep(200);
	}
}

int initScaleInfoTransThread()
{
    /* 初始化线程属性结构体 */
    pthread_attr_init( &gScaleInfoIDAttr);
	
    if ( 0 != pthread_create(&gScaleInfoID, &gScaleInfoIDAttr, (void *)scaleInfoTrans, NULL))
    {
        printf( "RUN Touch Info Gather THREAD ERROR!\n");
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
	fprintf(fp,"%d.%2d",MAJOR_VERSION, MINOR_VERSION);
	INFO_LOG("software version:%d.%02d\n",MAJOR_VERSION, MINOR_VERSION);
	fclose(fp);
	return 0;
	
}

int main(int argc, char **argv)
{
	int ret = 0;
	char ch;
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
	//初始化日志信息
	initLog();

	//创建队列，暂存触摸点信息
	ret = ComInitQueue(&gQueueTouchInfo,sizeof(TOUCHPOINT), NULL, 0, NULL);
	if(ret == COM_SUCC)
	{
		INFO_LOG("Succ to create touching info queue\n");
	}
	else {
		ERROR_LOG("Fail to Create Touch Info Queue!\n");
		exit(-1);
	}		
	
	//创建两个线程，一个获取触摸点信息，另一个发送缩放数据
	ret = initTouchInfoGatherThread();
	if(ret < 0){
		//printf("Fail to start initTouchInfoGatherThread Thread\n");
		ERROR_LOG("Fail to start initTouchInfoGatherThread Thread\n");
		exit(1);
	}
	ret = initScaleInfoTransThread();
	if(ret < 0){
		//printf("Fail to start initScaleInfoTransThread Thread\n");
		ERROR_LOG("Fail to start initScaleInfoTransThread Thread\n");
		exit(1);
	}

	pthread_join(gTouchInfoID,NULL);
	pthread_join(gScaleInfoID,NULL);
	close(gSocket);
	ComDestroyQueue (&gQueueTouchInfo);
	return 0;
}
