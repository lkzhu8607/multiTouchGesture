#ifndef __H_TOUCH_H__
#define __H_TOUCH_H__
/*
 * $Id: evtest.c,v 1.23 2005/02/06 13:51:42 vojtech Exp $
 *
 *  Copyright (c) 1999-2000 Vojtech Pavlik
 *
 *  Event device test program
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * 
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

#define MAX_LINE_LEN 256
typedef struct TOUCHINFOFROMFILETAG{
	long s;
	long us;
	int code;
	int value;
}TOUCHINFOFROMFILE;


//触摸屏参考信息
typedef struct DEVICETOUCHEVENTINFOTAG{
	//以下是单点信息
	unsigned short ABS_X_min;//event code 0
	unsigned short ABS_X_max;
	
	unsigned short ABS_Y_min;//event code 1
	unsigned short ABS_Y_max;
	
	unsigned short ABS_PRESSURE_min;//event code 24
	unsigned short ABS_PRESSURE_max;	

	//以下为多点模式信息
	unsigned short ABS_MT_SLOT_min;//event code 47
	unsigned short ABS_MT_SLOT_max;	

	unsigned short ABS_MT_POSITION_X_min;//event code 53
	unsigned short ABS_MT_POSITION_X_max;	

	unsigned short ABS_MT_POSITION_Y_min;//event code 54
	unsigned short ABS_MT_POSITION_Y_max;	

	unsigned short ABS_MT_TRACKING_ID_min;//event code 57
	unsigned short ABS_MT_TRACKING_ID_max;		

	unsigned short ABS_MT_PRESSURE_min;//event code 58
	unsigned short ABS_MT_PRESSURE_max;		
}DEVICETOUCHEVENTINFO;

//多点触摸屏的信息源携带的信息，该结构为单个点的信息
typedef struct TOUCHPOINTTAG{
    long s;
	long us;
	//int X;//code 0
	//int Y;//code 1
	int code_47;//slot
	int code_53;//x
	int code_54;//y
	//int pointNum;
	int code_57;	//tracking ID
	int code_58;
}TOUCHPOINT;

typedef struct COMMONBASEINFOTAG{
	unsigned short min;
	unsigned short max;
}COMMONBASEINFO;

typedef struct SCREENINFOTAG{
	COMMONBASEINFO slot;
	COMMONBASEINFO x;
	COMMONBASEINFO y;
	COMMONBASEINFO trackID;
	COMMONBASEINFO pressure;
}SCREENINFO;

int getTouchInfo(char *deviceFile, int mode);
int getTouchInfoFromLog(char *logFileName);
int formatTouchInfoFromLog(char *data, TOUCHINFOFROMFILE *touchInfo);

extern SCREENINFO gScreenInfo;
#endif
