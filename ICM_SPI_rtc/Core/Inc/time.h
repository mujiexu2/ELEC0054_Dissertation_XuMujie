/*
 * time.h
 *
 *  Created on: Aug 6, 2023
 *      Author: xmj_j
 */

#ifndef INC_TIME_H_
#define INC_TIME_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "rtc.h"

//UTC Time message
__packed typedef struct
{
 	uint16_t year;	//年份
	uint8_t month;	//月份
	uint8_t date;	//日期
	uint8_t hour; 	//小时
	uint8_t min; 	//分钟
	uint8_t sec; 	//秒钟
}nmea_time;

typedef struct{
    uint32_t unix_timestamp;
    nmea_time utc_time;
    nmea_time uk_time;
    uint32_t elapsed_minutes;
    uint32_t elapsed_seconds;
}time_data;

typedef struct {
    nmea_time local_time;
} NMEA_Result;

typedef struct t_xtime {
  int year; int month;  int day;
  int hour; int minute;  int second;
} _xtime ;

#define xMINUTE    (60        ) //Seconds to 1 minute
#define xHOUR      (60*xMINUTE) //1小时的秒数
#define xDAY        (24*xHOUR   ) //1天的秒数
#define xYEAR       (365*xDAY   ) //1年的秒数

#define ONEDAYTOSENCOND (24 * 60 * 60)    // seconds to one day
#define ONEMINUTETOSENCOND 60             // seconds to one minute

extern NMEA_Result NMEA_result;
extern RTC_TimeTypeDef sTime;
extern RTC_DateTypeDef sDate;

void time_request(void);
void CDC_Received_Callback(uint8_t* Buf, uint32_t Len);
void Set_RTC_From_Buffer(uint8_t* buffer);


//void UTC_to_BJtime(nmea_time*	utc_time, int8_t timezone);

unsigned int  xDate2Seconds(_xtime *time);

//UNIX转为UTC 已进行时区转换 北京时间UTC+8
void xSeconds2Date(unsigned long seconds,_xtime *time );
uint32_t ConvertDateToSecond(const uint8_t *date);
time_data read_time(uint32_t startTime);

#endif /* INC_TIME_H_ */
