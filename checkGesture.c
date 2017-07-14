#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include "queue.h"
#include "touch.h"
#include "checkGesture.h"
#include "common.h"
#include "log.h"
#include <math.h>

//static int fingerCount = 0;//�������ϵ�ǰ����ָ���������Ϊ10�����������ֵ���账��

static int frontDragPoint = -1;//for one point dragging
static int currentDragPoint = -1;//for one point dragging

static int currentPoint = -1;//for two points scaling
static int frontPoint = -1;//for two points scaling

static int startFlag = 0;
static int point0Flag = 0;
static int point1Flag = 0;

int existFingerCount = 0;//ǰ�μ�⵽����ָ����
int detectFingerCount = 0;//��ǰ���ڼ�⵽����ָ����

FINGER slotStatus[10] = {{0,0},{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{7,0},{8,0},{9,0}};


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

//���������ֵ
int calCoordinateDeviation(TOUCHPOINT point0, TOUCHPOINT point1, int *dX, int *dY)
{
	if(dX == NULL || dY == NULL)
		return -1;
	if(point0.code_53 <= point1.code_53)
		*dX = point1.code_53 - point0.code_53;
	else
		*dX = point0.code_53 - point1.code_53;
	if(point0.code_54 <= point1.code_54)
		*dY = point1.code_54 - point0.code_54;
	else
		*dY = point0.code_54 - point1.code_54;
	return 0;
}

int calD_value(AXIS value[2])
{
	int deltaX , deltaY;
	deltaX = deltaY = 0;
	if(value[0].x <= value[1].x)
		deltaX = value[1].x - value[0].x;
	else
		deltaX = value[0].x - value[1].x;
	
	if(value[0].y <= value[1].y)
		deltaY = value[1].y - value[0].y;
	else
		deltaY = value[0].y - value[1].y;	

	if((deltaX < DRAGDELTA_X * SHIFTRATIO_X) && (deltaY < DRAGDELTA_Y * SHIFTRATIO_Y)){
		return 0;
	}
	return -1;
}

int calDoublePointVariance(TOUCHPOINT point0, TOUCHPOINT point1)
{
	int variance = 0;
	int absX, absY;
	absX = absY = 0;
	if(point0.code_53 >= point1.code_53)
		absX = point0.code_53 - point1.code_53;
	else
		absX = point1.code_53 - point0.code_53;
	
	if(point0.code_54 >= point1.code_54)
		absY = point0.code_54 - point1.code_54;
	else
		absY = point1.code_54 - point0.code_54;

	variance = SQUARE(absX) + SQUARE(absY);
	
	return variance;
}

/*
* [in] pointInfo
* [in] vertexPoint Ϊһ�����ĸ�Ԫ�ص�����,�±�Ϊ0��Ԫ��Ϊx��������С�ĵ�,�±�Ϊ1��Ԫ��Ϊx���������ĵ�;
*					�±�Ϊ2��Ԫ��Ϊy��������С��Ԫ�صĵ�,�±�Ϊ4��Ԫ��Ϊy���������ĵ�;
* [out] pScalePoint
* @return
*		
*/
int classifyVertexPoint(TOUCHPOINT pointInfo[2][10], int pointCount, VERTEX vertexPoint[4], int vertexCount, TOUCHPOINT (*pScalePoint)[2])
{
	int L_max = 0;
	int i, j, L01, L02, L12;
	int Lx_min_max0 = 0;
	int Ly_min_max0 = 0;
	int Lx_min_max1 = 0;
	int Ly_min_max1 = 0;	
	int deltaX = 0;
	int deltaY = 0;
	int tmpVertexCount = 0;
	TOUCHPOINT triplrPoint[2][3];
	TOUCHPOINT quadrupletPoint[2][4];
	i = j = L01 = L02 = L12 = 0;	
	memset(triplrPoint, 0, sizeof(TOUCHPOINT)*2*3);
	memset(quadrupletPoint, 0, sizeof(TOUCHPOINT)*2*4);
	if(pointCount < vertexCount){
		return -1;
	}
	for(i = 0; i < 4; i++){
		if(tmpVertexCount == vertexCount){
			break;
		}
		if(i == 0){
			tmpVertexCount++;
		}
		else{
			if(vertexPoint[i].slot == vertexPoint[i-1].slot){
				continue;
			}
			else{
				tmpVertexCount++;
			}
		}
		for(j = 0; j < pointCount; j++){
			if(pointInfo[0][j].code_47 == vertexPoint[i].slot){
				if(vertexCount == 2){
					memcpy(&pScalePoint[0][tmpVertexCount-1], &pointInfo[0][j], sizeof(TOUCHPOINT));
					memcpy(&pScalePoint[1][tmpVertexCount-1], &pointInfo[1][j], sizeof(TOUCHPOINT));					
				}
				else if(vertexCount == 3){
					memcpy(&triplrPoint[0][tmpVertexCount-1], &pointInfo[0][j], sizeof(TOUCHPOINT));
					memcpy(&triplrPoint[1][tmpVertexCount-1], &pointInfo[1][j], sizeof(TOUCHPOINT));					
				}
				else if(vertexCount == 4){
					memcpy(&quadrupletPoint[0][tmpVertexCount-1], &pointInfo[0][j], sizeof(TOUCHPOINT));
					memcpy(&quadrupletPoint[1][tmpVertexCount-1], &pointInfo[1][j], sizeof(TOUCHPOINT));					
				}
				break;
			}			
		}
	}
	if(vertexCount == 2)
		return 0;
	else if(vertexCount == 3){//3�������������ҳ���������������Ϊ���ŵĻ�׼��
		L01 = calDoublePointVariance(triplrPoint[0][0], triplrPoint[0][1]);
		L02 = calDoublePointVariance(triplrPoint[0][0], triplrPoint[0][2]);
		L12 = calDoublePointVariance(triplrPoint[0][1], triplrPoint[0][2]);
		L_max = max(max(L01, L02), L12);
		if(L_max == L01){
			memcpy(&pScalePoint[0][0], &triplrPoint[0][0], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[1][0], &triplrPoint[1][0], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[0][1], &triplrPoint[0][1], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[1][1], &triplrPoint[1][1], sizeof(TOUCHPOINT));			
		}
		else if(L_max == L02){
			memcpy(&pScalePoint[0][0], &triplrPoint[0][0], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[1][0], &triplrPoint[1][0], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[0][1], &triplrPoint[0][2], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[1][1], &triplrPoint[1][2], sizeof(TOUCHPOINT));	
		}
		else if(L_max == L12){
			memcpy(&pScalePoint[0][0], &triplrPoint[0][1], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[1][0], &triplrPoint[1][1], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[0][1], &triplrPoint[0][2], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[1][1], &triplrPoint[1][2], sizeof(TOUCHPOINT));	
		}
	}	
	else if(vertexCount == 4){//4 //4������������x��������Сֵ����;��y��������Сֵ����
		Lx_min_max0 = calDoublePointVariance(quadrupletPoint[0][0], quadrupletPoint[0][1]);
		Ly_min_max0 = calDoublePointVariance(quadrupletPoint[0][2], quadrupletPoint[0][3]);
		Lx_min_max1 = calDoublePointVariance(quadrupletPoint[1][0], quadrupletPoint[1][1]);
		Ly_min_max1 = calDoublePointVariance(quadrupletPoint[1][2], quadrupletPoint[1][3]);	
		deltaX = Lx_min_max1 - Lx_min_max0;
		deltaY = Ly_min_max1 - Ly_min_max0;
		if(deltaX > 0 && deltaY == 0){
			memcpy(&pScalePoint[0][0], &quadrupletPoint[0][0], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[1][0], &quadrupletPoint[1][0], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[0][1], &quadrupletPoint[0][1], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[1][1], &quadrupletPoint[1][1], sizeof(TOUCHPOINT));	

		}
		else if(deltaY > 0 && deltaX == 0){
			memcpy(&pScalePoint[0][0], &quadrupletPoint[0][2], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[1][0], &quadrupletPoint[1][2], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[0][1], &quadrupletPoint[0][3], sizeof(TOUCHPOINT));
			memcpy(&pScalePoint[1][1], &quadrupletPoint[1][3], sizeof(TOUCHPOINT));	

		}
		else if(deltaY > 0 && deltaX > 0){
			if(Lx_min_max0 > Ly_min_max0){
				memcpy(&pScalePoint[0][0], &quadrupletPoint[0][0], sizeof(TOUCHPOINT));
				memcpy(&pScalePoint[1][0], &quadrupletPoint[1][0], sizeof(TOUCHPOINT));
				memcpy(&pScalePoint[0][1], &quadrupletPoint[0][1], sizeof(TOUCHPOINT));
				memcpy(&pScalePoint[1][1], &quadrupletPoint[1][1], sizeof(TOUCHPOINT)); 

			}
			else{
				memcpy(&pScalePoint[0][0], &quadrupletPoint[0][2], sizeof(TOUCHPOINT));
				memcpy(&pScalePoint[1][0], &quadrupletPoint[1][2], sizeof(TOUCHPOINT));
				memcpy(&pScalePoint[0][1], &quadrupletPoint[0][3], sizeof(TOUCHPOINT));
				memcpy(&pScalePoint[1][1], &quadrupletPoint[1][3], sizeof(TOUCHPOINT)); 
			}
		}
	}
	return 0;
}

/*
* [in] pointInfo �����Ĳ���������
* [in] pointCount ��ǰ����Ҫ����Ĵ�����ĸ���
* [out] vertexPoint Ϊһ�����ĸ�Ԫ�ص�����,�±�Ϊ0��Ԫ��Ϊx��������С�ĵ�,�±�Ϊ1��Ԫ��Ϊx���������ĵ�;
*					�±�Ϊ2��Ԫ��Ϊy��������С��Ԫ�صĵ�,�±�Ϊ4��Ԫ��Ϊy���������ĵ�;
* @return 
*		validCount ��ǰ���������Ķ���ĸ�����minX,minY,maxX,maxY�����ڼ�������
*/
int findVertexCoordinate(TOUCHPOINT pointInfo[2][10], int pointCount, VERTEX *vertexPoint)
{
	int i, j, minX, maxX, minY, maxY;
	int validCount =4;//���
	i = j = minX = maxX = minY = maxY = 0;	
	for(i = 0; i < pointCount; i++){
		if(i == 0){
			minX = pointInfo[0][i].code_53;
			maxX = pointInfo[0][i].code_53;
			minY = pointInfo[0][i].code_54;
			maxY = pointInfo[0][i].code_54;
			vertexPoint[0].slot = pointInfo[0][i].code_47;
			vertexPoint[1].slot = pointInfo[0][i].code_47;
			vertexPoint[2].slot = pointInfo[0][i].code_47;
			vertexPoint[3].slot = pointInfo[0][i].code_47;
			//vertexPoint[0].value = i;
			
		}
		else{
			if(minX > pointInfo[0][i].code_53){
				minX = pointInfo[0][i].code_53;
				vertexPoint[0].slot = pointInfo[0][i].code_47;
			}
			if(maxX < pointInfo[0][i].code_53){
				maxX = pointInfo[0][i].code_53;
				vertexPoint[1].slot = pointInfo[0][i].code_47;
			}
			if(minY > pointInfo[0][i].code_54){
				minY = pointInfo[0][i].code_54;
				vertexPoint[2].slot = pointInfo[0][i].code_47;
			}
			if(maxY < pointInfo[0][i].code_54){
				maxY = pointInfo[0][i].code_54;
				vertexPoint[3].slot = pointInfo[0][i].code_47;
			}
		}
	}
	
	for(i = 0; i < 4; i++){		
		for(j = (i+1); j < 4; j++){
			if(vertexPoint[i].slot == vertexPoint[j].slot)
				validCount--;
		}
	}
	return validCount;
}

int calScaleAverageCoordinate(TOUCHPOINT pointInfo[2][10], CLASSIFY classify, TOUCHPOINT *classifyPoint)
{
	int x0, x1;
	int y0, y1;
	int p0, p1;
	int calCount = 0;
	int i, j;
	i = j =0;
	x0 = x1 = 0;
	y0 = y1 = 0;
	p0 = p1 = 0;
	if(classify.count == 0)
		return -1;
	for(i = 0; i < 10; i++){//j = 0; j < classify.count; j++
		if(calCount == classify.count){
			break;
		}
		for(j = 0; j < classify.count; j++){
			if(pointInfo[0][i].code_47 == classify.slot[j]){
				x0 += pointInfo[0][i].code_53;
				y0 += pointInfo[0][i].code_54;
				p0 += pointInfo[0][i].code_58;
				
				x1 += pointInfo[1][i].code_53;
				y1 += pointInfo[1][i].code_54;
				p1 += pointInfo[1][i].code_58;		
				calCount += 1;
			}
		}
	}

	x0 /= classify.count;
	y0 /= classify.count;
	p0 /= classify.count;
	
	x1 /= classify.count;
	y1 /= classify.count;
	p1 /= classify.count;	

	classifyPoint[0].code_53 = x0;
	classifyPoint[0].code_54 = y0;
	classifyPoint[0].code_58 = p0;
	
	classifyPoint[1].code_53 = x1;
	classifyPoint[1].code_54 = y1;
	classifyPoint[1].code_58 = p1;	
	return 0;
}

/*
* �ú����������Ƕ�����֪�����Ŷ���һ�鴥������Ϣ���з��࣬��ʹ���ŵĲ�������ȷ
* [in] pointInfo ���Ŷ�����һ���
* [in] pointCount ����������ָ����
* [in] DValue �����仯ǰ�ľ��룬�Լ��仯��ľ���
* [out] pScalePoint �����������ı仯ǰ�ͱ仯��Ĵ�����Ϣ
* @return:
*
*
*/
int classifyDValue(TOUCHPOINT pointInfo[2][10], int valideCount, CLASSIFY *tmpClassify)
{
	AXIS tmpDValue[2];
	AXIS DValue[2][10];
	int i = 0;
	int j = 0;
	int index = 0;
	memset(tmpDValue, 0 ,sizeof(AXIS)*2);
	memset(DValue, 0 ,sizeof(AXIS)*2*10);
	for(i = 0; i < 2; i++){
		for(j = 0; j < valideCount; j++){
			if(j == 0)
				continue;
			/*DValue���㷨Ϊ0-->1, 0-->2, 0-->3, ..., 0-->9,����0λ�±�Ϊ0��pointInfo�Ĵ�����Ϣ�飬1, 2, 3, ..., 9λ�±�Ϊ��Ӧ�Ĵ������ɼ���Ϣ��*/
			calCoordinateDeviation(pointInfo[i][0], pointInfo[i][j], &DValue[i][j-1].x , &DValue[i][j-1].y);
		}
	}
	tmpClassify->slot[index++] = pointInfo[0][0].code_47;
	tmpClassify->count++;
	for(i = 0; i< (valideCount-1); i++){
		memset(tmpDValue, 0 ,sizeof(AXIS)*2);
		memcpy(&tmpDValue[0], &DValue[0][i], sizeof(AXIS));
		memcpy(&tmpDValue[1], &DValue[1][i], sizeof(AXIS));
		if(!calD_value(tmpDValue)){
			tmpClassify->slot[index++] = pointInfo[0][i+1].code_47;
			tmpClassify->count++;
		}
	}

	return 0;
}

/*
* �ú����������Ƕ�����֪�����Ŷ���һ�鴥������Ϣ���з��࣬��ʹ���ŵĲ�������ȷ
* [in] pointInfo ���Ŷ�����һ���
* [in] pointCount ����������ָ����
* [in] DValue �����仯ǰ�ľ��룬�Լ��仯��ľ���
* [out] pScalePoint �����������ı仯ǰ�ͱ仯��Ĵ�����Ϣ
* @return:
*
*
*/
int classifyTouchPoint(TOUCHPOINT pointInfo[2][10], int pointCount, TOUCHPOINT (*pScalePoint)[2])
{
	int i,j,k;
	int index = 0;
	int tmpIndex = 0;
	int totalCount = 0;
	int firstFlag = 1;
	int validCount = 0;
	int existFlag = 0;
	TOUCHPOINT tmpPoint[2][10];
	
	VERTEX vertexPoint[4];
	int vertexCount = 0;
	
	TOUCHPOINT classifyPoint[2];
	
	int slotFlag[10];//��DValue��Ӧ��0��ʾ��Ӧ��DValueû�����࣬1��ʾ��Ӧ��DValue������
	
	CLASSIFY classify[10]; //�Ǹ���ָ���10���
	int curClassifyCount = 0;//��ǰ��ָ����������,��ʼֵΪ 1
	i = j = k = 0;
	if(pointCount < 3 || pointCount > 10)
		return -1;
	memset(slotFlag, -1, sizeof(int)*10);
	memset(tmpPoint, 0, sizeof(TOUCHPOINT)*2*10);
	/*for(j = 0; j < 10; j++){
		
	}*/
	memset(vertexPoint, 0, sizeof(VERTEX)*4);
	memset(classify, 0, sizeof(CLASSIFY)*10);
	while(1){//����
		//index = 0;
		
		if(firstFlag == 1){
			classifyDValue(pointInfo, pointCount, &classify[curClassifyCount]);//���е�ĵ�һ�ι���
			firstFlag = 0;
		}
		else{
			if(validCount == 1){//�����ʣ1����δ��������
				slotFlag[index] = tmpPoint[0][0].code_47;
				classify[curClassifyCount].slot[0] = tmpPoint[0][0].code_47;
				classify[curClassifyCount].count = 1;
				//totalCount += 1;
			}
			else if(validCount > 1)
				classifyDValue(tmpPoint, validCount, &classify[curClassifyCount]);//ʣ�����ٹ���
		}

		//ÿ�ι����ͳ���Ѿ�������ĵ����
		totalCount += classify[curClassifyCount].count;

		//�����ѱ�����ĵ��slotֵ
		for(j = 0;j < classify[curClassifyCount].count; j++){
			slotFlag[index++] = classify[curClassifyCount].slot[j];
		}	

		if(totalCount == pointCount){
			curClassifyCount++;
			break;
		}
		validCount = pointCount - totalCount;//�����ʣ��ķѷ���ĵ����

		//׼��δ����ĵ���Ϊ�´��ٷ���Ĳ���
		memset(tmpPoint, 0, sizeof(TOUCHPOINT)*2*10);
		tmpIndex = 0;
		for(j = 0; j < 10; j++){
			
			for(k = 0; k < totalCount; k++){//�����Ƿ��Ѿ������࣬����Ѿ��������򲻻��ٴν��й��࣬���򽫽��й������
				if(pointInfo[0][j].code_47 == slotFlag[k]){
					existFlag = 1;
				}
				else{
					continue;
				}
			}
			if(existFlag == 1){
				existFlag = 0;
				continue;
			}
			else{
				memcpy(&tmpPoint[0][tmpIndex], &pointInfo[0][j], sizeof(TOUCHPOINT));
				memcpy(&tmpPoint[1][tmpIndex], &pointInfo[1][j], sizeof(TOUCHPOINT));
				tmpIndex++;
				existFlag = 0;
			}			
		}	
		curClassifyCount++;
	}

	if(curClassifyCount == 2){
		for(i = 0; i < curClassifyCount; i++){
			memset(classifyPoint, 0, sizeof(TOUCHPOINT)*2);
			if(classify[i].count == 1){
				for(j = 0; j < 10; j++){
					if(pointInfo[0][j].code_47 == classify[i].slot[0]){
						memcpy(&pScalePoint[0][i], &pointInfo[0][j], sizeof(TOUCHPOINT));
						memcpy(&pScalePoint[1][i], &pointInfo[1][j], sizeof(TOUCHPOINT));
						break;
					}
				}				
			}
			if(!calScaleAverageCoordinate(pointInfo, classify[i], classifyPoint)){
				memcpy(&pScalePoint[0][i], &classifyPoint[0], sizeof(TOUCHPOINT));
				memcpy(&pScalePoint[1][i], &classifyPoint[1], sizeof(TOUCHPOINT));
			}
		}
	}
	else if(curClassifyCount > 2){
		vertexCount = findVertexCoordinate(pointInfo, pointCount, vertexPoint);
		classifyVertexPoint(pointInfo, pointCount, vertexPoint, vertexCount, pScalePoint);
	}
	return 0;
}


/*�����ָ��ק����ʼ���ƽ��ֵ����*/
int calDragAverageCoordinate(TOUCHPOINT pointInfo[2][10], int pointCount, AXIS *pDragAxis)
{
	//int i = 0;
	int j = 0;
	if(pointCount == 0)
		return -1;
	for(j = 0; j < pointCount; j++){
		if(pointInfo[0][j].code_0 > 0 || pointInfo[0][j].code_1 > 0 ){
			pDragAxis[0].x = pointInfo[0][j].code_0;
			pDragAxis[0].y = pointInfo[0][j].code_1;
			pDragAxis[1].x = pointInfo[1][j].code_0;
			pDragAxis[1].y = pointInfo[1][j].code_1;
			break;
		}
	}
	
	/*for(i = 0; i < 2; i++){
		for(j = 0; j < pointCount; j++){
			pDragAxis[i].x +=  pointInfo[i][j].code_53;
			pDragAxis[i].y +=  pointInfo[i][j].code_54;
		}
		pDragAxis[i].x /= pointCount;
		pDragAxis[i].y /= pointCount;
	}*/
	return 0;
}

/*
* [in] pointInfo, ÿ��Ҫʶ�����ƵĲ�������Ϣ����
* [in] pointCount, ����Ҫ�����Ĳ��������
* [out] pGestureFlag, ʶ������������:1,��ʾ�϶�; 2,��ʾ����
* [out] pDragAxis, ��ק����ʱ��ʼ��ͽ�������Ϣ���������
* [out] pScalePoint, ��������ʱ����������
*
* @return:
*	0:�����ɹ�
*	-1:����,Ҫ�����Ĳ����������Ϸ����Ϸ�����ָ����Ϊ3~10, 1,2����������
*
*/
int multiPointCoordinateDeviation(TOUCHPOINT pointInfo[2][10], int pointCount, int *pGestureFlag, AXIS *pDragAxis, TOUCHPOINT (*pScalePoint)[2])
{
	AXIS DValue[2][10];
	AXIS tmpDValue[2];
	int i = 0;
	int j = 0;
	int scaleFlag = 0;//0,��ʾ���Ŷ���δ����; 1,��ʾ���Ŷ�������
	memset(DValue, 0 ,sizeof(AXIS)*2*10);
	
	if(pointCount < 3 || pointCount > 10)
		return -1;
	for(i = 0; i < 2; i++){
		for(j = 0; j < pointCount; j++){
			if(j == 0)
				continue;
			/*DValue���㷨Ϊ0-->1, 0-->2, 0-->3, ..., 0-->9,����0λ�±�Ϊ0��pointInfo�Ĵ�����Ϣ�飬1, 2, 3, ..., 9λ�±�Ϊ��Ӧ�Ĵ������ɼ���Ϣ��*/
			calCoordinateDeviation(pointInfo[i][0], pointInfo[i][j], &DValue[i][j-1].x , &DValue[i][j-1].y);
		}
	}

	for(i = 0; i< (pointCount-1); i++){
		memset(tmpDValue, 0 ,sizeof(AXIS)*2);
		memcpy(&tmpDValue[0], &DValue[0][i], sizeof(AXIS));
		memcpy(&tmpDValue[1], &DValue[1][i], sizeof(AXIS));
		if(calD_value(tmpDValue) == 0)
			*pGestureFlag = 1;    //drag
		else
			scaleFlag = 1;    //scale
	}

	if(scaleFlag == 1){
		*pGestureFlag = 2; //scale
	}
	
	if(*pGestureFlag == 1){//drag
		calDragAverageCoordinate(pointInfo, pointCount, pDragAxis);
	}
	else if(*pGestureFlag == 2){//scaling
		classifyTouchPoint(pointInfo, pointCount, pScalePoint);
	}
	return 0;
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

int updateSampleInfo(TOUCHPOINT srcPoint, TOUCHPOINT *targetPoint)
{
	if(targetPoint == NULL)
		return -1;
	if(srcPoint.code_0 >= 0)
		targetPoint->code_0 = srcPoint.code_0;
	if(srcPoint.code_1 >= 0)
		targetPoint->code_1 = srcPoint.code_1;
	if(srcPoint.code_47 >= 0)
		targetPoint->code_47 = srcPoint.code_47;
	if(srcPoint.code_53 >= 0)
		targetPoint->code_53 = srcPoint.code_53;
	if(srcPoint.code_54 >= 0)
		targetPoint->code_54 = srcPoint.code_54;
	if(srcPoint.code_57 >= 0)
		targetPoint->code_57 = srcPoint.code_57;
	if(srcPoint.code_58 >= 0)
		targetPoint->code_58 = srcPoint.code_58;
	return 0;	
}

int complementSampleInfo(TOUCHPOINT srcPoint, TOUCHPOINT *targetPoint)
{
	if(targetPoint == NULL)
		return -1;
	if(targetPoint->code_47 < 0)
		targetPoint->code_47 = srcPoint.code_47;
	if(targetPoint->code_53 < 0)
		targetPoint->code_53 = srcPoint.code_53;
	if(targetPoint->code_54 < 0)
		targetPoint->code_54 = srcPoint.code_54;
	if(targetPoint->code_57 < 0)
		targetPoint->code_57 = srcPoint.code_57;
	if(targetPoint->code_58 < 0)
		targetPoint->code_58 = srcPoint.code_58;
	if(targetPoint->code_0 < 0)
		targetPoint->code_0 = srcPoint.code_0;
	if(targetPoint->code_1 < 0)
		targetPoint->code_1 = srcPoint.code_1;	
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

/*
return value:
	-1:����
	0:�����ɹ�������δ���Ӻ�ɾ���������ϵ���ָ����
	1:��ָ������һ
	2:��ָ������һ
*/
int slotCheck(int slot, int touchedFlag)
{
	int i = 0;
	if(slot < 0 || slot > 9)
		return -1;
	for(i = 0; i< 10; i++){
		if(slotStatus[i].fingerNum == slot){
			if(touchedFlag == 1){
				if(slotStatus[i].touchedFlag == 0){
					slotStatus[i].touchedFlag = touchedFlag;
					return 1;
				}
			}
			else if(touchedFlag == 0){
				if(slotStatus[i].touchedFlag == 1){
					slotStatus[i].touchedFlag = touchedFlag;
					return 2;
				}
			}		
			break;
			//slotStatus[i].touchedFlag = touchedFlag;
			//return 0;
		}
	}
	return 0;
}

int samplePointInfo(TOUCHPOINT pointIn, TOUCHPOINT *pointOut)
{
	static int singleFlag = 0;
	if(pointOut == NULL)
		return -1;
	if(frontPoint == -1){
		if(pointIn.code_57 == -1){
			if(existFingerCount > 0)
				existFingerCount--;				
			currentPoint = -1;
			frontPoint = currentPoint;
			pointOut->code_47 = pointIn.code_47;
			pointOut->code_53 = pointIn.code_53;
			pointOut->code_54 = pointIn.code_54;
			pointOut->code_57 = pointIn.code_57;
			pointOut->code_58 = pointIn.code_58;	
			pointOut->code_0 = pointIn.code_0;
			pointOut->code_1 = pointIn.code_1;
			return 0;
		}
			if(pointIn.code_47 >= 0){
				if(slotCheck(pointIn.code_47, 1) == 1){
					if(singleFlag == 1){
						existFingerCount -= 1;
						singleFlag = 0;
					}
					existFingerCount++;
				}
				currentPoint = pointIn.code_47;
			}
			else{
				existFingerCount = 1;
				singleFlag = 1;
				currentPoint = -1;
			}
		pointOut->code_47 = pointIn.code_47;
		pointOut->code_53 = pointIn.code_53;
		pointOut->code_54 = pointIn.code_54;
		pointOut->code_57 = pointIn.code_57;
		pointOut->code_58 = pointIn.code_58;
		pointOut->code_0 = pointIn.code_0;
		pointOut->code_1 = pointIn.code_1;
	}
	else{
		if(pointIn.code_57 == -1){
			if(slotCheck(frontPoint, 0) == 2){
				if(existFingerCount > 0)
					existFingerCount--;				
			}	
			currentPoint = -1;
			pointOut->code_47 = frontPoint;
			pointOut->code_57 = -1;
			frontPoint = currentPoint;
			//return -1;
		}
		else{
			if(pointIn.code_47 >= 0){
				if(slotCheck(pointIn.code_47, 1) == 1){
					existFingerCount++;
				}
				currentPoint = pointIn.code_47;				
			}
			else{
				 currentPoint = frontPoint;
			}
			if(pointIn.code_53 > 0 || pointIn.code_54 > 0){
				//pointOut->code_47 = pointIn.code_47;//point->code_47 = frontDragPoint;	
				pointOut->code_47 = currentPoint;
				pointOut->code_53 = pointIn.code_53;
				pointOut->code_54 = pointIn.code_54;
				pointOut->code_57 = pointIn.code_57;
				pointOut->code_58 = pointIn.code_58;
				pointOut->code_0 = pointIn.code_0;
				pointOut->code_1 = pointIn.code_1;
			}			
		}
	}
	frontPoint = currentPoint;
	return 0;
}
