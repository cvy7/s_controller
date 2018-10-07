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
//#include "mk_pl_bus.h"
#include "mb.h"

//#ifdef HW1408
//pin6
#define KBD_PIN0 (PINE & (1<<2))
//pin7
#define KBD_PIN1 (PINF & (1<<0))
//pin8
#define KBD_PIN2 (PINF & (1<<1))
//pin9
#define KBD_PIN3 (PINF & (1<<2))

#define KBD_PIN_READY KBD_PIN0
#define KBD_PIN_HEAT  KBD_PIN1
#define KBD_PIN_RESET KBD_PIN2
#define KBD_PIN_AUX   KBD_PIN3

//pin1
#define KBD_PORT0(a) if(a) {PORTB &=~(1<<0); DDRB &=~(1<<0);} else DDRB |=(1<<0);
//pin2
#define KBD_PORT1(a) if(a) {PORTE &=~(1<<7); DDRE &=~(1<<7);} else DDRE |=(1<<7);
//pin3
#define KBD_PORT2(a) if(a) {PORTE &=~(1<<6); DDRE &=~(1<<6);} else DDRE |=(1<<6);
//pin4
#define KBD_PORT3(a) if(a) {PORTE &=~(1<<5); DDRE &=~(1<<5);} else DDRE |=(1<<5);
//pin5
#define KBD_PORT4(a) if(a) {PORTE &=~(1<<3); DDRE &=~(1<<3);} else DDRE |=(1<<3);
#ifdef ADCM
   // Вместо D8 выхода 68 нога
    #define KBD_PORT5(a) if(a) {PORTA &=~(1<<3); DDRA &=~(1<<3);} else DDRA |=(1<<3);
#else
    //доп разъем для кнопок на 14_08 - перемычка с затвора непоставленного транзистора крайней релюшки
    #define KBD_PORT5(a) if(a) {PORTF &=~(1<<6); DDRF &=~(1<<6);} else DDRF |=(1<<6);
#endif

#define ENC_INTERRUPT INT4_vect
#define EN_ENC_INT EICRB|= (1<<ISC41);EIMSK|=(1<<INT4)
#define ENC_PIN (PINF & (1<<5))
//#endif


#define KBD_BUFFER_SIZE 16
//Время периодического вызова опроса клавиатуры в мкс
#define KBD_TIME (1000)
//Время минимально необходимого отжатого состояния по каждой линии в мс- фактическое время - в 5 раз больше
#define KBD_DEBOUNCE (10)
//Время удержания сброса в мс для запуска процесса прописывания заводских настроек
#define KBD_TIME_RESET_SET (2000)
//Время  в мс после включения, после истечения которого клавиатура начнет реагировать на нажатия
#define KBD_STARTUP_TIME (2000)

#define KBD_K_ENC 4

#define KBD_MAX_LINE (6)
#define KBD_MAX_HW_BUTTON (6)
#define KBD_HW0 (0)
#define KBD_HW1 (1)
#define KBD_HW2 (2)
#define KBD_HW3 (3)
#define KBD_HWF (4)
#define KBD_HWF1 (5)

char KBD_buffer[KBD_BUFFER_SIZE];
char KBD_rd_index, KBD_wr_index,KBD_counter;
char KBD_event;
char KBD_next_run=0;
int KBD_time;
int KBD_reset_cntr;
char KBD_debounce[KBD_MAX_LINE+KBD_MAX_HW_BUTTON];
char KBD_state=0;
char KBD_shift;
volatile int KBD_enc;
int KBD_startup_timer;



void KBD_init(){
   KBD_rd_index=0;
   KBD_wr_index=0;
   KBD_counter=0;
   KBD_event=0;
   EN_ENC_INT;
}

static inline void KBD_putchar(char data){
   cli();
   if(KBD_counter <= KBD_BUFFER_SIZE){
      KBD_buffer[KBD_wr_index] = data;
      if(++KBD_wr_index == KBD_BUFFER_SIZE)
          KBD_wr_index = 0;
      KBD_counter++; 
   };
   sei();
}

static inline void KBD_putchar_DBh(char c, char n){  // независимое гашение дребезга- для hw кнопок и зажатой 'F'(shift)
  if(n>(KBD_MAX_HW_BUTTON-1)) n=(KBD_MAX_HW_BUTTON-1);
  if(c) {
    if(!KBD_debounce[KBD_MAX_LINE+n]) KBD_putchar(c);//ипользуются отдельные ячейки для таймера гашения
                                                     // их количество - KBD_MAX_HW_BUTTON !!!!
    KBD_debounce[KBD_state]= (KBD_DEBOUNCE*10);      // только для определения отпускания в этой линии
    KBD_debounce[KBD_MAX_LINE+n]= (KBD_DEBOUNCE*5);  // отпускание для повторного срабатывания ТОЙ-ЖЕ клавиши
  }
  else
    if (KBD_debounce[KBD_MAX_LINE+n]) KBD_debounce[KBD_MAX_LINE+n]--;
}

static inline void KBD_putchar_DBn(char c){// Автоповтор- для стрелок
    if(!KBD_debounce[KBD_state]){
        KBD_putchar(c);
        KBD_debounce[KBD_state]= (KBD_DEBOUNCE*5);//<--с периодичностью
    }
}

static inline void KBD_putchar_DB1(char c){// Однократное нажатие- для большинства клавиш
    if(!KBD_debounce[KBD_state]) KBD_putchar(c);
    KBD_debounce[KBD_state]= KBD_DEBOUNCE;
}
/*
SIGNAL(ENC_INTERRUPT)
{
    if(ENC_PIN) KBD_enc--;
    else        KBD_enc++;

#if 0
    if(KBD_enc < -KBD_K_ENC) { KBD_putchar('-');KBD_enc=0;}
    else
    if(KBD_enc >  KBD_K_ENC) { KBD_putchar('+');KBD_enc=0;};
#endif

}
*/
void KBD_poll()
{
    cli();
    usRegInputBuf[0]=KBD_enc;
    sei();

    if(!KBD_next_run){
        KBD_init();
        KBD_next_run=1;
    }
#if 0
    if(KBD_startup_timer< KBD_STARTUP_TIME){
        KBD_startup_timer++;
        return;
    }

    if(KBD_debounce[KBD_state]) KBD_debounce[KBD_state]--;
    switch(KBD_state){
    default:
        KBD_state=0;
        if(!KBD_PIN0) KBD_putchar_DB1('\n');// enter
        //if(!KBD_PIN3) KBD_putchar_DB1(0);
        if(!KBD_PIN1) KBD_putchar_DBn('B');// dn
        else
        if(!KBD_PIN2) KBD_putchar_DBn('C');// -->
        else// стрелки отжаты- укорачиваем паузу до номинальной
           if(KBD_debounce[KBD_state] > KBD_DEBOUNCE) KBD_debounce[KBD_state]= KBD_DEBOUNCE;

        KBD_PORT0(1);KBD_PORT1(0);KBD_PORT2(1);KBD_PORT3(1);KBD_PORT4(1);KBD_PORT5(1);
    break;
    case 1:
        if(!KBD_PIN0) KBD_putchar_DB1(0x1b);// esc
        //if(!KBD_PIN3) KBD_putchar_DB1(0);
        if(!KBD_PIN1) KBD_putchar_DBn('D');// <--
        else
        if(!KBD_PIN2) KBD_putchar_DBn('A');// up
        else// стрелки отжаты- укорачиваем паузу до номинальной
           if(KBD_debounce[KBD_state] > KBD_DEBOUNCE) KBD_debounce[KBD_state]= KBD_DEBOUNCE;

        KBD_PORT0(1);KBD_PORT1(1);KBD_PORT2(0);KBD_PORT3(1);KBD_PORT4(1);KBD_PORT5(1);
        break;
    case 2:
        if(KBD_shift){
            if(!KBD_PIN0) KBD_putchar_DBh('y',KBD_HWF1);// 9
            else
            if(!KBD_PIN1) KBD_putchar_DBh('z',KBD_HWF1);// 0
            else
            if(!KBD_PIN2) KBD_putchar_DBh('o',KBD_HWF1);// .
            else          KBD_putchar_DBh(0,  KBD_HWF1);
        }
        else{
        if(!KBD_PIN0) KBD_putchar_DB1('9'); // 9
        if(!KBD_PIN1) KBD_putchar_DB1('0');// 0
        if(!KBD_PIN2) KBD_putchar_DB1('.');// .
        }

        if(!KBD_PIN3) {KBD_putchar_DBh('=',KBD_HWF);KBD_shift=1;}// F->'='->shift
            else       KBD_putchar_DBh(0,KBD_HWF);

        if(KBD_debounce[KBD_MAX_LINE+KBD_HWF]== 1) KBD_shift=0;// F отпущен

        KBD_PORT0(1);KBD_PORT1(1);KBD_PORT2(1);KBD_PORT3(0);KBD_PORT4(1);KBD_PORT5(1);
        break;
    case 3:
        if(!KBD_PIN0) KBD_putchar_DB1('5');// 5
        if(!KBD_PIN1) KBD_putchar_DB1('6');// 6
        if(!KBD_PIN2) KBD_putchar_DB1('7');// 7
        if(!KBD_PIN3) KBD_putchar_DB1('8');// 8
        KBD_PORT0(1);KBD_PORT1(1);KBD_PORT2(1);KBD_PORT3(1);KBD_PORT4(0);KBD_PORT5(1);
        break;
    case 4:
        if(KBD_shift){
            if(!KBD_PIN0) KBD_putchar_DB1('p');// 1
            if(!KBD_PIN1) KBD_putchar_DB1('q');// 2
            if(!KBD_PIN2) KBD_putchar_DB1('r');// 3
            if(!KBD_PIN3) KBD_putchar_DB1('s');// 4
        }
        else {
        if(!KBD_PIN0) KBD_putchar_DB1('1');// 1
        if(!KBD_PIN1) KBD_putchar_DB1('2');// 2
        if(!KBD_PIN2) KBD_putchar_DB1('3');// 3
        if(!KBD_PIN3) KBD_putchar_DB1('4');// 4
        }
        KBD_PORT0(1);KBD_PORT1(1);KBD_PORT2(1);KBD_PORT3(1);KBD_PORT4(1);KBD_PORT5(0);
        break;
    case 5:
        if(!KBD_PIN_READY) KBD_putchar_DBh('R',KBD_HW0);// pin3 KBD_PIN0 Готов
           else KBD_putchar_DBh(0,KBD_HW0);
        if(!KBD_PIN_HEAT) KBD_putchar_DBh('S',KBD_HW1);// pin4 KBD_PIN1 Нагрев
           else KBD_putchar_DBh(0,KBD_HW1);
        if(!KBD_PIN_RESET) { KBD_putchar_DBh('P',KBD_HW2);// pin5 KBD_PIN2 Сброс
                        KBD_reset_cntr++;
                      }
           else KBD_putchar_DBh(0,KBD_HW2);
        if(!KBD_PIN_AUX) KBD_putchar_DBh('T',KBD_HW3);// pin6 KBD_PIN_3
           else KBD_putchar_DBh(0,KBD_HW3);

        if(KBD_debounce[KBD_state]== 1)         KBD_putchar('Q');// Все отпущено -> значит Стоп
        if(KBD_debounce[(KBD_MAX_LINE+KBD_HW1)]== 1)  KBD_putchar('R');// Нагрев отпущен->ready
        if(KBD_debounce[(KBD_MAX_LINE+KBD_HW2)]== 1) { KBD_putchar('U');// Сброс отпущен
                                                     KBD_reset_cntr=0;
                                                   }
        if(KBD_reset_cntr == KBD_TIME_RESET_SET) KBD_putchar('o');// Удержание сброса- зав настройки

        KBD_PORT0(0);KBD_PORT1(1);KBD_PORT2(1);KBD_PORT3(1);KBD_PORT4(1);KBD_PORT5(1);
        break;
    }
    if(KBD_counter) KBD_event=1;
    if(KBD_state<(KBD_MAX_LINE-1)) KBD_state++;
    else KBD_state=0;
#endif

};

char KBD_getchar()
{
    char data;
      if(KBD_counter == 0) data=0;
      else{
          data = KBD_buffer[KBD_rd_index];
          if(++KBD_rd_index == KBD_BUFFER_SIZE)
             KBD_rd_index = 0;
          cli();
          --KBD_counter;
          sei();
          };
    return data;
};
