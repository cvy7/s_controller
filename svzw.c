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
#define ERR_CONNECT (999)
//*******************************************************************************
// протокол связи с внешними устройствами
#define PMODBUS
//*******************************************************************************
//альтернативное использование led()- например,для индикации сигналов
//#define ALT_LED
//****************************************************************************/
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/twi.h>
#include <stdio.h>
#include <string.h>

#include "svzw.h"
#include "defs.h"

unsigned char err_hw=1;
//char MAIN_err=0;
volatile unsigned int time65;

//tconv conv;//сигналы относящиеся к преобразователю

//адрес начала загрузочной секции ATmega128
#define BOOT_SECTION 0xF800
void(*pBootloader)(void) ;
//pBootloader = (void(*)())BOOT _SECTION;
//pBootloader ();
//**********************************************************************************************************/
#include "mb.h"
#include "mbport.h"
//#include "mk_pl_bus.h"
#include "ser.h"
#include "pin.h"
#include "aut.h"
#include "lcd_hd4478_driver.h"
#include "kbd.h"
#include "hmi.h"
#include "conv.h"
#include "conv_ser_interf.h"
#include "i2c.h"
//#include "termo.h"
//**********************************************************************************************************
void  main(void){
int time;
PIN_init();
#ifdef PMODBUS
    PMODBUS_init();
#endif
    //var_init();
for(;;){
sei();
time=TCNT1;// системное время
#if 0
    if(BIT(HMI_change_event,CEVENT_SET1)){
        SET(HMI_change_event,CEVENT_SET1,0);
        HMI_var_set1();
       // var_set1();
    }
    else
    if(BIT(HMI_change_event,CEVENT_MAIN)){
        SET(HMI_change_event,CEVENT_MAIN,0);
        //if(!conv.mestnoe_upr ){
            HMI_var_set();
            //var_set();
        //}
        //else HMI_var_get();
    }
#endif
 /*   if(BIT(HMI_change_event,CEVENT_INIT)){
        SET(HMI_change_event,CEVENT_INIT,0);
        var_init();
    }^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
*/
    if((time-i2c_time)>= I2C_TIME){//444мкс //максимальный пакет 250 мкс в сумме со стартом и стопом
        i2c_time=time;//таким образом - что к следующему вызову модбаса/автоматики/связи
                      //все входы- выходы обновляются
        i2c_poll();
        PIN_poll();
    }

    if((time65-AUT_time)>= AUT_TIME){//3555.5(5) мкс
        AUT_time=time65;
        SER_poll();
        PMODBUS_rx();
        AUT_poll();
        CONV_comm_poll();
#ifdef PMODBUS
     ( void )eMBPoll(  );
#endif
        //CONV_poll();
        //var_get();
    }

#if 0
    if((time-KBD_time)>= KBD_TIME){//1000мкс
        //KBD_time=time;

        KBD_poll();
#endif
#if 0
        if(HMI_asinc_time>HMI_ASINC_TIME) {
            HMI_asinc_event=1;
            HMI_asinc_time=0;
        }
        else
            HMI_asinc_time++;
            HMI_hwo();
        KBD_time=time;
    }
#endif
#if 0
    if((time-TERMO_time)> TERMO_TIME){//1000мкс
        //TERMO_time=time;
        TERMO_poll();
        TERMO_time=time;
    }
#endif

#if 0
    if(KBD_event || AUT_event /*|| conv_is_received */||HMI_asinc_event || BIT(HMI_change_event,CEVENT_SET2)){//200мс макс.
            SET(HMI_change_event,CEVENT_SET2,0);
            HMI_poll();
            AUT_event=0;
    }
    SER_poll();
#endif
//*************************************

//***********************************
     wdt_reset();
}// end for(;;)
}// end main()


