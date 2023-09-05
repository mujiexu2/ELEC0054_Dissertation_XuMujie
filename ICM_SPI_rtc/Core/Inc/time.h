/***********************************************************************
*
* Filename:     time.h
*
* Description:  This header file provides data structures, macros,
*               and function declarations related to time management,
*               including NMEA time format, Unix timestamp conversion,
*               and operations for RTC (Real-Time Clock). The file
*               also includes utility functions for converting and
*               displaying time in different formats.
*
* Author:       xmj_j
*
* Created on:   Aug 6, 2023
*
* Notes:        Library file to do time related conversions, data types definition,
*
* Rev History:
*       Date            Author      Description
*       ----            ------      -----------
*       2023-08-06      xmj_j      Initial creation
*
***********************************************************************/

#ifndef INC_TIME_H_
#define INC_TIME_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "rtc.h"

//UTC Time message
__packed typedef struct
{
 	uint16_t year;	//year
	uint8_t month;	//month
	uint8_t date;	//date
	uint8_t hour; 	//hour
	uint8_t min; 	//minute
	uint8_t sec; 	//second
}nmea_time;

//Store the data needed to transmit
typedef struct{
    uint32_t unix_timestamp;
    nmea_time utc_time;
    nmea_time uk_time;
    uint32_t elapsed_minutes;
    uint32_t elapsed_seconds;
}time_data;

//Current UK time
typedef struct {
    nmea_time local_time;
} NMEA_Result;

typedef struct t_xtime {
  int year; int month;  int day;
  int hour; int minute;  int second;
} _xtime ;

#define xMINUTE    (60        ) //seconds in one minute
#define xHOUR      (60*xMINUTE) //seconds in 1 hour
#define xDAY        (24*xHOUR   ) //seconds in 1 day
#define xYEAR       (365*xDAY   ) //seconds in 1 year

#define ONEDAYTOSENCOND (24 * 60 * 60)    // day second conversion
#define ONEMINUTETOSENCOND 60             // minute second conversion

extern NMEA_Result NMEA_result;
extern RTC_TimeTypeDef sTime;
extern RTC_DateTypeDef sDate;

void time_request(void);
void CDC_Received_Callback(uint8_t* Buf, uint32_t Len);
void Set_RTC_From_Buffer(uint8_t* buffer);


//void UTC_to_BJtime(nmea_time*	utc_time, int8_t timezone);

unsigned int  xDate2Seconds(_xtime *time);

//UNIX to UTC has been converted to desired time zone
void xSeconds2Date(unsigned long seconds,_xtime *time );
uint32_t ConvertDateToSecond(const uint8_t *date);
time_data read_time(uint32_t startTime);

#endif /* INC_TIME_H_ */
