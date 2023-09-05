/**
 * @file    time.c
 * @brief   This file contains functions for managing and converting time data.
 *
 * It includes the following functionalities:
 * 1. Sending a time request to a PC via USB VCP. (for old plan: current data get from PC through USB VCP)
 * 2. Receiving time data via USB CDC and updating the STM32's RTC. (for old plan: current data get from PC through USB VCP)
 * 3. Converting UTC time to Beijing time (commented out).
 * 4. Checking for leap years and calculating the number of days in a month.
 * 5. Determining whether daylight saving time adjustments should be applied.
 * 6. Converting UTC time to UK time considering DST.
 * 7. Converting time to seconds since the UNIX epoch (1970).
 * 8. Converting seconds since the UNIX epoch to a date-time structure.
 * 9. Convert GPS Date to seconds.
 *
 * @author  [Author's Name]
 * @date    [Date]
 * @version 1.0
 */
#include "time.h"
#include "rtc.h"

//send timing request to PC from USB VCP
void time_request(void) {
    uint8_t request[] = "Get Time from VCP!!\r\n";
    CDC_Transmit_FS(request, sizeof(request) - 1); // NOTE: Use sizeof(request) - 1 because we don't want to send the terminating character '\0'
	HAL_Delay(1000);
}

//STM32 receive time data
uint8_t rx_buffer[64];  // declare a receiver buffer

// a callback function, when data passes USB CDC will call this function
void CDC_Received_Callback(uint8_t* Buf, uint32_t Len) {
    memcpy(rx_buffer, Buf, Len);  // copy received data to rx_buffer copy received data to rx_buffer
    Set_RTC_From_Buffer(rx_buffer);
}

void Set_RTC_From_Buffer(uint8_t* buffer) {
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    // Assume that the format sent by your PC program is: "YYYY-MM-DD HH:MM:SS"
    sscanf((char*)buffer, "%04d-%02d-%02d %02d:%02d:%02d", &sDate.Year, &sDate.Month, &sDate.Date, &sTime.Hours, &sTime.Minutes, &sTime.Seconds);

    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

//
////UTC time convert to BeiJing time
//void UTC_to_BJtime(nmea_time* utc_time, int8_t timezone)
//{
//	int year,month,day,hour;
//    int lastday = 0;					//last day of this month
//    int lastlastday = 0;			//last day of last month
//
//    year	 = utc_time->year;			 //utc time
//    month  = utc_time->month;
//    day 	 = utc_time->date;
//    hour 	 = utc_time->hour + timezone;
//
//    if(month==1 || month==3 || month==5 || month==7 || month==8 || month==10 || month==12){
//        lastday = 31;
//        if(month == 3){
//            if((year%400 == 0)||(year%4 == 0 && year%100 != 0))				//if this is lunar year
//                lastlastday = 29;
//            else
//                lastlastday = 28;
//        }
//        if(month == 8)
//            lastlastday = 31;
//    }
//    else if(month == 4 || month == 6 || month == 9 || month == 11){
//        lastday = 30;
//        lastlastday = 31;
//    }
//    else{
//        lastlastday = 31;
//        if((year%400 == 0)||(year%4 == 0 && year%100 != 0))
//            lastday = 29;
//        else
//            lastday = 28;
//    }
//
//    if(hour >= 24){					// if >24, day+1
//            hour -= 24;
//            day += 1;
//
//            if(day > lastday){ 		// next month,  day-lastday of this month
//                day -= lastday;
//                month += 1;
//
//                if(month > 12){		//	next year , month-12
//                    month -= 12;
//                    year += 1;
//                }
//            }
//        }
//    if(hour < 0){										// if <0, day-1
//            hour += 24;
//            day -= 1;
//            if(day < 1){					  // month-1, day=last day of last month
//                day = lastlastday;
//                month -= 1;
//                if(month < 1){ 			// last year , month=12
//                    month = 12;
//                    year -= 1;
//                }
//            }
//        }
//   // transfer value to NMEA_result.local_time
//	NMEA_result.local_time.year  = year;
//	NMEA_result.local_time.month = month;
//	NMEA_result.local_time.date  = day;
//	NMEA_result.local_time.hour  = hour;
//	NMEA_result.local_time.min	 = utc_time->min;
//	NMEA_result.local_time.sec	 = utc_time->sec;
//}

// assess if this year is a leap year
int isLeapYear(int year) {
    if(year % 400 == 0) return 1;
    if(year % 100 == 0) return 0;
    if(year % 4 == 0) return 1;
    return 0;
}

// Returns the number of days in the specified month
int daysInMonth(int month, int year) {
    switch(month) {
        case 2: return isLeapYear(year) ? 29 : 28;
        case 4: case 6: case 9: case 11: return 30;
        default: return 31;
    }
}
// Determine whether daylight saving time adjustments should be applied
int isInDST(nmea_time* time) {
    int lastSundayOfMarch = 31 - (2 + 31 - time->year) % 7;
    int lastSundayOfOctober = 31 - (5 + 31 - time->year) % 7;

    if(time->month > 3 && time->month < 10) return 1;  // DST is active between last Sunday of March and October
    if(time->month == 3 && time->date > lastSundayOfMarch) return 1;  // After the last Sunday of March
    if(time->month == 3 && time->date == lastSundayOfMarch && time->hour >= 1) return 1;  // On the last Sunday of March but after 1am UTC
    if(time->month == 10 && time->date < lastSundayOfOctober) return 1;  // Before the last Sunday of October
    if(time->month == 10 && time->date == lastSundayOfOctober && time->hour < 1) return 1;  // On the last Sunday of October but before 1am UTC

    return 0;  // If none of the above conditions is met, it's not DST
}

void UTC_to_UKtime(nmea_time* utc_time) {
    int year, month, day, hour;

    year = utc_time->year;
    month = utc_time->month;
    day = utc_time->date;
    hour = utc_time->hour;

    if(isInDST(utc_time)) {
        hour++; // Move one hour forward for DST
    }

    // Check for time overflow after adjustment
    if(hour >= 24) {
        hour -= 24;
        day++;
        if(day > daysInMonth(month, year)) {
            day = 1;
            month++;
            if(month > 12) {
                month = 1;
                year++;
            }
        }
    }

    NMEA_result.local_time.year = year;
    NMEA_result.local_time.month = month;
    NMEA_result.local_time.date = day;
    NMEA_result.local_time.hour = hour;
    NMEA_result.local_time.min = utc_time->min;
    NMEA_result.local_time.sec = utc_time->sec;
}

unsigned int  xDate2Seconds(_xtime *time)
{
  static unsigned int  month[12]={
    /*January*/xDAY*(0),
    /*February*/xDAY*(31),
    /*March*/xDAY*(31+28),
    /*April*/xDAY*(31+28+31),
    /*May*/xDAY*(31+28+31+30),
    /*June*/xDAY*(31+28+31+30+31),
    /*July*/xDAY*(31+28+31+30+31+30),
    /*August*/xDAY*(31+28+31+30+31+30+31),
    /*September*/xDAY*(31+28+31+30+31+30+31+31),
    /*October*/xDAY*(31+28+31+30+31+30+31+31+30),
    /*November*/xDAY*(31+28+31+30+31+30+31+31+30+31),
    /*December*/xDAY*(31+28+31+30+31+30+31+31+30+31+30)
  };
  unsigned int  seconds = 0;
  unsigned int  year = 0;
  year = time->year-1970;       //Not considering the Y2K problem in 2100
  seconds = xYEAR*year + xDAY*((year+1)/4);  //Number of seconds elapsed in previous years
  seconds += month[time->month-1];      //plus the number of seconds elapsed this month
  if( (time->month > 2) && (((year+2)%4)==0) )//2008 is a leap year
    seconds += xDAY;            //Add 1 day and second in leap year
  seconds += xDAY*(time->day-1);         //plus the number of seconds elapsed today
  seconds += xHOUR*time->hour;           //plus the seconds elapsed in this hour
  seconds += xMINUTE*time->minute;       //plus the seconds elapsed in this minute
  seconds += time->second;               //Add the current number of seconds<br> seconds -= 8 * xHOUR;
  return seconds;
}

//UNIX to UTC has been converted to time zone Beijing time UTC+8
void xSeconds2Date(unsigned long seconds,_xtime *time )
{
    static unsigned int month[12]={
        /*January*/31,
        /*February*/28,
        /*March*/31,
        /*April*/30,
        /*May*/31,
        /*June*/30,
        /*July*/31,
        /*August*/31,
        /*September*/30,
        /*October*/31,
        /*November*/30,
        /*December*/31
    };
    unsigned int days;
    unsigned short leap_y_count;
    time->second      = seconds % 60;//Get seconds
    seconds          /= 60;
    time->minute      =  seconds % 60;//Get minutes
//    seconds          += 8 * 60 ;        //Get second time zone correction to UTC+8 bylzs
    seconds          /= 60;
    time->hour        = seconds % 24;//get hour
    days              = seconds / 24;//get total days
    leap_y_count = (days + 365) / 1461;//How many leap years have passed (one leap year every 4 years)
    if( ((days + 366) % 1461) == 0)
    {//The last day of leap year
        time->year = 1970 + (days / 366);//get year
        time->month = 12;              //adjust month
        time->day = 31;
        return;
    }
    days -= leap_y_count;
    time->year = 1970 + (days / 365);     //get year
    days %= 365;                       //What day is this year
    days = 01 + days;                  //Starting on the 1st
    if( (time->year % 4) == 0 )
    {
        if(days > 60)--days;            //Leap year adjustment
        else
        {
            if(days == 60)
            {
                time->month = 2;
                time->day = 29;
                return;
            }
        }
    }
    for(time->month = 0;month[time->month] < days;time->month++)
    {
        days -= month[time->month];
    }
    ++time->month;               //adjust month
    time->day = days;           //adjust days
}

/*******************************************************************************
* Function Name  : ConvertTimeToSecond
* Description    : Convert GPS Date to Log buffer.
* Input          : @date: format 'DDMMYY,HHMMSS.SSS'
* Output         : None
* Return         : Second
*******************************************************************************/
uint32_t ConvertDateToSecond(const uint8_t *date)
{
    static const uint16_t months[12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334  // February in leap years requires special treatment
    };

    uint32_t seconds = 0;
    uint16_t temp = 1970;
    uint16_t days = 0;

    if(NULL == date) {
        return 0;
    }
    //year
    temp = (date[4] - 0x30) * 10 + (date[5] - 0x30) + 2000;
    if(0 == (temp % 4) && (temp % 100 != 0 || temp % 400 == 0)) {
        days += 1;
    }
    temp -= 1;
    //UTC time start 1970
    for(; temp >= 1970; temp--) {
        if(temp % 4) {
            days += 365;
        } else {
            //leap year
            days += 366;
        }
    }
    //month
    temp = (date[2] - 0x30) * 10 + (date[3] - 0x30);
    temp -= 1;
    days += months[temp];
    if(temp > 1 && (date[4] - 0x30) * 10 + (date[5] - 0x30) % 4 == 0) {
        days += 1;  // If it is a leap year and the month exceeds February, add one day
    }
    //day
    temp = (date[0] - 0x30) * 10 + (date[1] - 0x30);
    days += temp - 1;
    //hour
    temp = (date[7] - 0x30) * 10 + (date[8] - 0x30);
    seconds += temp * ONEMINUTETOSENCOND * 60;
    //min
    temp = (date[9] - 0x30) * 10 + (date[10] - 0x30);
    seconds += temp * 60;
    //second
    temp = (date[11] - 0x30) * 10 + (date[12] - 0x30);
    seconds += temp;

    seconds += days * ONEDAYTOSENCOND;

    return seconds;
}

time_data read_time(uint32_t startTime){

	  time_data result;
	  // get RTC time and Date
	  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	  // show date and time
	  /* Display date Format : yy/mm/dd */
	  printf("%04d/%02d/%02d\r\n",2000 + sDate.Year, sDate.Month, sDate.Date);
	  /* Display time Format : hh:mm:ss */
	  printf("UTC Time is: %02d:%02d:%02d\r\n",sTime.Hours, sTime.Minutes, sTime.Seconds);

	  // create an array that converts the date and time obtained from the RTC into "DDMMYY,HHMMSS.SSS" format.
	  uint8_t rtcDate[14];
	  sprintf((char *)rtcDate, "%02d%02d%02d,%02d%02d%02d.000",
	          sDate.Date, sDate.Month, sDate.Year,
	          sTime.Hours, sTime.Minutes, sTime.Seconds);

	  // Convert RTC date and time to Unix timestamp
	  uint32_t timestamp = ConvertDateToSecond(rtcDate);
	  printf("Unix Timestamp: %u\n", timestamp);

	    // Populate the NMEA time structure with the date and time of the RTC
	  nmea_time testTime = {
			  .year = 2000 + sDate.Year,
			  .month = sDate.Month,
			  .date = sDate.Date,
			  .hour = sTime.Hours,
			  .min = sTime.Minutes,
			  .sec = sTime.Seconds
	  };


	   // Use UTC_to_UKtime function to convert time
	  UTC_to_UKtime(&testTime);

	   // print local uk time
	  printf("Local UK time: %04d-%02d-%02d %02d:%02d:%02d\n",
	           NMEA_result.local_time.year, NMEA_result.local_time.month, NMEA_result.local_time.date,
	           NMEA_result.local_time.hour, NMEA_result.local_time.min, NMEA_result.local_time.sec);

	  // Calculate the elapsed time since the program was run
	  uint32_t elapsedTime = HAL_GetTick() - startTime;
	  uint32_t elapsedSeconds = elapsedTime / 1000;
	  uint32_t elapsedMinutes = elapsedSeconds / 60;
	  elapsedSeconds %= 60;

	  printf("Elapsed time: %02u:%02u\n", elapsedMinutes, elapsedSeconds);
	  printf("\r\n");

	  result.unix_timestamp = timestamp;
	  // Assume that the NMEA_time structure can be directly assigned a value.
	  // If not, please assign a value to each field separately.
	  result.utc_time = testTime;
	  result.uk_time = NMEA_result.local_time;
	  result.elapsed_minutes = elapsedMinutes;
	  result.elapsed_seconds = elapsedSeconds;

	  return result;
}


