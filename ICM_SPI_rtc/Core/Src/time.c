#include "time.h"
#include "rtc.h"

//send timing request to PC from USB VCP
void time_request(void) {
    uint8_t request[] = "Get Time from VCP!!\r\n";
    CDC_Transmit_FS(request, sizeof(request) - 1); // 注意: 使用 sizeof(request) - 1，因为我们不想发送结束字符'\0'
	HAL_Delay(1000);
}

//STM32 receive time data
uint8_t rx_buffer[64];  // declare a receiver buffer

// 这是一个回调函数，当有数据通过USB CDC接收时会被调用 a callback function, when data passes USB CDC will call this function
void CDC_Received_Callback(uint8_t* Buf, uint32_t Len) {
    memcpy(rx_buffer, Buf, Len);  // 拷贝接收到的数据到rx_buffer copy received data to rx_buffer
    Set_RTC_From_Buffer(rx_buffer);
}

void Set_RTC_From_Buffer(uint8_t* buffer) {
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    // 假设你的PC端程序发送的格式为: "YYYY-MM-DD HH:MM:SS"
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

// 判断给定的年份是否为闰年
int isLeapYear(int year) {
    if(year % 400 == 0) return 1;
    if(year % 100 == 0) return 0;
    if(year % 4 == 0) return 1;
    return 0;
}

// 返回指定月份的天数
int daysInMonth(int month, int year) {
    switch(month) {
        case 2: return isLeapYear(year) ? 29 : 28;
        case 4: case 6: case 9: case 11: return 30;
        default: return 31;
    }
}
// 判断是否应该应用夏令时调整
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
    /*01月*/xDAY*(0),
    /*02月*/xDAY*(31),
    /*03月*/xDAY*(31+28),
    /*04月*/xDAY*(31+28+31),
    /*05月*/xDAY*(31+28+31+30),
    /*06月*/xDAY*(31+28+31+30+31),
    /*07月*/xDAY*(31+28+31+30+31+30),
    /*08月*/xDAY*(31+28+31+30+31+30+31),
    /*09月*/xDAY*(31+28+31+30+31+30+31+31),
    /*10月*/xDAY*(31+28+31+30+31+30+31+31+30),
    /*11月*/xDAY*(31+28+31+30+31+30+31+31+30+31),
    /*12月*/xDAY*(31+28+31+30+31+30+31+31+30+31+30)
  };
  unsigned int  seconds = 0;
  unsigned int  year = 0;
  year = time->year-1970;       //不考虑2100年千年虫问题
  seconds = xYEAR*year + xDAY*((year+1)/4);  //前几年过去的秒数
  seconds += month[time->month-1];      //加上今年本月过去的秒数
  if( (time->month > 2) && (((year+2)%4)==0) )//2008年为闰年
    seconds += xDAY;            //闰年加1天秒数
  seconds += xDAY*(time->day-1);         //加上本天过去的秒数
  seconds += xHOUR*time->hour;           //加上本小时过去的秒数
  seconds += xMINUTE*time->minute;       //加上本分钟过去的秒数
  seconds += time->second;               //加上当前秒数<br>　seconds -= 8 * xHOUR;
  return seconds;
}

//UNIX转为UTC 已进行时区转换 北京时间UTC+8
void xSeconds2Date(unsigned long seconds,_xtime *time )
{
    static unsigned int month[12]={
        /*01月*/31,
        /*02月*/28,
        /*03月*/31,
        /*04月*/30,
        /*05月*/31,
        /*06月*/30,
        /*07月*/31,
        /*08月*/31,
        /*09月*/30,
        /*10月*/31,
        /*11月*/30,
        /*12月*/31
    };
    unsigned int days;
    unsigned short leap_y_count;
    time->second      = seconds % 60;//获得秒
    seconds          /= 60;
    time->minute      =  seconds % 60;//获得分
//    seconds          += 8 * 60 ;        //时区矫正 转为UTC+8 bylzs
    seconds          /= 60;
    time->hour        = seconds % 24;//获得时
    days              = seconds / 24;//获得总天数
    leap_y_count = (days + 365) / 1461;//过去了多少个闰年(4年一闰)
    if( ((days + 366) % 1461) == 0)
    {//闰年的最后1天
        time->year = 1970 + (days / 366);//获得年
        time->month = 12;              //调整月
        time->day = 31;
        return;
    }
    days -= leap_y_count;
    time->year = 1970 + (days / 365);     //获得年
    days %= 365;                       //今年的第几天
    days = 01 + days;                  //1日开始
    if( (time->year % 4) == 0 )
    {
        if(days > 60)--days;            //闰年调整
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
    ++time->month;               //调整月
    time->day = days;           //获得日
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
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334  // 闰年的2月需要特殊处理
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
        days += 1;  // 如果是闰年并且月份超过2月，加一天
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
	  // 获�?� RTC 的时间和日期
	  HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	  HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	  // 显示日期和时间
	  /* Display date Format : yy/mm/dd */
	  printf("%04d/%02d/%02d\r\n",2000 + sDate.Year, sDate.Month, sDate.Date);
	  /* Display time Format : hh:mm:ss */
	  printf("UTC Time is: %02d:%02d:%02d\r\n",sTime.Hours, sTime.Minutes, sTime.Seconds);

	  // 创建一个数组，将从 RTC 获�?�的日期和时间转�?�为 "DDMMYY,HHMMSS.SSS" 格�?
	  uint8_t rtcDate[14];
	  sprintf((char *)rtcDate, "%02d%02d%02d,%02d%02d%02d.000",
	          sDate.Date, sDate.Month, sDate.Year,
	          sTime.Hours, sTime.Minutes, sTime.Seconds);

	  // 将 RTC 的日期和时间转�?�为 Unix 时间戳
	  uint32_t timestamp = ConvertDateToSecond(rtcDate);
	  printf("Unix Timestamp: %u\n", timestamp);

	    // 为NMEA time结构填充RTC的日期和时间
	  nmea_time testTime = {
			  .year = 2000 + sDate.Year,
			  .month = sDate.Month,
			  .date = sDate.Date,
			  .hour = sTime.Hours,
			  .min = sTime.Minutes,
			  .sec = sTime.Seconds
	  };


	   // 使用UTC_to_UKtime函数转�?�时间
	  UTC_to_UKtime(&testTime);

	    // 打�?�转�?��?�的英国�?令时时间
	  printf("Local UK time: %04d-%02d-%02d %02d:%02d:%02d\n",
	           NMEA_result.local_time.year, NMEA_result.local_time.month, NMEA_result.local_time.date,
	           NMEA_result.local_time.hour, NMEA_result.local_time.min, NMEA_result.local_time.sec);

	  // 计算程序运行后的流逝时间
	  uint32_t elapsedTime = HAL_GetTick() - startTime;
	  uint32_t elapsedSeconds = elapsedTime / 1000;
	  uint32_t elapsedMinutes = elapsedSeconds / 60;
	  elapsedSeconds %= 60;

	  printf("Elapsed time: %02u:%02u\n", elapsedMinutes, elapsedSeconds);
	  printf("\r\n");

	  result.unix_timestamp = timestamp;
	  result.utc_time = testTime;  // 假设NMEA_time结构体可以直接赋值，如果不行，请分别为每个字段赋值
	  result.uk_time = NMEA_result.local_time;
	  result.elapsed_minutes = elapsedMinutes;
	  result.elapsed_seconds = elapsedSeconds;

	  return result;
}


