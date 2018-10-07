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
#include "defs.h"
#include "pin.h"
#include "mb.h"
#include "mb_regs.h"
#include "i2c.h"
#include "config.h"
//signed int (long)
//usRegInputBuf[0]-usRegInputBuf[3]- adc 0-255mv 0-10v 4-20ma
//usRegInputBuf[4]- adc 0-5v на гнезде энкодера - ручной задатчик- либо вход с платы расширения
//usRegInputBuf[5]-usRegInputBuf[6] enc 0
//usRegInputBuf[7]-usRegInputBuf[8] enc 1
//usRegInputBuf[9]-usRegInputBuf[10] enc 2 - ручной задатчик энкодером - либо вхлд с пл. расширения



//usRegDiscBuf[0]-usRegDiscBuf[15] дискретные входы

//usRegHoldingBuf[0]-usRegHoldingBuf[3]- dac 4-20 ma
//usRegHoldingBuf[4] 4 дискр.выход PWM 281Гц 16bit // 4 и 6 usRegHoldingBuf модуляция одного шима другим.
//usRegHoldingBuf[5] 5 дискр.выход PWM 281Гц 16bit
//usRegHoldingBuf[6] 4 дискр.выход PWM 72кГц 8bit /синтез частоты PIN_PWM_0
//usRegHoldingBuf[7] 5 дискр.выход PWM 72кГц 8bit /синтез частоты PIN_PWM_1

//usRegCoilsBuf[0]-usRegCoilsBuf[14] дискретные выходы
//usRegCoilsBuf[15]- 1 отключение внотреннего в плате управления выходами (только от модбаса)

//32-63 -то же самое для ведомой по опто-SPI платы

#define PIN_DEB 4

#define PIN_PWM_0 1
#define PIN_PWM_1 1

char PIN_debounce[16];

unsigned int integ_adc;
char integ_adc_cntr;

extern volatile unsigned int time65;

void PIN_init(){

    wdt_enable(WDTO_2S);
    wdt_reset();
    PIN_output_init();

    // 0 таймер - шим
    TCCR0=(1<<WGM01)|(PIN_PWM_1<<WGM00)|(1<<CS00);;

    // 1 таймер-системный таймер
    OCR1A=0xffff;//FAST PWM TOP=OCR1A
    TCCR1A=(1<<WGM10)|(1<<WGM11);
    TCCR1B=(1<<ICNC1)|(0<<ICES1)|(1<<WGM13)|(1<<WGM12)|(1<<CS10);
    TIMSK=(1<<ICF1)|(1<<TOIE1);

    //2 таймер -шим 72 кГц
    TCCR2=(1<<WGM21)|(PIN_PWM_0<<WGM20)|(1<<CS20);
    //WGM20=1 - PWM 72 kHz WGM20=0 CTC- синтез частоты 18432000/ 2*OCR2

    //Компаратор (6 вход)
    ACSR=(1<<ACBG)|(1<<ACIC)|(1<<ACIE);
    //прерывание на оба фронта ACIS1, ACIS0=0 + input capture

    EIMSK=(1<<INT4);// RX RS232 для трансляции в 485
    EICRB=(0<<ISC40);//  0 - только спад

    EIMSK|=(1<<INT5);// 7 вход- энкодер
    EICRB|=(1<<ISC50);// оба фронта 0 - только спад

    EIMSK|=(1<<INT7);// 1-2 вход разъем 5v энкодера
    EICRB|=(1<<ISC70);// оба фронта 0 - только спад

    EIMSK|=(1<<INT6);// 5-6 вход разъем 5v энкодера
    EICRB|=(1<<ISC60);// оба фронта 0 - только спад

    ADMUX=(1<<REFS0);//v1
    //ADMUX=(1<<REFS0)|(1<<MUX0); //v1_1 5v потенциометр
    //ADMUX=(1<<REFS1)|(1<<REFS0)|(1<<MUX3)|(1<<MUX0); //v1_1 0.256v
    //ADMUX=(1<<REFS1)|(1<<REFS0)|(1<<MUX3)|(1<<MUX1)|(1<<MUX0); //v1_1 0.0128v

    ADCSRA=(1<<ADEN)|(1<<ADSC)|(1<<ADFR)|(1<<ADIF)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);

    //USART1 Оптический вход
    //UCSR1A=(1<<U2X1);
    UCSR1B=(1<<RXCIE1)|(1<<TXEN1)|(1<<RXEN1);
    UCSR1C=(1<<USBS1)|(1<<UCSZ10)|(1<<UCSZ11);//UPM11 -Even Parity
    UBRR1H=0;
  #ifdef TPCH
    UBRR1L=119;//9600 8n2 @18.432MHz
  #else
    UBRR1L=9;//115200 8n2 @18.432MHz
  #endif

    //USART0 modbus 485 232 + отладочный разъем
    UCSR0B=(1<<TXEN0)|(1<<RXEN0);
    UCSR0C=(1<<USBS0)|(1<<UCSZ10)|(1<<UCSZ11);//UPM11 -Even Parity
    UBRR0H=0;
    UBRR0L=9;//115200 18,432 МГц
}

SIGNAL(TIMER1_OVF_vect) //системный таймер
{
          //conv_timeout++;
          //conv_start_tx=1;
          time65++;
}

SIGNAL(TIMER1_CAPT_vect)// прерывание по 6 входу - input capture измерение частоты/периода
{

}

SIGNAL(ANALOG_COMP_vect)// прерывание по 6 входу- оба фронта - энкодер
{

}



SIGNAL(INT5_vect)// прерывание по 7 входу - оба фронта - энкодер
{

}

SIGNAL(INT7_vect)// прерывание по вх 1-2 5v энкодер - оба фронта -
{

}

SIGNAL(INT6_vect)// прерывание по вх 5-6 5v энкодер - оба фронта -
{

}

SIGNAL(ADC_vect)
{
    integ_adc+=ADC;
    if(integ_adc_cntr>31){
        usRegInputBuf[4]/*adc_in[8]*/=integ_adc;
        integ_adc_cntr=0;
    }
    else integ_adc_cntr++;
}
void PIN_output_init(){
    SET(PORTD,4,1);SET(DDRD,4,1);
    SET(PORTF,2,1);SET(DDRF,2,1);
    SET(PORTD,6,1);SET(DDRD,6,1);
    SET(PORTD,7,1);SET(DDRD,7,1);
    SET(PORTB,7,1);SET(DDRB,7,1);
    SET(PORTB,6,1);SET(DDRB,6,1);
    SET(PORTG,0,1);SET(DDRG,0,1);
    SET(PORTG,1,1);SET(DDRG,1,1);
}

#define PIN_SET_0(var) SET(PORTD,4,!var)
#define PIN_SET_1(var) SET(PORTF,2,!var)
#define PIN_SET_2(var) SET(PORTD,6,!var)
#define PIN_SET_3(var) SET(PORTD,7,!var)
#define PIN_SET_4(var) SET(PORTB,7,!var)
#define PIN_SET_5(var) SET(PORTB,6,!var)
#define PIN_SET_6(var) SET(PORTG,0,!var)
#define PIN_SET_7(var) SET(PORTG,1,!var)

#define PIN_BIT_0 BIT(PINA,2)
#define PIN_BIT_1 BIT(PINA,1)
#define PIN_BIT_2 BIT(PINA,0)
#define PIN_BIT_3 BIT(PINF,7)
#define PIN_BIT_4 BIT(PINF,6)
#define PIN_BIT_5 BIT(PINF,5)
#define PIN_BIT_6 BIT(PINF,4)
#define PIN_BIT_7 BIT(PINF,3)


void PIN_poll(){

static int coils=0;
static int holding[4];

    if(!PIN_BIT_0 && PIN_debounce[0]<PIN_DEB) PIN_debounce[0]++;
    if(PIN_BIT_0 && PIN_debounce[0]) PIN_debounce[0]--;

    if(!PIN_BIT_1 && PIN_debounce[1]<PIN_DEB) PIN_debounce[1]++;
    if(PIN_BIT_1 && PIN_debounce[1]) PIN_debounce[1]--;

    if(!PIN_BIT_2 && PIN_debounce[2]<PIN_DEB) PIN_debounce[2]++;
    if(PIN_BIT_2 && PIN_debounce[2]) PIN_debounce[2]--;

    if(!PIN_BIT_3 && PIN_debounce[3]<PIN_DEB) PIN_debounce[3]++;//ADC7 JTAG TDI !!!
    if(PIN_BIT_3 && PIN_debounce[3]) PIN_debounce[3]--;

    if(!PIN_BIT_4 && PIN_debounce[4]<PIN_DEB) PIN_debounce[4]++;//ADC6 JTAG TDO !!!
    if(PIN_BIT_4 && PIN_debounce[4]) PIN_debounce[4]--;

    if(!PIN_BIT_5 && PIN_debounce[5]<PIN_DEB) PIN_debounce[5]++;//ADC5 JTAG TMS !!!
    if(PIN_BIT_5 && PIN_debounce[5]) PIN_debounce[5]--;

    if(!PIN_BIT_6 && PIN_debounce[6]<PIN_DEB) PIN_debounce[6]++;//PF4 PE2 PE3 ANALOG COMP IRQ /INPUT CAPTURE
    if(PIN_BIT_6 && PIN_debounce[6]) PIN_debounce[6]--;

    if(!PIN_BIT_7 && PIN_debounce[7]<PIN_DEB) PIN_debounce[7]++;//PF3 PE5 INT5
    if(PIN_BIT_7 && PIN_debounce[7]) PIN_debounce[7]--;

    for(char i=0;i<8;i++) {
        usRegDiscBuf[i]=(PIN_debounce[i] > PIN_DEB/2);
        //PIN_output(usRegCoilsBuf[i],i);
        if(usRegCoilsBuf[i]!=BIT(coils,i)){
            SET(coils,i,usRegCoilsBuf[i]);
            switch(i){
            case 0:PIN_SET_0(usRegCoilsBuf[0]);break;
            case 1:PIN_SET_1(usRegCoilsBuf[1]);break;
            case 2:PIN_SET_2(usRegCoilsBuf[2]);break;
            case 3:PIN_SET_3(usRegCoilsBuf[3]);break;
            case 4:PIN_SET_4(usRegCoilsBuf[4]);break;
            case 5:PIN_SET_5(usRegCoilsBuf[5]);break;
            case 6:PIN_SET_6(usRegCoilsBuf[6]);break;
            case 7:PIN_SET_7(usRegCoilsBuf[7]);break;
            }
        }
        //4 дискр.выход PWM 281Гц 16bit // 4 и 6 usRegHoldingBuf модуляция одного шима другим.
        if(usRegHoldingBuf[4]!=holding[0]){
            holding[0]=usRegHoldingBuf[4];
            OCR1C=usRegHoldingBuf[4];
            TCCR1A|=(1<<COM1C0)|(1<<COM1C1);
        }
        //5 дискр.выход PWM 281Гц 16bit
        if(usRegHoldingBuf[5]!=holding[1]){
            holding[1]=usRegHoldingBuf[5];
            OCR1B=usRegHoldingBuf[5];
            TCCR1A|=(1<<COM1B0)|(1<<COM1B1);
        }
        //4 дискр.выход PWM 72кГц 8bit
        if(usRegHoldingBuf[6]!=holding[2]){
            holding[2]=usRegHoldingBuf[6];
            OCR2=usRegHoldingBuf[6];
            TCCR2|=(1<<COM20)|(PIN_PWM_0<<COM21);
           // COM21=0 - CTC COM21=1 PWM
        }
        //5 дискр.выход PWM 72кГц 8bit
        if(usRegHoldingBuf[7]!=holding[3]){
            holding[3]=usRegHoldingBuf[7];
            OCR0=usRegHoldingBuf[7];
            SET(DDRB,4,1);SET(DDRB,6,0);
            TCCR0|=(1<<COM00)|(PIN_PWM_1<<COM01);
           // COM01=0 - CTC COM01=1 PWM
        }
    }
}



unsigned int PIN_input(char chan)
{
int c;
switch(chan){
    case 0: c=(PIN_debounce[0] > PIN_DEB/2);
  break;
    case 1: c=(PIN_debounce[1] > PIN_DEB/2);
  break;
    case 2: c=(PIN_debounce[2] > PIN_DEB/2);
  break;
    case 3: c=(PIN_debounce[3] > PIN_DEB/2);
  break;
    case 4: c=(PIN_debounce[4] > PIN_DEB/2);
  break;
    case 5: c=(PIN_debounce[5] > PIN_DEB/2);
  break;
    case 6: c=(PIN_debounce[6] > PIN_DEB/2);
  break;
    case 7: c=(PIN_debounce[7] > PIN_DEB/2);
  break;
    default: c=0;
  }
return c;
}

void PIN_output(unsigned int var, unsigned char chan)
{
 switch(chan){
    case 0:  PIN_SET_0(var);  break;
    case 1:  PIN_SET_1(var);  break;
    case 2:  PIN_SET_2(var);  break;
    case 3:  PIN_SET_3(var);  break;
    case 4:  PIN_SET_4(var);//SET(PORTB,7,!var);//OC2/OC1C PORTG3 PORTG4
     TCCR1A&=~(1<<COM1C0)|(1<<COM1C1);
     TCCR2&=~ (1<<COM20)|(1<<COM21);
  break;
    case 5:  PIN_SET_5(var);//SET(PORTB,6,!var);//PORTB4 B5 B6 OC0 OC1A OC1B
    TCCR1A&=~(1<<COM1B0)|(1<<COM1B1);
    TCCR0&=~(1<<COM00)|(1<<COM01);
  break;
    case 6:  PIN_SET_6(var);  break;
    case 7:  PIN_SET_7(var);  break;
 case ANA8://4 канал PWM 281Гц 16bit // 8 и 10 канал модуляция одного шима другим.
      OCR1C=var;
      TCCR1A|=(1<<COM1C0)|(1<<COM1C1);
break;
 case ANA9://5 канал PWM 281Гц 16bit
      OCR1B=var;
      TCCR1A|=(1<<COM1B0)|(1<<COM1B1);
break;
 case ANA10://4 канал PWM 72кГц 8bit
      OCR2=var;
      TCCR2|=(1<<COM20)|(1<<COM21);
     // COM21=0 - CTC COM21=1 PWM
break;
 case ANA11://5 канал PWM 72кГц 8bit
      OCR0=var;
      SET(DDRB,4,1);SET(DDRB,6,0);
      TCCR0|=(1<<COM00)|(1<<COM01);
break;
   }
}

void led(char L)
{
#ifdef HW0305
if(L==GREEN){
   PORTD|=(1<<6);
   PORTD&=~(1<<5);
  }
else if(L==RED){
   PORTD&=~(1<<6);
   PORTD|=(1<<5);
  }
else if(L==GREEN_TOGGLE){
   PORTD&=~(1<<5);
   SET(PORTD,6,(time65 & 1));
   //PORTD^=(1<<6);
  }
else if(L==RED_TOGGLE){
   PORTD&=~(1<<6);
   SET(PORTD,5,(time65 & 1));
   //PORTD^=(1<<6);
  }
else if(L==YELL){
   PORTD|=(1<<5);
   PORTD|=(1<<6);
  }
#endif
}

void alt_led(unsigned char c)
{
#ifdef ALT_LED
if (c) led(GREEN); else led(RED);

#endif
}

