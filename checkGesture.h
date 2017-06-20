#ifndef __H_CHECKGESTURE_H__
#define __H_CHECKGESTURE_H__

//该结构体主要针对两点操作的坐标信息
typedef struct COORDINATEINFOTAG{
	int center_x;
	int center_y;	
	int abs_x_axis;
	int abs_y_axis;
	int abs_p0_to_C_x_axis;
	int abs_p0_to_C_y_axis;
	int abs_p1_to_C_x_axis;
	int abs_p1_to_C_y_axis;	
	int variance;//方差
	int p0_to_C_variance;
	int p1_to_C_variance;
}COORDINATEINFO;

//与当前显示系统通信的结构体
typedef struct TRANSINFOTAG{
	int scale;//缩放参考值
	int x;//缩放中心点的x坐标
	int y;//缩放中心点的y坐标
	int dragStart_x;
	int dragStart_y;
	int dragEnd_x;
	int dragEnd_y;
}TRANSINFO;

typedef struct TWOPOINTDRAGTAG{
	int startX;
	int startY;
	int endX;
	int endY;
}TWOPOINTDRAG;

typedef struct AXISTAG{
	int x;//单点的横坐标
	int y;//单点的纵坐标
}AXIS;

#define DRAGSUITDISTANCE 0 //可以当做两点拖拽的操作的，前后两次距离差的合适最大差值

#define STATICDELTA 5 //静态时到动态缩放的坐标移动触发缩放的阈值
#define DYNAMICDELTA 0 //动态时的坐标移动触发缩放的阈值

#define DRAGTRIGGERDISTANCE 1 //拖拽的触发长度
#define DRAGMAXDISTANCE 60 //单次拖拽最大的移动距离

//threshold;
#define DOUBLE_SCALE_THRESHOLD_DISTANCE 90 //触发两指缩放发生的两指间初始间距

int calCenterCoordinate(TOUCHPOINT touchPoint0, TOUCHPOINT touchPoint1, COORDINATEINFO *coordinateInfo);
int sampleBasePointInfo(TOUCHPOINT touchInfo, TOUCHPOINT *point0, TOUCHPOINT *point1);
int calScaling(int base0variance, int base1variance);
void setSlotValue(int front, int current);
int calSinglePointMoveDistance(TOUCHPOINT point0, TOUCHPOINT point1);
int sampleBaseDragsinglePointInfo(TOUCHPOINT touchInfo, TOUCHPOINT *point);
int polishingSampleInfo(TOUCHPOINT base0[2], TOUCHPOINT *base1, TOUCHPOINT *base2);
int freshValue(TOUCHPOINT pointIn, TOUCHPOINT *pointOut);
int calTwoPointDrag(TOUCHPOINT groupPoint0[2], TOUCHPOINT groupPoint1[2], TWOPOINTDRAG * doubleFingerCenter);
#endif