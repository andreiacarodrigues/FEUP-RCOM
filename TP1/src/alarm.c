#include <signal.h> //sigaction
#include <unistd.h> //alarm

#include <stdio.h> //printf

#include "alarm.h"

int alarmFlag;
static unsigned int alarmTime;

void startAlarm(unsigned int timeout)
{
    //Reset the flag
    alarmFlag = 0;

    //Setup the signal handler
    struct sigaction action;
    action.sa_handler = sigalrm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGALRM, &action, NULL);

    //Set up the alarm itself
    alarmTime = timeout;
    alarm(alarmTime);
}

void stopAlarm()
{
    struct sigaction action;
    action.sa_handler = NULL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGALRM, &action, NULL);

    alarm(0);
}

void sigalrm_handler(int signo)
{
    if (signo != SIGALRM)
    {
        return;
    }

    if (DEBUG_MSGS)
    {
        printf("sigalrm_handler.\n");
    }

    alarmFlag = 1;

    alarm(alarmTime);
}