/****************************************************************************
 *   Copyright (C) 2010-2017 by cvy7                                        *
 *                                                                          *
 *   This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation; either version 2 of the License.         *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program; if not, write to the                          *
 *   Free Software Foundation, Inc.,                                        *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.              *
 ****************************************************************************/ 
//****************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#define AUT_MAX_TIMERS   16
// коэффициент расчета дла таймеров
#define AUT_KT 61L/4L+2
//(1./0.065535)
#define AUT_TIMER_RESET 1

extern unsigned int time65;
typedef struct {
                int timer;
                int time;
               } AUT_t_timer;

int AUT_time;
char AUT_event=0;
char AUT_next_run;
AUT_t_timer AUT_timers[AUT_MAX_TIMERS];

int AUT_timer(int time, char chan){
    cli();
    int   AUT_time65=time65;
    sei();
    if(time){
      if(time > 32700) time=32700;
      AUT_timers[chan].timer= AUT_time65;
      AUT_timers[chan].time = time-1;
    }
    else{
        if(AUT_timers[chan].time){
         if((AUT_time65-AUT_timers[chan].timer) > AUT_timers[chan].time) {
          AUT_timers[chan].time=0;
          return 1;
         }
        }
    }
    return 0;
}

float AUT_pid(float set,float mes, float kp, float ki, float *integ)
{
float pid_output=0;
float delta=set-mes;

if( ((delta>0) && (*integ<1)) || ((delta<0)&&(*integ>-1)))
            (*integ)+=ki*delta;

pid_output=kp*delta+(*integ);
if(pid_output>1) pid_output=1;
if(pid_output<-1) pid_output=-1;
return pid_output;
}

float AUT_interp(int per, float * interp_data){
    return pgm_read_float(interp_data+per);
}


