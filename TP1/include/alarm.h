#ifndef _ALARM_H_
#define _ALARM_H_

#include "shared.h"

extern int alarmFlag;

void startAlarm(unsigned int timeout);

void stopAlarm();

void sigalrm_handler(int signo);

#endif //_ALARM_H_
