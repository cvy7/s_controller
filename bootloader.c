/*в Makefile
 * patch Makefile boot.patch
*/
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
 *                                                                          *
 *     AVRUSBBoot - USB bootloader for Atmel AVR controllers                *
 *     Thomas Fischl <tfischl@gmx.de>                                       *
 ****************************************************************************/
//****************************************************************************/

//if 0
//точка входа- первая функция в секции
#include <avr/boot.h>
void BOOTLOADER_SECTION  boot();
void __attribute__ ((section (".bootloader_start"))) __attribute__((noreturn)) boot_entry(){
boot();
}
//******************************************************************************
#include <avr/io.h>
//#include <avr/signal.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/crc16.h>
#include "config.h"

#define BOOT_ADDR              101

#define BOOT_STATE_IDLE       1
#define BOOT_STATE_WRITE_PAGE 2
#define BOOT_STATE_F          3
#define BOOT_STATE_ADR_MSB    4
#define BOOT_STATE_ADR_LSB    5
#define BOOT_STATE_NREG_MSB   6
#define BOOT_STATE_NREG_LSB   7
#define BOOT_STATE_NBYTES     8
#define BOOT_STATE_DATA_MSB   9
#define BOOT_STATE_DATA_LSB   10
#define BOOT_STATE_CRC_MSB    11
#define BOOT_STATE_CRC_LSB    12

#define BOOT_FUNC_WRITE_PAGE 2
#define BOOT_FUNC_LEAVE_BOOT 1
#define BOOT_FUNC_GET_PAGESIZE 3
/*
Датаграмма модифицированный Modbus RTU
Запрос для основной программы
10   адрес
0x06 функция запись  регистра
00   старший байт адреса
0xff младший байт адреса
0xde старший байт
0xad младший байт
     CRC старший байт
     CRC младший байт
********************************
отвечает основная программа повтором,
или ошибкой
10   адрес
>127 ошибка
код ошибки
CRC
CRC

если нет ошибки переход на бутлоадер
с записью в eeprom по адресу 4095 0xde

иначе загрузчик отваливается
******************************************
Запрос
100  адрес
0x10 функция запись нескольких регистров
XX   старший байт адреса (номер страницы)
XX   младший байт адреса (номер страницы)
0x00 количество реш=гистров старший байт
0x10 кол-во регистров младший байт
00   кол-во байт далее- формально - правильно, байт 256, но не соответствует спецификации modbus
     старший байт буфера[0]
     младший байт буфера[0]
--------------------------
     старший байт буфера[255]
     младший байт буфера[255]
     CRC старший байт
     CRC младший байт
********************************************
пауза 1,75mc (2mc)
Ответ
100  адрес
0x10 функция запись нескольких регистров
XX   старший байт адреса (номер страницы)
XX   младший байт адреса (номер страницы)
0x01 количество реш=гистров старший байт
0x00 кол-во регистров младший байт
 CRC старший байт
 CRC младший байт
*/
typedef struct {//на место основных переменных
char state;
char port;
unsigned char ucRTUBuf[2048];
} tbootvars;

tbootvars * const bootvars=0x100;

void BOOTLOADER_SECTION boot_program_page (uint32_t page, uint8_t *buf);
void BOOTLOADER_SECTION leaveBootloader();
int  BOOTLOADER_SECTION boot_mb_read();
void BOOTLOADER_SECTION boot_eeprom_update_byte (uint8_t *__p, uint8_t __value);
char BOOTLOADER_SECTION boot_uart0_end_of_write();
uint8_t BOOTLOADER_SECTION  boot_eeprom_read_byte (const uint8_t *__p);

//****************************************************************************************************
#if 0
static const __attribute__((__progmem__)) char aucCRCHi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1,
    0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80,
    0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00,
    0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81,
    0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1,
    0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81,
    0x40
};

static const __attribute__((__progmem__)) char aucCRCLo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
    0x05, 0xC5, 0xC4,
    0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB,
    0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE,
    0xDF, 0x1F, 0xDD,
    0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2,
    0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
    0x36, 0xF6, 0xF7,
    0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E,
    0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B,
    0x2A, 0xEA, 0xEE,
    0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27,
    0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
    0x63, 0xA3, 0xA2,
    0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD,
    0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8,
    0xB9, 0x79, 0xBB,
    0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4,
    0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
    0x50, 0x90, 0x91,
    0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94,
    0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59,
    0x58, 0x98, 0x88,
    0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D,
    0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
    0x41, 0x81, 0x80,
    0x40
};

int BOOTLOADER_SECTION
bootusMBCRC16( char * pucFrame, int usLen )
{
    char           ucCRCHi = 0xFF;
    char           ucCRCLo = 0xFF;
    int             iIndex;

    while( usLen-- )
    {
        iIndex = ucCRCLo ^ *( pucFrame++ );
        ucCRCLo = ucCRCHi ^ pgm_read_byte( &aucCRCHi[iIndex] );
        ucCRCHi = pgm_read_byte( &aucCRCLo[iIndex] );;
    }
    return ucCRCHi << 8 | ucCRCLo;
}
#endif
//****************************************************************************************************
void BOOTLOADER_SECTION  boot(){
    __asm__ volatile (
    "eor	r1, r1"
                );// ? gcc думает что в r1 всегда 0 ?
SPH=0x10;
SPL=0xff;
SREG=0;
wdt_enable(WDTO_1S);
 //leaveBootloader();

for(int i=0x100;i<0x1100;i++) *(char *)i=0;

char cond=boot_eeprom_read_byte (4095);
if(cond==0xde)      {
    //bootvars->port=0;

    UCSR0A=(1<<RXC0)|(1<<TXC0)|(1<<UDRE0);
    UCSR0B=(1<<TXEN0)|(1<<RXEN0);
    UCSR0C=(1<<UCSZ01)|(1<<UCSZ00)|(1<<USBS0);
    //UBRR1L=12;//38400 8n1 @8MHz
    //UBRR1L=8;//115200 8n1 @8MHz
    //UBRR0L=9;//115200 8n1 @18.432MHz
    UBRR0L=29;//38400 8n1 @18.432MHz
    DDRB=(1<<0);
    PORTB=(1<<0);

    //UCSR1A|=(1<<U2X1);
    UBRR0H=0;
    UDR0=BOOT_ADDR;//при каждой загрузке в бутлоадер (например по watchdog у) кидается своим адресом
    while (!(UCSR0A & (1<<TXC0)) && !(UCSR0A & (1<<UDRE0)));
    UCSR0A |= (1<<TXC0);
    PORTB&=~(1<<0);//DTX 485
    char c=UDR0;
}
else if (cond==0xdf){
    bootvars->port=1;

    UCSR1A=(1<<RXC1)|(1<<TXC1)|(1<<UDRE1);
    UCSR1B=(1<<TXEN1)|(1<<RXEN1);
    UCSR1C=(1<<UCSZ11)|(1<<UCSZ10)|(1<<USBS1);
    //UBRR1L=12;//38400 8n1 @8MHz
    //UBRR1L=8;//115200 8n1 @8MHz
#ifdef TPCH
    UBRR1L=119;//9600 8n2 @18.432MHz
#else
    UBRR1L=9;//115200 8n2 @18.432MHz
#endif
    //UCSR1A|=(1<<U2X1);
    UBRR1H=0;
    UDR1=BOOT_ADDR;//при каждой загрузке в бутлоадер (например по watchdog у) кидается своим адресом
}
else leaveBootloader();

DDRA=0;DDRB=1;DDRC=0;DDRD=0;DDRE=0;DDRG=0;

for(;;){
   bootvars->state = BOOT_STATE_IDLE;
   int r=boot_mb_read();

   uint32_t page =0;
            page =bootvars->ucRTUBuf[2];
            page<<=8;
            page+=bootvars->ucRTUBuf[3];
            page<<=8;

   if (r>=0) {  boot_program_page (page,  &bootvars->ucRTUBuf[7]);
               if(boot_program_verify (page,  &bootvars->ucRTUBuf[7])<0) r=-1;
             }

   boot_mb_write(r);

   if (r==1) leaveBootloader();
   wdt_reset();
   }
}

void BOOTLOADER_SECTION leaveBootloader() {
      void (*jump_to_app)(void)=0;
      cli();
      boot_rww_enable();
      //GICR = (1 << IVCE);  /* enable change of interrupt vectors */
      //GICR = (0 << IVSEL); /* move interrupts to application flash section */
      boot_eeprom_update_byte (4095, 1);
      jump_to_app();
      //__ctors_end();
}

int BOOTLOADER_SECTION boot_program_verify (uint32_t page, uint8_t *buf)
{   uint16_t i;
    uint32_t adr;
    for (i=0; i<SPM_PAGESIZE-1; i++)
    {
        adr=page +i;
        uint8_t c = pgm_read_byte( adr);
        if (c!=*(buf+i)) {
            boot_uart_write(0xf1);//boot_uart_write(i);
            return -1;
        }
    }
    return 0;
}

void BOOTLOADER_SECTION boot_program_page (uint32_t page, uint8_t *buf)
{
    uint16_t i;
    uint8_t sreg;

    // Disable interrupts.

    //sreg = SREG;
    cli();

    eeprom_busy_wait ();

    if(bootvars->state==BOOT_STATE_CRC_LSB) boot_page_erase (page);
    if(bootvars->state==BOOT_STATE_CRC_LSB) boot_spm_busy_wait ();      // Wait until the memory is erased.

    for (i=0; i<SPM_PAGESIZE; i+=2)
    {
        // Set up little-endian word.

        uint16_t w = *buf++;
        w += (*buf++) << 8;

        boot_page_fill (page + i, w);
    }

    if(bootvars->state==BOOT_STATE_CRC_LSB) boot_page_write (page);     // Store buffer in flash page.
    if(bootvars->state==BOOT_STATE_CRC_LSB) boot_spm_busy_wait();       // Wait until the memory is written.

    // Reenable RWW-section again. We need this if we want to jump back
    // to the application after bootloading.

    boot_rww_enable ();

    // Re-enable interrupts (if they were ever enabled).

   // SREG = sreg;
}


int BOOTLOADER_SECTION
bootusMBCRC16( char * pucFrame, int usLen )
{
    int crc=0xffff;
    while( usLen-- )
    {
        crc=_crc16_update(crc, *( pucFrame++ ));//inlined
    }
    return crc;
}

void BOOTLOADER_SECTION boot_uart_write(char c){
    if(!bootvars->port){
        while (!(UCSR0A & (1<<TXC0)) && !(UCSR0A & (1<<UDRE0)));
        UCSR0A |= (1<<TXC0);
        UDR0=c;//таймаут ожидания таким образом определяется watchdog ом
        DDRB|=(1<<0);//DTX 485
        PORTB|=(1<<0);//DTX 485
    }
    else {
        while (!(UCSR1A & (1<<TXC1)) && !(UCSR1A & (1<<UDRE1)));
        UCSR1A |= (1<<TXC1);
        UDR1=c;//таймаут ожидания таким образом определяется watchdog ом
    }
}

char BOOTLOADER_SECTION boot_uart0_end_of_write(){
    char c=0;
    if(!bootvars->port){
    while (!(UCSR0A & (1<<TXC0)) && !(UCSR0A & (1<<UDRE0)));
    UDR0=0xff;
    UCSR0A |= (1<<TXC0);
    while (!(UCSR0A & (1<<TXC0)) && !(UCSR0A & (1<<UDRE0)));
    UCSR0A |= (1<<TXC0);
    PORTB&=~(1<<0);//DTX 485
    c= UDR0;//flush uart
    c= UDR0;
    }
    return c;
}

char BOOTLOADER_SECTION boot_uart_read(){
    char c;
    for(;;){//таймаут ожидания таким образом определяется watchdog ом
        if(!bootvars->port){
            if(UCSR0A & (1<<RXC0)) {
                c= UDR0;   
                // boot_uart_write(c);
                // UDR0=c;
                return c;
            }
        }
        else{
            if(UCSR1A & (1<<RXC1)) {
                c= UDR1;
                 //boot_uart_write(c);
                 //UDR1=c;
                return c;
            }
        }  
    }
}

int  BOOTLOADER_SECTION boot_mb_read(){
  unsigned char r,*buf;
  for(;;){
   // switch (bootvars->state) {// создает таблицу во flash-> поэтому if-ы
        if(bootvars->state==BOOT_STATE_CRC_LSB){
            r=boot_uart_read();
            *buf=r;
            int crc=bootusMBCRC16( bootvars->ucRTUBuf, (256+7) );

            if(((crc>>8)&0xff)==*(buf) && (crc&0xff)==*(buf-1) /*bootusMBCRC16( bootvars->ucRTUBuf, (256+9) )*/) {
                if(!(bootvars->ucRTUBuf[6])) return 0;//если количество байт в посылке-256 ucRTUBuf[6]=0;- посылку пишем
                else                         return 1;//если какое то другое количество байт - это последний пакет, даже если он 256 байт
            }                                         //опсле записи покидаем бутлоадер - запись закончена

            boot_uart_write(0xf0);//boot_uart_write(crc>>8);boot_uart_write(crc);
            return -1;//err
        }

        if(bootvars->state==BOOT_STATE_CRC_MSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_CRC_LSB;
            continue;
        }

        if(bootvars->state==BOOT_STATE_DATA_LSB){
            r=boot_uart_read();
            *buf++=r;
            if((buf - bootvars->ucRTUBuf) > (256+6)) bootvars->state=BOOT_STATE_CRC_MSB;
            else                                     bootvars->state=BOOT_STATE_DATA_LSB;
            wdt_reset();
            continue;
        }

        if(bootvars->state==BOOT_STATE_DATA_MSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_DATA_LSB;
            continue;
        }

        if(bootvars->state==BOOT_STATE_NBYTES){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_DATA_MSB;
            continue;
        }

        if(bootvars->state== BOOT_STATE_NREG_LSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_NBYTES;
            continue;
        }

        if(bootvars->state==BOOT_STATE_NREG_MSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_NREG_LSB;
            continue;
        }

        if(bootvars->state==BOOT_STATE_ADR_LSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_NREG_MSB;
            continue;
        }

        if(bootvars->state==BOOT_STATE_ADR_MSB){
            r=boot_uart_read();
            *buf++=r;
            bootvars->state=BOOT_STATE_ADR_LSB;
            continue;
        }

        if(bootvars->state==BOOT_STATE_F){
            r=boot_uart_read();
            *buf++=r;
            if(r==0x10) bootvars->state=BOOT_STATE_ADR_MSB;//если 0x10 функция-запись нескольких регистров- принимаем их
            else        bootvars->state=BOOT_STATE_IDLE; //это ошибка синхронизации - ждем адреса
            continue;
        }

        //default:// BOOT_STATE_IDLE ждем адреса бутлодера
        buf=bootvars->ucRTUBuf;
        r=boot_uart_read();
        *buf++=r;
        if(r==BOOT_ADDR) bootvars->state=BOOT_STATE_F;
        wdt_reset();
    }
}


int  BOOTLOADER_SECTION boot_mb_write(int res){
  unsigned char *buf;
  int crc;
  /*пауза 1,75mc (2mc)
  Ответ
  100  адрес
  0x10 функция запись нескольких регистров
  XX   старший байт адреса (номер страницы)
  XX   младший байт адреса (номер страницы)
  0x01 количество реш=гистров старший байт
  0x00 кол-во регистров младший байт
   CRC старший байт
   CRC младший байт
  */
      if(res<0){ //err
         for(;;);// пока все с начала
      }
      else{
      buf=bootvars->ucRTUBuf;
      boot_uart_write(0);//3нуля с запасом на переключение репитеров
      boot_uart_write(0);
      boot_uart_write(0);
      boot_uart_write(0);
      boot_uart_write(0);
      boot_uart_write(*buf++);
      boot_uart_write(*buf++);
      boot_uart_write(*buf++);
      boot_uart_write(*buf++);
      boot_uart_write(*buf++);
      boot_uart_write(*buf++);
      crc=bootusMBCRC16( bootvars->ucRTUBuf,6);
      *buf=crc;
      boot_uart_write(*buf++);
      *buf=crc>>8;
      boot_uart_write(*buf++);
      boot_uart0_end_of_write();
      }
}

void BOOTLOADER_SECTION boot_eeprom_update_byte (uint8_t *__p, uint8_t __value){
//    0000420e <__eeupd_byte_m128>:
    __asm__ volatile (
                "mov	r18, r22" "\n\t"
                "sbic	0x1c, 1" "\n\t"
                "rjmp	.-4" "\n\t"
                "out	0x1f, r25" "\n\t"
                "out	0x1e, r24" "\n\t"
                "sbi	0x1c, 0" "\n\t"
                "sbiw	r24, 0x01" "\n\t"
                "in	r0, 0x1d" "\n\t"
                "cp	r0, r18" "\n\t"
                "breq	.+12" "\n\t"
                "out	0x1d, r18" "\n\t"
                "in	r0, 0x3f" "\n\t"
                "cli" "\n\t"
                "sbi	0x1c, 2" "\n\t"
                "sbi	0x1c, 1" "\n\t"
                "out	0x3f, r0" "\n\t"
                "ret" "\n\t"
 );
}

uint8_t BOOTLOADER_SECTION  boot_eeprom_read_byte (const uint8_t *__p){
/*0000420e <__eerd_byte_m128>:*/
    __asm__ volatile (
            "sbic	0x1c, 1" "\n\t"
            "rjmp	.-4" "\n\t"
            "out	0x1f, r25" "\n\t"
            "out	0x1e, r24" "\n\t"
            "sbi	0x1c, 0" "\n\t"
            "eor	r25, r25" "\n\t"
            "in	r24, 0x1d" "\n\t"
            "ret"
            );
}
//#endif
