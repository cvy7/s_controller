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
#include "defs.h"
#include "i2c.h"
#include "mb_regs.h"
#include "config.h"
//********************************************************************************************************
//Инициализация аппаратной шины I2C
char i2c_state, device_number;
char ads1114_cfg=100;
int i2c_time;

//extern  int adc_in [9];
//extern  int dac_out[9];


int dac_out_nc[4];
int adc_in_nc[4];

inline void i2c_start_1(void);
inline void i2c_stop(void);

void i2c_init(void)
{
    TWSR=0;
    TWDR=0;
    TWAR=0;
    TWBR = TWI_TWBR;

    if      (TWSR == 0)         i2c_stop();// error recovery
    else if (i2c_state++ == 0)  TWCR=0;
    else                       {i2c_state=0; i2c_start_1();}
}

inline void test(char tst){
    SET(PORTD,4,!BIT(tst,0));
    SET(PORTF,2,!BIT(tst,1));
    SET(PORTD,6,!BIT(tst,2));
    SET(PORTD,7,!BIT(tst,3));
    SET(PORTB,7,!BIT(tst,4));
    SET(PORTB,6,!BIT(tst,5));
    SET(PORTG,0,!BIT(tst,6));
    SET(PORTG,1,!BIT(tst,7));
}

inline void i2c_start_1(void){
   TWCR=(1<<TWEN)|(1<<TWINT)|(1<<TWSTA)|(1<<TWIE);
}

inline void i2c_start_2(void){
   if((TWSR != START)&&(TWSR != REP_START))
      i2c_state=0xff;
}

inline void i2c_stop(void)
{
  TWCR = (1<<TWEN)|(1<<TWINT)|(1<<TWSTO)|(1<<TWIE);
}


inline void i2c_tx_1(uint8_t data)
{
  TWDR = data;
  TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWIE);
}

inline void i2c_tx_2(void){
    if(TWSR != MTX_DATA_ACK)
        i2c_state=0xff;
}

inline void i2c_tx_addr_1(uint8_t adr){
    TWDR = adr;
    TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWIE);
}

inline void i2c_tx_addr_2(void){
    if((TWSR != MTX_ADR_ACK)&&(TWSR != MRX_ADR_ACK))
      i2c_state=0xff;
}

inline void i2c_rx_1(void){
    TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWIE);
}

inline void i2c_rx_1_last(void){
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA)|(1<<TWIE);
}

inline char i2c_rx_2(void){
    if(TWSR != MRX_DATA_ACK)
      i2c_state=0xff;
    return TWDR;
}

inline char i2c_rx_2_last(void){
    if (TWSR != MRX_DATA_NACK)
      i2c_state=0xff;
    return TWDR;
}

inline void i2c_tx16(char addr, int data){
    i2c_state++;
    switch (i2c_state) {
    case 1: i2c_start_2();      break;
    case 2: i2c_tx_addr_2();    break;
    case 3: i2c_tx_2();         break;
    case 4: i2c_tx_2();         break;
    //default://error
    }

    switch (i2c_state) {
    case 1: i2c_tx_addr_1(addr);break;
    case 2: i2c_tx_1(data>>8);  break;
    case 3: i2c_tx_1(data);     break;
    case 4: //ok
    default://error
        i2c_stop();
    }
}

#define ADS1114_OS 15
//1 : Start a single conversion (when in power-down state)
#define ADS1114_MUX2 14
#define ADS1114_MUX1 13
#define ADS1114_MUX0 12
//000 :AIN P=AIN0 AIN N = AIN1 (default)
#define ADS1114_PGA2 11
#define ADS1114_PGA1 10
#define ADS1114_PGA0 9
//000 :FSR = ±6.144V
//001 :FSR = ±4.096V
//010 :FSR = ±2.048V
//011 :FSR = ±1.024V
//100 :FSR = ±0.512V
//101 :FSR = ±0.256V
//110 :FSR = ±0.256V
//111 :FSR = ±0.256V
#define ADS1114_MODE 8
//0 : Continuous-conversion mode
//1 : Single-shot mode or power-down state (default)
#define ADS1114_DR2 7
#define ADS1114_DR1 6
#define ADS1114_DR0 5
//000 :8 SPS
//001 :16 SPS
//010 :32 SPS
//011 :64 SPS
//100 :128 SPS (default)
//101 :250 SPS
//110 :475 SPS
//111 :860 SPS

inline void i2c_ads1114_cfg(char addr){
    i2c_state++;
    int ads1114_config=(1<<ADS1114_OS)|(1<<ADS1114_PGA1)|(1<<ADS1114_DR2)|(1<<ADS1114_DR0);

    switch (i2c_state) {
    case 1: i2c_start_2();      break;
    case 2: i2c_tx_addr_2();    break;
    case 3: i2c_tx_2();         break;
    case 4: i2c_tx_2();         break;
    case 5: i2c_tx_2();         break;
    case 6: i2c_start_2();      break;
    case 7: i2c_tx_addr_2();    break;
    case 8: i2c_tx_2();         break;
    //default://error
    }

    switch (i2c_state) {
    case 1: i2c_tx_addr_1(addr);        break;
    case 2: i2c_tx_1(1);                break;//addres p 2 config
    case 3: i2c_tx_1(ads1114_config>>8);break;
    case 4: i2c_tx_1(ads1114_config);   break;
    case 5: i2c_start_1();              break;
    case 6: i2c_tx_addr_1(addr);        break;
    case 7: i2c_tx_1(0);                break;//addres p 2 conversion
    case 8: //ok
    default://error
        i2c_stop();
    }
}

inline void i2c_ads1114(char addr,int *data){
    i2c_state++;
    switch (i2c_state) {
    case 1: i2c_start_2();              break;
    case 2: i2c_tx_addr_2();            break;
    case 3: *data =i2c_rx_2();
            *data<<=8;                  break;
    case 4: *data|=i2c_rx_2();          break;
    default:*data =0;//error
    }

    switch (i2c_state) {
    case 1: i2c_tx_addr_1(addr | 1);break;
    case 2: i2c_rx_1_last();        break;
    case 3: i2c_rx_1_last();        break;
    case 4: //ok
    default://error
        i2c_stop();
    }
}

//*****************************************************
SIGNAL(TWI_vect) {

//test(TWSR);//-----------------------------------------

    switch (device_number) {

    default:
        i2c_tx16(0x98  ,dac_out_nc[0]);//dac7571-ch.0
        break;

    case 1:
        i2c_tx16(0x98|2,dac_out_nc[1]);//dac7571-ch.1
         break;

    case 2:
        i2c_tx16(0xc0  ,dac_out_nc[2]);//mcp4725-ch.2
        break;

    case 3:
        i2c_tx16(0xc0|2,dac_out_nc[3]);//mcp4725-ch.3
         break;

    case 4:
        if(ads1114_cfg) i2c_ads1114_cfg(0x90);
        else            i2c_ads1114(0x90  ,&adc_in_nc[0]);//ads1114-ch.0
         break;

    case 5:
        if(ads1114_cfg) i2c_ads1114_cfg(0x92);
        else            i2c_ads1114(0x92,&adc_in_nc[1]);//ads1114-ch.1
         break;

    case 6:
        if(ads1114_cfg) i2c_ads1114_cfg(0x94);
        else            i2c_ads1114(0x94,&adc_in_nc[2]);//ads1114-ch.2
         break;

    case 7:
        if(ads1114_cfg) i2c_ads1114_cfg(0x96);
        else            i2c_ads1114(0x96,&adc_in_nc[3]);//ads1114-ch.3

         break;
    }
}



void i2c_poll(void){

    long dac_p;
//    test(TWSR);//-----------------------------------------
    if(ads1114_cfg)  ads1114_cfg--;
    else
    switch (device_number) {

    case 4:
    dac_p=(adc_in_nc[0]-ICH0_S)*ICH0_K;
    dac_p>>=14;
    if(dac_p > 32767) dac_p= 32767;
    if(dac_p <-32768) dac_p=-32768;
    usRegInputBuf[0]/*adc_in[0]*/=dac_p;//ads1114-ch.0
        break;

    case 5:
    dac_p=(adc_in_nc[1]-ICH1_S)*ICH1_K;
    dac_p>>=14;
    if(dac_p > 32767) dac_p= 32767;
    if(dac_p <-32768) dac_p=-32768;
    usRegInputBuf[1]/*adc_in[1]*/=dac_p;//ads1114-ch.1
        break;

    case 6:
    dac_p=(adc_in_nc[2]-ICH2_S)*ICH2_K;
    dac_p>>=14;
    if(dac_p > 32767) dac_p= 32767;
    if(dac_p <-32768) dac_p=-32768;
    usRegInputBuf[2]/*adc_in[2]*/=dac_p;//ads1114-ch2
        break;

    case 7:
    dac_p=(adc_in_nc[3]-ICH3_S)*ICH3_K;
    dac_p>>=14;
    if(dac_p > 32767) dac_p= 32767;
    if(dac_p <-32768) dac_p=-32768;
    usRegInputBuf[3]/*adc_in[3]*/=dac_p;//ads1114-ch3
        break;
    }

    device_number++;
    if(device_number>5) device_number=0;
    //device_number=4;
    //ads1114_cfg=100;

    switch (device_number) {

    case 0:
        dac_p=(int)usRegHoldingBuf[0]/*dac_out[0]*/*CH0_K;
        dac_out_nc[0]=(dac_p>>16) + CH0_S;//dac7571-ch.0
        if(dac_out_nc[0]>4095) dac_out_nc[0]=4095;
        if(dac_out_nc[0]<0) dac_out_nc[0]=0;
            break;

    case 1:
        dac_p=(int)usRegHoldingBuf[1]/*dac_out[1]*/*CH1_K;
        dac_out_nc[1]=(dac_p>>16) + CH1_S;//dac7571-ch.1
        if(dac_out_nc[1]>4095) dac_out_nc[1]=4095;
        if(dac_out_nc[1]<0) dac_out_nc[1]=0;
            break;

    case 2:
        dac_p=(int)usRegHoldingBuf[2]/*dac_out[2]*/*CH2_K;
        dac_out_nc[2]=(dac_p>>16) + CH2_S;//mcp4725-ch.2
        if(dac_out_nc[2]>4095) dac_out_nc[2]=4095;
        if(dac_out_nc[2]<0) dac_out_nc[2]=0;
            break;

    case 3:
        dac_p=(int)usRegHoldingBuf[3]/*dac_out[3]*/*CH3_K;
        dac_out_nc[3]=(dac_p>>16) + CH3_S;//mcp4725-ch.3
        if(dac_out_nc[3]>4095) dac_out_nc[3]=4095;
        if(dac_out_nc[3]<0) dac_out_nc[3]=0;
            break;

    default:
         break;
    }
    i2c_init();
}

