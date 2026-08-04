/*
  Time.h - low level time and date functions
*/

#ifndef _Time_h
#define _Time_h

#include <inttypes.h>
#include <sys/types.h>

#if !defined(__time_t_defined) && !defined(_TIME_T_DEFINED)
typedef unsigned long time_t;
#endif

typedef enum {timeNotSet, timeNeedsSync, timeSet
}  timeStatus_t ;

typedef struct  {
  uint8_t Second;
  uint8_t Minute;
  uint8_t Hour;
  uint8_t Wday;   // day of week, sunday is day 1
  uint8_t Day;
  uint8_t Month;
  uint8_t Year;   // offset from 1970;
} 	tmElements_t, TimeElements, *tmElementsPtr_t;

//convenience macros to convert to and from tm years
#define  tmYearToCalendar(Y) ((Y) + 1970)  // full four digit year
#define  CalendarYrToTm(Y)   ((Y) - 1970)

typedef time_t(*getExternalTime)();

/*==============================================================================*/
/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)

/*  time and date functions   */
int     hour();
int     hour(time_t t);
int     minute();
int     minute(time_t t);
int     second();
int     second(time_t t);
int     day();
int     day(time_t t);
int     month();
int     month(time_t t);
int     year();
int     year(time_t t);

time_t  now();
void    setTime(time_t t);
void    setTime(int hr,int min,int sec,int day, int month, int yr);

/* time sync functions	*/
timeStatus_t timeStatus();
void    setSyncProvider( getExternalTime getTimeFunction);
void    setSyncInterval(time_t interval);

void breakTime(time_t time, tmElements_t &tm);
time_t makeTime(tmElements_t &tm);

#endif
