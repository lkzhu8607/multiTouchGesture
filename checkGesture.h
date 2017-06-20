#ifndef __H_CHECKGESTURE_H__
#define __H_CHECKGESTURE_H__

//�ýṹ����Ҫ������������������Ϣ
typedef struct COORDINATEINFOTAG{
	int center_x;
	int center_y;	
	int abs_x_axis;
	int abs_y_axis;
	int abs_p0_to_C_x_axis;
	int abs_p0_to_C_y_axis;
	int abs_p1_to_C_x_axis;
	int abs_p1_to_C_y_axis;	
	int variance;//����
	int p0_to_C_variance;
	int p1_to_C_variance;
}COORDINATEINFO;

//�뵱ǰ��ʾϵͳͨ�ŵĽṹ��
typedef struct TRANSINFOTAG{
	int scale;//���Ųο�ֵ
	int x;//�������ĵ��x����
	int y;//�������ĵ��y����
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
	int x;//����ĺ�����
	int y;//�����������
}AXIS;

#define DRAGSUITDISTANCE 0 //���Ե���������ק�Ĳ����ģ�ǰ�����ξ����ĺ�������ֵ

#define STATICDELTA 5 //��̬ʱ����̬���ŵ������ƶ��������ŵ���ֵ
#define DYNAMICDELTA 0 //��̬ʱ�������ƶ��������ŵ���ֵ

#define DRAGTRIGGERDISTANCE 1 //��ק�Ĵ�������
#define DRAGMAXDISTANCE 60 //������ק�����ƶ�����

//threshold;
#define DOUBLE_SCALE_THRESHOLD_DISTANCE 90 //������ָ���ŷ�������ָ���ʼ���

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