#include <pthread.h>
#include <stdio.h>
#include "queue.h"
#include "touch.h"
#include "checkGesture.h"
#include "common.h"
#include "log.h"
#include <math.h>

static int fingerCount = 0;//�������ϵ�ǰ����ָ���������Ϊ10�����������ֵ���账��

static int frontDragPoint = -1;//for one point dragging
static int currentDragPoint = -1;//for one point dragging

static int currentPoint = -1;//for two points scaling
static int frontPoint = -1;//for two points scaling

static int startFlag = 0;
static int point0Flag = 0;
static int point1Flag = 0;


//������ק
int calTwoPointDrag(TOUCHPOINT groupPoint0[2], TOUCHPOINT groupPoint1[2], TWOPOINTDRAG * doubleFingerCenter)
{
	int variance0;
	int variance1;
	double distance = 0;
	double base0Len = 0.0;
	double base1Len = 0.0;	
	COORDINATEINFO coordinateInfo;
	memset(&coordinateInfo, 0, sizeof(COORDINATEINFO));
	variance0 = variance1 = 0;
	if(doubleFingerCenter == NULL)
		return -1;
	variance0 = calCenterCoordinate(groupPoint0[0], groupPoint0[1], &coordinateInfo);
	variance1 = calCenterCoordinate(groupPoint1[0], groupPoint1[1], &coordinateInfo);
	base0Len = sqrt(variance0);
	base1Len = sqrt(variance1);
	if(base0Len >= base1Len){
		//
		distance = base0Len - base1Len;
	}
	else{
		distance = base1Len - base0Len;
	}
	if(distance < DRAGSUITDISTANCE){
		doubleFingerCenter->startX = (groupPoint0[0].code_53 + groupPoint0[1].code_53)/2;
		doubleFingerCenter->startY = (groupPoint0[0].code_54 + groupPoint0[1].code_54)/2;
		doubleFingerCenter->endX = (groupPoint1[0].code_53 + groupPoint1[1].code_53)/2;
		doubleFingerCenter->endY = (groupPoint1[0].code_54 + groupPoint1[1].code_54)/2;		
		return 0;
	}
	return -1;
}


//����������Ϣ�����ط���ô��Ǽ������
int calCenterCoordinate(TOUCHPOINT touchPoint0, TOUCHPOINT touchPoint1, COORDINATEINFO *coordinateInfo)
{
	//int abs_x_axis;
	//int abs_y_axis;
	coordinateInfo->center_x = (touchPoint0.code_53 + touchPoint1.code_53)/2;
	coordinateInfo->center_y = (touchPoint0.code_54 + touchPoint1.code_54)/2;
	
	if(touchPoint0.code_53 > touchPoint1.code_53)
		coordinateInfo->abs_x_axis = touchPoint0.code_53 - touchPoint1.code_53;
	else
		coordinateInfo->abs_x_axis = touchPoint1.code_53 - touchPoint0.code_53;
	if(touchPoint0.code_54 > touchPoint1.code_54)
		coordinateInfo->abs_y_axis = touchPoint0.code_54 - touchPoint1.code_54;
	else
		coordinateInfo->abs_y_axis = touchPoint1.code_54 - touchPoint0.code_54;
	coordinateInfo->variance = SQUARE(coordinateInfo->abs_y_axis) + SQUARE(coordinateInfo->abs_x_axis);
	
	//�ȱ����Ծ����������а���
	/*if(touchPoint0.code_53 > coordinateInfo->abs_x_axis)
		coordinateInfo->abs_p0_to_C_x_axis = touchPoint0.code_53 - coordinateInfo->abs_x_axis;
	else
		coordinateInfo->abs_p0_to_C_x_axis = coordinateInfo->abs_x_axis - touchPoint0->code_53;
	if(touchPoint0.code_54 > coordinateInfo->abs_y_axis)
		coordinateInfo->abs_p0_to_C_y_axis = touchPoint0.code_54 - coordinateInfo->abs_y_axis;
	else
		coordinateInfo->abs_p0_to_C_y_axis = coordinateInfo->abs_y_axis - touchPoint0->code_54;	
	coordinateInfo->p0_to_C_variance = (SQUARE(coordinateInfo->abs_p0_to_C_x_axis) + SQUARE(coordinateInfo->abs_p0_to_C_y_axis));
	
	
	if(touchPoint1.code_53 > coordinateInfo->abs_x_axis)
		coordinateInfo->abs_p1_to_C_x_axis = touchPoint1.code_53 - coordinateInfo->abs_x_axis;
	else
		coordinateInfo->abs_p1_to_C_x_axis = coordinateInfo->abs_x_axis - touchPoint1->code_53;
	if(touchPoint1.code_54 > coordinateInfo->abs_y_axis)
		coordinateInfo->abs_p1_to_C_y_axis = touchPoint1.code_54 - coordinateInfo->abs_y_axis;
	else
		coordinateInfo->abs_p1_to_C_y_axis = coordinateInfo->abs_y_axis - touchPoint1->code_54;	
	coordinateInfo->p1_to_C_variance = (SQUARE(coordinateInfo->abs_p1_to_C_x_axis) + SQUARE(coordinateInfo->abs_p1_to_C_y_axis));*/	
	return coordinateInfo->variance;
}


/*
0:��ʾ��������ק����ֵ
-1:��ʾ��ק������Ч
*/
int calSinglePointMoveDistance(TOUCHPOINT point0, TOUCHPOINT point1)
{
	int x,y;
	int tmpX,tmpY;
	int delta = 0;//double delta = 0;
	x = y = 0;
	tmpX = tmpY = 0;
	if((point0.code_53 < 0 && point0.code_54 < 0)){
		return -1;
	}
	if(point1.code_53 > 0){
		if((point0.code_53 >= point1.code_53)){
			x = point0.code_53 - point1.code_53;
		}
		else
			x = point1.code_53 - point0.code_53;
		
		if(point1.code_54 > 0){
			if(point0.code_54 >= point1.code_54)
				y = point0.code_54 - point1.code_54;
			else
				y = point1.code_54 - point0.code_54;			
		}
		else{
			if(point0.code_53 > point1.code_53)
				delta = point0.code_53 - point1.code_53;
			else
				delta = point1.code_53 - point0.code_53;	
		}
	}
	else{//x������δ��
		if(point1.code_54 > 0){
			if((point0.code_54 >= point1.code_54))
				delta = point0.code_54 - point1.code_54;
			else
				delta = point1.code_54 - point0.code_54;			
		}
		else
			return -1;
	}

	if(x > 0 && y > 0){
		if(gSetting.mode == 1){
			tmpX = x * SCREENWIDTH / gScreenInfo.x.max;
			tmpY = y * SCREENHEIGTH / gScreenInfo.y.max;
		}
		else if(gSetting.mode == 2){
			tmpX = x * SCREENWIDTH / 5801;
			tmpY = y * SCREENHEIGTH / 4095;			
		}
		delta = sqrt(SQUARE(tmpX) + SQUARE(tmpY));
	}
	
	if(delta > DRAGTRIGGERDISTANCE && delta < DRAGMAXDISTANCE)
		return 0;
	return -1;
}

//������ק��Ϣ�ɼ�
/*
0:�ɹ�
-1:ʧ��,��Ч��Ϣ
-2:��ʾǰһ������Ϣ��ʧ�ˣ��ҳ�����һ���µ�
-3:��ʾǰһ������ʧ
*/
int sampleBaseDragsinglePointInfo(TOUCHPOINT touchInfo, TOUCHPOINT *point){
	//if(frontDragPoint == -1 && touchInfo.code_47 < 0)
		//return -1;
	if(frontDragPoint == -1)
	{
		if(touchInfo.code_57 == -1)
			return -3;
		if(touchInfo.code_47 >= 0){
			currentDragPoint = touchInfo.code_47;
			point->code_47 = touchInfo.code_47;
			point->code_53 = touchInfo.code_53;
			point->code_54 = touchInfo.code_54;
			point->code_57 = touchInfo.code_57;
		}
		else{
			currentDragPoint = -1;
		}
		point->code_47 = touchInfo.code_47;
		point->code_53 = touchInfo.code_53;
		point->code_54 = touchInfo.code_54;
		point->code_57 = touchInfo.code_57;		
	}
	else
	{
		if(touchInfo.code_57 == -1){
			currentDragPoint = -1;
			frontDragPoint = currentDragPoint;
			return -3;
		}
		if(touchInfo.code_47 >= 0)
		{
				currentDragPoint = touchInfo.code_47;
				point->code_47 = touchInfo.code_47;
				point->code_53 = touchInfo.code_53;
				point->code_54 = touchInfo.code_54;
				point->code_57 = touchInfo.code_57;		
			if(touchInfo.code_47 != frontDragPoint){//ǰһ�����뵱ǰ�㲻��ͬһ������Դ
				frontDragPoint = currentDragPoint;
				return -2;
			}
		}
		//if(touchInfo.code_53 != 0 || touchInfo.code_54 != 0){
		if(touchInfo.code_53 > 0 || touchInfo.code_54 > 0){
			point->code_47 = touchInfo.code_47;//point->code_47 = frontDragPoint;			
			point->code_53 = touchInfo.code_53;
			point->code_54 = touchInfo.code_54;
			point->code_57 = touchInfo.code_57;
		}
	}
	frontDragPoint = currentDragPoint;
	return 0;
}


//�������ŵڶ������Ϣ�������
int polishingSampleInfo(TOUCHPOINT base0[2], TOUCHPOINT *base1, TOUCHPOINT *base2)
{
	if(base1->code_47 < 0)
		base1->code_47 = base0[0].code_47;
	if(base1->code_53 < 0)
		base1->code_53 = base0[0].code_53;
	if(base1->code_54 < 0)
		base1->code_54 = base0[0].code_54;

	if(base2->code_47 < 0)
		base2->code_47 = base0[1].code_47;
	if(base2->code_53 < 0)
		base2->code_53 = base0[1].code_53;
	if(base2->code_54 < 0)
		base2->code_54 = base0[1].code_54;	
	return 0;
}

//���㴥��ȷ������
/*
0:�ɹ�
-1:ʧ��
-2:��ʾ��һ������ʧ
-3:��ʾ�ڶ�������ʧ
*/
int sampleBasePointInfo(TOUCHPOINT touchInfo, TOUCHPOINT *point0, TOUCHPOINT *point1)
{
	if(frontPoint == -1 && touchInfo.code_47 < 0)
		return -1;
	
	if(frontPoint == -1)
	{
		if(touchInfo.code_47 == 0)
		{
			//currentPoint = 0;
			if(touchInfo.code_57 == -1)
			{
				point0Flag = 0;
				//INFO_LOG("The first point info disappear\n");
				return -2;
			}
			else
			{
				point0->code_47 = touchInfo.code_47;
				if(touchInfo.code_53 != -1)
					point0->code_53 = touchInfo.code_53;
				if(touchInfo.code_54 != -1)
					point0->code_54 = touchInfo.code_54;
				point0->code_57 = touchInfo.code_57;
				point0Flag = 1;
			}

		}
		else if(touchInfo.code_47 == 1)
		{
			//currentPoint = 1;
			if(touchInfo.code_57 == -1)
			{
				point1Flag = 0;
				//INFO_LOG("The second point info disappear\n");
				return -3;
			}
			else
			{
				point1->code_47 = touchInfo.code_47;
				if(touchInfo.code_53 > 0)
					point1->code_53 = touchInfo.code_53;
				if(touchInfo.code_54 > 0)
					point1->code_54 = touchInfo.code_54;
				point1->code_57 = touchInfo.code_57;
				point1Flag = 1;
			}		
		}	
		else{
			return -1;
		}
		currentPoint = touchInfo.code_47;		
	}
	else 
	{
		if(touchInfo.code_47 == 0)
		{
			//currentPoint = 0;
			if(touchInfo.code_57 == -1)
			{
				currentPoint = -1;
				point0Flag = 0;	
				//INFO_LOG("The first point info disappear\n");
				return -2;
			}
			else
				{
				currentPoint = 0;
				point0->code_47 = touchInfo.code_47;
				if(touchInfo.code_53  > 0)
					point0->code_53 = touchInfo.code_53;
				if(touchInfo.code_54  > 0)
					point0->code_54 = touchInfo.code_54;
				point0->code_57 = touchInfo.code_57;
				point0Flag = 1;
			}	
		}
		else if(touchInfo.code_47 == 1)
		{
			//currentPoint = 1;
			if(touchInfo.code_57 == -1)
			{
				currentPoint = -1;
				point1Flag = 0;	
				//INFO_LOG("The second point info disappear\n");
				return -3;
			}		
			else
			{
				currentPoint = 1;
				point1->code_47 = touchInfo.code_47;
				if(touchInfo.code_53  > 0)
					point1->code_53 = touchInfo.code_53;
				if(touchInfo.code_54  > 0)
					point1->code_54 = touchInfo.code_54;
				point1->code_57 = touchInfo.code_57;
				point1Flag = 1; 
			}
		}
		else
		{
			if(frontPoint == 0)
			{
				//currentPoint = 0;
				if(touchInfo.code_57 == -1)
				{
					currentPoint = -1;
					point0Flag = 0;
					//INFO_LOG("The first point info disappear\n");
					return -2;
				}
				else{
					currentPoint = 0;
					if(touchInfo.code_53  > 0)
						point0->code_53 = touchInfo.code_53;
					if(touchInfo.code_54  > 0)
						point0->code_54 = touchInfo.code_54;
					point0->code_57 = touchInfo.code_57;
					point0Flag = 1;
				}			
			}
			else if(frontPoint == 1)
			{
				//currentPoint = 1;
				if(touchInfo.code_57 == -1)
				{
					currentPoint = -1;
					point1Flag = 0;
					//INFO_LOG("The scond point info disappear\n");
					return -3;
				}
				else
				{
					currentPoint = 1;
					if(touchInfo.code_53  > 0)
						point1->code_53 = touchInfo.code_53;
					if(touchInfo.code_54  > 0)
						point1->code_54 = touchInfo.code_54;
					point1->code_57 = touchInfo.code_57;
					point1Flag = 1;
				}
			}			
		}		
	}	
	//if
	frontPoint = currentPoint;
	if(point0Flag == 1 && point1Flag == 1)
	{
		point0Flag = 0;
		point1Flag = 0;
		return 0;
	}
	return -1;
}

//���㴥�������ű�������
int calScaling(int base0variance, int base1variance)
{
	double scale;
	double base0Len = sqrt(base0variance);
	double base1Len = sqrt(base1variance);
	
	if(base0variance == 0)
		return -1;
	if(startFlag == 0)
	{
		if((base1Len - base0Len) >= 0)
		{
			if((base1Len - base0Len) < STATICDELTA)
				return -1;
		}
		else
		{
			if((base0Len - base1Len) < STATICDELTA)
				return -1;			
		}
		startFlag = 1;
	}
	else
	{
		if((base1Len - base0Len) >= 0)
		{
			if((base1Len - base0Len) < STATICDELTA)
				return -1;
		}
		else
		{
			if((base0Len - base1Len) < STATICDELTA)
				return -1;			
		}

		//startFlag = 1;			
	}
	scale = (base1Len - base0Len)/base0Len * 10000;
	return (10000+scale);
}

int freshValue(TOUCHPOINT pointIn, TOUCHPOINT *pointOut)
{
	if(pointOut == NULL)
		return -1;
	if(pointOut->code_47 < 0)
		pointOut->code_47 = pointIn.code_47;
	if(pointOut->code_53 < 0)
		pointOut->code_53 = pointIn.code_53;
	if(pointOut->code_54 < 0)
		pointOut->code_54 = pointIn.code_54;	
	return 0;
}

//������ָ������slotֵ
void setSlotValue(int front, int current)
{
	frontPoint = front;
	currentPoint = current;
}
