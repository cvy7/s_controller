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
#include "ports.h"
#include "conv.h"
//#include "mk_pl_bus.h"
//#include "lcd_hd4478_driver.h"
#include "aut.h"

tconv  conv;//сигналы относящиеся к преобразователю
//tdiags diags;//диагностики
//tplst plst;
signed char CONV_state;
int         CONV_cn_state;
#define CONV_NMOD  4
//4 сигнала контроля модулей
//************************************************************************
#if 0
const PROGMEM char CONV_err1 []="1:Превышен входн.ток";
#define ERR_ID_MAX 1
const PROGMEM char CONV_err2 []="2НизкоеВх.напр.Фаза?";
#define ERR_UD_MIN 2
const PROGMEM char CONV_err3 []="3:Высокое выпр.напр.";
#define ERR_UD_MAX 3
const PROGMEM char CONV_err4 []="4:Превышен выход.ток";
#define ERR_IO_MAX 4
const PROGMEM char CONV_err5 []="5Превышен ток коммут";
#define ERR_IO_MAXC 5
const PROGMEM char CONV_err6 []="6Превыш.выход.напряж";
#define ERR_UO_MAX 6
const PROGMEM char CONV_err7 []="7Превыш.част.контура";
#define ERR_FO_MAX 7
const PROGMEM char CONV_err8 []="8Низкая.част.контура";
#define ERR_FO_MIN 8
const PROGMEM char CONV_err9 []="9:АПЧвне захвата ОС?";
#define ERR_PLL    9
const PROGMEM char CONV_err10[]="10Неисправн.нагр/ОС?";
#define ERR_NAGR   10
const PROGMEM char CONV_err11[]="11Блокировка по воде";
#define ERR_WTR    11
const PROGMEM char CONV_err12[]="12Блокировка по воде";
#define ERR_WTR1   12
const PROGMEM char CONV_err13[]="13Нажат Авар.Останов";
#define ERR_EMER_STOP 13
const PROGMEM char CONV_err14[]="14Открыты двери шкаф";
#define ERR_DOORS 14
const PROGMEM char CONV_err15[]="15Трансф.ток.вых нев";
#define ERR_TT 15

#define ERR_OVH     20
const PROGMEM char CONV_err20[]="20Перегрев Выпр.верх";
const PROGMEM char CONV_err21[]="21Перегрев Инв. верх";
const PROGMEM char CONV_err22[]="22Перегрев Выпр.нижн";
const PROGMEM char CONV_err23[]="23Перегрев Инв. нижн";
const PROGMEM char CONV_err24[]="24Перегрев          ";
const PROGMEM char CONV_err25[]="25Перегрев          ";
const PROGMEM char CONV_err26[]="26Перегрев          ";
const PROGMEM char CONV_err27[]="27Перегрев          ";

#define ERR_T_BK     49
const PROGMEM char CONV_err49[]="49Перегрев конден.БК";

#define ERR_MOD 50
const PROGMEM char CONV_err50[]="50Защита IGBT12 верх";
const PROGMEM char CONV_err51[]="51Защита IGBT34 верх";
const PROGMEM char CONV_err52[]="52Защита IGBT12 нижн";
const PROGMEM char CONV_err53[]="53Защита IGBT34 нижн";

#define ERR_MOD1 100
const PROGMEM char CONV_err100[]="100Неиспр.дрвр12верх";
const PROGMEM char CONV_err101[]="101Неиспр.дрвр34верх";
const PROGMEM char CONV_err102[]="102Неиспр.дрвр12нижн";
const PROGMEM char CONV_err103[]="103Неиспр.дрвр34нижн";

#define ERR_MOD2 150
const PROGMEM char CONV_err150[]="150:Неиспр.мод.верх";
const PROGMEM char CONV_err151[]="151:Неиспр.мод.нижн";

#define ERR_UNIDENT 500
const PROGMEM char CONV_err500[]="500:Неидентиф.ошибк";

#define ERR_CONNECT 999
const PROGMEM char CONV_err999[]="999:Нет связи с ПДУ";

void CONV_err_print(){
    switch(conv.err_cod[0]) {
        case 1:LCD_PPrint(CONV_err1);break;
        case 2:LCD_PPrint(CONV_err2);break;
        case 3:LCD_PPrint(CONV_err3);break;
        case 4:LCD_PPrint(CONV_err4);break;
        case 5:LCD_PPrint(CONV_err5);break;
        case 6:LCD_PPrint(CONV_err6);break;
        case 7:LCD_PPrint(CONV_err7);break;
        case 8:LCD_PPrint(CONV_err8);break;
        case 9:LCD_PPrint(CONV_err9);break;
        case 10:LCD_PPrint(CONV_err10);break;
        case 11:LCD_PPrint(CONV_err11);break;
        case 12:LCD_PPrint(CONV_err12);break;
        case 13:LCD_PPrint(CONV_err13);break;
        case 14:LCD_PPrint(CONV_err14);break;
        case 15:LCD_PPrint(CONV_err15);break;

        case 20:LCD_PPrint(CONV_err20);break;
        case 21:LCD_PPrint(CONV_err21);break;
        case 22:LCD_PPrint(CONV_err22);break;
        case 23:LCD_PPrint(CONV_err23);break;
        case 24:LCD_PPrint(CONV_err24);break;
        case 25:LCD_PPrint(CONV_err25);break;
        case 26:LCD_PPrint(CONV_err26);break;
        case 27:LCD_PPrint(CONV_err27);break;

        case 49:LCD_PPrint(CONV_err49);break;

        case 50:LCD_PPrint(CONV_err50);break;
        case 51:LCD_PPrint(CONV_err51);break;
        case 52:LCD_PPrint(CONV_err52);break;
        case 53:LCD_PPrint(CONV_err53);break;

        case 100:LCD_PPrint(CONV_err100);break;
        case 101:LCD_PPrint(CONV_err101);break;
        case 102:LCD_PPrint(CONV_err102);break;
        case 103:LCD_PPrint(CONV_err103);break;


        case 150:LCD_PPrint(CONV_err150);break;
        case 151:LCD_PPrint(CONV_err151);break;

        case 500:LCD_PPrint(CONV_err500);break;

        case 999:LCD_PPrint(CONV_err999);break;
    }
}
#endif
void CONV_charge(int a){
 /*   var_w(RO_CHARGE,a);
    AUT_charge(a);*/
}


void CONV_err(){
    CONV_heat_off();
    CONV_state=CONV_ERR_ON;
    conv.err=1;
}

void CONV_err_off(){
    CONV_off();
    CONV_state=CONV_ERR_OFF;
    conv.err=1;
}



void CONV_err_read(){

}
//**************************************************************************************************************************
void CONV_heat_on(void)
{
  conv.command_heat_on=1;
  conv.heat_on=1;
  //if(conv.err_cod[0]==ERR_CONNECT) CONV_state=CONV_HEAT_ON;
}

//*************************************************************************
void CONV_heat_off(void)
{
conv.heat_on=0;
conv.command_heat_on=0;
//if(conv.err_cod[0]==ERR_CONNECT) CONV_state=CONV_READY;
}
//************************************************************************
void CONV_off(void)
{
    conv.ready=0;
    conv.command_ready=0;
#if 0
    if(conv.err_cod[0]==ERR_CONNECT) {
    if(CONV_state==CONV_ERR_ON || CONV_state==CONV_ERR_OFF) {
        CONV_state=CONV_ERR_OFF;
    }
    else   CONV_state=CONV_OFF;
    }
#endif
}
//************************************************************************
void CONV_ready(void)
{
   conv.command_ready=1;
   conv.ready=1;
   //if(conv.err_cod[0]==ERR_CONNECT) CONV_state=CONV_READY;

}

void CONV_reset(void)
{
    conv.err=0;
    conv.heat_on=0;
    conv.command_reset=1;
    //if(conv.err_cod[0]==ERR_CONNECT) CONV_state= CONV_READY;
}
//***********************************************************************

void CONV_poll(void){/*
    switch(CONV_state){
    case CONV_READY: //100ms
        if(conv.command_heat_on)
            CONV_heat_on();
        break;
    case CONV_OFF:
        if(conv.command_ready)
            CONV_ready();
        break;
    }*/
}

