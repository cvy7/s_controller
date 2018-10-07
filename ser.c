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
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/twi.h>
#include <stdio.h>
#include <string.h>
#include "conv_ser_interf.h"
#include "defs.h"
#include "port/port.h"

#define TX_BUFFER_SIZE1 64
#define RX_BUFFER_SIZE1 64

unsigned char tx_rd_index1=0;
unsigned char tx_wr_index1=0;
unsigned char tx_counter1=0;

unsigned char rx_rd_index1=0;
unsigned char rx_wr_index1=0;
unsigned char rx_counter1=0;

unsigned char tx_buffer1[TX_BUFFER_SIZE1];
unsigned char rx_buffer1[RX_BUFFER_SIZE1];

char SER_rts_time;

SIGNAL(INT4_vect)// прерывание по rxd 232 по спаду
{
RTS_HIGH;
SER_rts_time=2;
}

SIGNAL(USART1_RX_vect)
{
    unsigned char status = UCSR1A;
    unsigned char data= UDR1;
    if((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN)) == 0)
      {
        //CONV_rx(data);//парсинг ППЧ (ППЧ- ППЧ)
        //UDR0 = data;//сквозная отправка на комп (ППЧ- modbus)
        rx_buffer1[rx_wr_index1] = data;
        if(++rx_wr_index1 == RX_BUFFER_SIZE1)
            rx_wr_index1 = 0;
        ++rx_counter1;
    }
}

SIGNAL(USART1_TX_vect) // В случае ППЧ- modbus- не вызывается
{
if(tx_counter1)
      {
      --tx_counter1;
      UDR1 = tx_buffer1[tx_rd_index1];
      if(++tx_rd_index1 == TX_BUFFER_SIZE1)
           tx_rd_index1 = 0;
      }
else
    UCSR1B&=~(1<<TXCIE1);
}

int SER_Getchar(void){
    char data;

    if(rx_counter1)
          {
          cli();

          --rx_counter1;
          data = rx_buffer1[rx_rd_index1];
          if(++rx_rd_index1 == RX_BUFFER_SIZE1)
               rx_rd_index1 = 0;

          sei();
          return data;
          }
    else return -1;
}

void SER_Putchar( const char c, char port){ // В случае ППЧ- modbus- не вызывается

    if(tx_counter1 || ((UCSR1A & DATA_REGISTER_EMPTY) == 0))
          {
            cli();
            tx_buffer1[tx_wr_index1] = c;
            if(++tx_wr_index1 == TX_BUFFER_SIZE1)
                tx_wr_index1 = 0;
            ++tx_counter1;
            sei();
          }
        else {
              UDR1 = c;
              UCSR1B|=(1<<TXCIE1);
              }

}

void SER_poll(){

if(SER_rts_time){
        SER_rts_time--;
        if(!SER_rts_time) RTS_LOW;
    }
}

void SER_Print( const char  * pstr, char port) // В случае ППЧ- modbus- не вызывается
{
char *p=pstr;
for(char i=0; *p!=0;p++)
    {
     SER_Putchar(*p,port);
    }
}
