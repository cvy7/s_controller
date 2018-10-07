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
// ППЧ
// STX,(адрес устр-ва- ППЧ=0x20)),(0x20+длина пакета), base64((тип запрашиваемого параметра0 (>127))..(тип запрашиваемого параметра n (>127))
//     (тип устанавливаемого параметра0 < 128), (параметр 0)(m)..(тип устанавливаемого параметра n < 128), (параметр n)(m), (crc8)),EOT
// ППЧ передает
// STX,ADDR_PLA(0x21),(0x20+12),base64(T_POWER,мощность(4),T_STATUS,статус(1),crc8),EOT
// Плата передает ППЧ
// STX,ADDR_PPCH(0x20),(0x20+16),base64((T_POWER+128),(T_STATUS+128),T_POWER,задание(4),T_STATUS,команды(1), crc8(с начала параметров base64)),EOT
// В 64 только-только укладывается полная посылка ППЧ->Плата!
//****************************************************************************

//#define PPCH
//#define TPCH
//#define EMUL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include "conv.h"
//#include "mk_pl_bus.h"
#include "svzw.h"
#include "ser.h"
#include "defs.h"
#include "config.h"

//таймаут связи преобразователь-платка, в циклах передачи
#define CONV_TIMEOUT 200
#define RX_BUFFER_SIZE0 64
//proto
#define ADDR_PPCH (0x20)
#define ADDR_PLA  (0x21)

#define MAX_PARS 16

unsigned char conv_timeout;
unsigned char conv_start_tx=0;
unsigned char conv_num_byte=0;
unsigned char conv_receiving=0;
unsigned char conv_err_receive=0;
unsigned char conv_is_received=0;
unsigned char pack_len=RX_BUFFER_SIZE0;;
//unsigned char conv_received[RX_BUFFER_SIZE0];
//unsigned char rxppch_buff[RX_BUFFER_SIZE0];
unsigned char encode_buff[RX_BUFFER_SIZE0-2];
unsigned char decode_buff[((RX_BUFFER_SIZE0-2)*3)/4];
unsigned char decode_len,encode_len;
//unsigned char tx_phrase[MAX_PARS]={T_POWER,T_STATUS,T_ID,T_IO,T_UO,T_FO,T_ERR_CODE,T_TEMPERAT,0};
//60 из 62 с этими параметрами
//пакет ровно 64 байта

#ifdef PPCH
#define STX 2
#define PACK_LEN 4
#define EOT 4
static const unsigned char PROGMEM dscrc_table[]  = {
0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
157,195, 33,127,252,162, 64, 30, 95, 1,227,189, 62, 96,130,220,
35,125,159,193, 66, 28,254,160,225,191, 93, 3,128,222, 60, 98,
190,224, 2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89, 7,
219,133,103, 57,186,228, 6, 88, 25, 71,165,251,120, 38,196,154,
101, 59,217,135, 4, 90,184,230,167,249, 27, 69,198,152,122, 36,
248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91, 5,231,185,
140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
202,148,118, 40,171,245, 23, 73, 8, 86,180,234,105, 55,213,139,
87, 9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};
/**********************************************************************************************************/
//BASE64 http://ru.wikipedia.org/wiki/Base64
static  char encodeblock( unsigned char* in, unsigned char* out, int len )//inline
{
    unsigned char i=0,j=0;
    for(j=0;j<len;j+=3){
        out[0+i] =0x20 + (( in[0+j] >> 2                        ) & 0x3F);
        out[1+i] =0x20 + (((in[0+j] << 4) | ((in[1+j] >> 4) & 0xF)) & 0x3F);
        out[2+i] =0x20 + (((in[1+j] << 2) | ((in[2+j] >> 6) & 0x3)) & 0x3F);
        out[3+i] =0x20 + (( in[2+j]                             ) & 0x3F);
        i+=4;
    }
   return i;
}

static  char decodeblock( unsigned char* in, unsigned char* out, int len )//inline
{
    unsigned char i=0,j=0;
    for(j=0;j<len;j+=4){
        out[ 0+i ]=(( in[0+j]-' ')<<2  ) + (((in[1+j]-' ')&0x30)>>4);
        out[ 1+i ]=(((in[1+j]-' ')&0x0f)<<4) + (((in[2+j]-' ')&0x3c)>>2);
        out[ 2+i ]=(((in[2+j]-' ')&0x03)<<6) + ((in[3+j]-' ')&0x3f);
        i+=3;
    }
return i;
}

unsigned char crc8(unsigned char* in, unsigned char len)
{
    unsigned char crc=0,i=0;
    for(i=0;i<len;i++)
        crc = pgm_read_byte(&dscrc_table[crc^in[i]]);
        //crc^=in[i];
    return crc;
}

unsigned char is_crc(){

return (crc8(decode_buff,decode_len)==0);
return 1;
}

void CONV_rx(char data){
if(conv_receiving){

        if(conv_num_byte>(pack_len+1)){
           conv_num_byte=0;
           conv_receiving=0;
           if(data==EOT){
           conv_is_received=1;
           //memcpy(rxppch_buff,conv_received,pack_len);
           }
           else
           conv_err_receive=1;

           conv_num_byte=0;
           conv_receiving=0;
        }
        else {

              if((data!=ADDR_PLA) && (conv_num_byte==0)) conv_receiving=0;
              else {
                   if(conv_num_byte==1) {
                          pack_len=data-0x20;
                          if(pack_len>(RX_BUFFER_SIZE0)) {
                                conv_err_receive=1;
                                conv_receiving=0;
                            }
                       }
                       else if(conv_num_byte>1)  /*conv_received*/encode_buff[(conv_num_byte-2)]=data;
              }
              conv_num_byte++;

    }
}
else if(data==STX) {
               conv_receiving=1;
               conv_num_byte=0;
               conv_err_receive=0;
               pack_len=RX_BUFFER_SIZE0;
               //conv_timeout=0;
               }
}

void conv_rxp(){
    static char c_ready=0;
    static char c_heat_on=0;
    tstatus ppch_status;

    decode_len=decodeblock(encode_buff,decode_buff,pack_len);
    if(is_crc() && !conv_err_receive){
            char tx_index=0;
            conv_timeout=0;
            conv.conn_err=0;

            for(unsigned char i=0;i<(decode_len-1);){
                switch(decode_buff[i]){

                case T_POWER:
                            i++;
                            memcpy(&conv.P, &decode_buff[i],sizeof(conv.P));
                            i+=sizeof(tpower);
                            break;
                case T_UO:
                            i++;
                            memcpy(&conv.uou, &decode_buff[i],sizeof(conv.uou));
                            i+=sizeof(tuo);
                            break;
                case T_FO:
                            i++;
                            memcpy(&conv.fou, &decode_buff[i],sizeof(conv.fou));
                            i+=sizeof(tfo);
                            break;
                case T_IO:
                            i++;
                            memcpy(&conv.iou, &decode_buff[i],sizeof(conv.iou));
                            i+=sizeof(tio);
                            break;
                case T_ID:
                            i++;
                            memcpy(&conv.id, &decode_buff[i],sizeof(conv.id));
                            i+=sizeof(tid);
                            break;
                case T_UD:
                            i++;
                            memcpy(&conv.ud, &decode_buff[i],sizeof(conv.ud));
                            i+=sizeof(tud);
                            break;
                case T_STATUS:
                            i++;
                            memcpy(&ppch_status,&decode_buff[i],sizeof(ppch_status));
                            i+=sizeof(ppch_status);

                            conv.err=ppch_status.err;
 //                           conv.bloked=ppch_status.bloked;
                            conv.reg_io=ppch_status.reg_io;
                            conv.heat_on=ppch_status.heat_on;
                            conv.ready=ppch_status.ready;
//                            conv.so_on=ppch_status.water_norm;
                            conv.reg_uo=ppch_status.reg_uo;
                            conv.reg_not_in_limit=ppch_status.reg_in_limit;
                            conv.switch_on=ppch_status.charge;
                            conv.mestnoe_upr=ppch_status.mestnoe_upr;

                            CONV_state=CONV_OFF;
                            if(conv.switch_on) CONV_state=CONV_CHARGE;
                            if(conv.ready)  CONV_state=CONV_READY;
                            if(conv.heat_on) CONV_state=CONV_HEAT_ON;
                            if(conv.err) CONV_state=CONV_ERR_ON;

                            break;
                case T_ERR_CODE:
                            i++;
                            memcpy(&conv.err_cod[0], &decode_buff[i],sizeof(conv.err_cod));
                            i+=sizeof(terr_code);
                            for(char pp=0;pp<ERR_MAX;pp++) if(conv.err_cod[pp]>999) conv.err_cod[pp]=0;
                            break;
                case T_TEMPERAT:
                            i++;
                            memcpy(&conv.t_water, &decode_buff[i],sizeof(ttemperat));
                            i+=sizeof(ttemperat);
                            break;
                default: return;// неизвестный этой платке тип данных
                    //дальнейший парсинг невозможен тк неизвестна его длина

                }
            }
    }
}

void conv_txp(){
    tstatus ppch_command;
    tpower ppch_txpower;
    tuo ppch_txuo;
    static char conv_txp_fr=0;
    /*tx_phrase[0]=T_POWER;
    tx_phrase[1]=T_STATUS;
    tx_phrase[2]=T_ID;
    tx_phrase[3]=T_IO;
    tx_phrase[4]=T_UO;
    tx_phrase[5]=T_FO;
    tx_phrase[6]=T_ERR_CODE;
    tx_phrase[7]=T_TEMPERAT;
    tx_phrase[8]=0;*/


    ppch_command.heat_on = conv.command_heat_on/* && conv.ready*/;
    ppch_txpower.power=conv.ps;
    ppch_txuo.uo=conv.us;
   /* if(!conv.heat_on && !conv.command_heat_on) {*/
         ppch_command.ready  = conv.command_ready;
         if(conv_txp_fr<10){
                ppch_command.err=1;
                conv_txp_fr++;
         }
         else   ppch_command.err=conv.command_reset;
         //conv.command_reset=0;
   /*      }*/
    //*********
    unsigned char i=0;
    decode_buff[i++]=T_POWER;
    memcpy(&decode_buff[i],&ppch_txpower,sizeof(ppch_txpower));
    i+=sizeof(ppch_txpower);

    decode_buff[i++]=T_UO;
    memcpy(&decode_buff[i],&ppch_txuo,sizeof(ppch_txuo));
    i+=sizeof(ppch_txuo);

    decode_buff[i++]=T_STATUS;
    memcpy(&decode_buff[i],&ppch_command,sizeof(ppch_command));
    i+=sizeof(ppch_command);

    decode_buff[i++]=T_STATUS+128;
    decode_buff[i++]=T_POWER+128;
    //*****************************************
    //decode_buff[i++]=T_ID+128;
    decode_buff[i++]=T_UD+128;
    decode_buff[i++]=T_IO+128;
    decode_buff[i++]=T_UO+128;
    decode_buff[i++]=T_FO+128;

    decode_buff[i++]=T_ERR_CODE+128;
    decode_buff[i++]=T_TEMPERAT+128;
    //*****************************************
    for( ;i%3 != 2;) decode_buff[i++]=0;//добиваем 0-ми до кратности длины 3 с учетом срс
    decode_buff[i]=crc8(decode_buff,i);
    i++;
    encode_len=encodeblock(decode_buff,encode_buff,i);
    //encode_buff[0]=0x20;
    //********
    SER_Putchar(STX,1);
    SER_Putchar(ADDR_PPCH,1);
    SER_Putchar(0x20+encode_len,1);
    for(unsigned char i=0; i < encode_len; i ++) SER_Putchar(encode_buff[i],0);
    SER_Putchar(EOT,1);
}

#else
#ifdef TPCH
  #define PACK_LEN (sizeof(trxtpch)-1)
  #define PACK_LEN_N (sizeof(trxtpch_n)-1)
//proto
  #define STX 0x55
  #define EOT 0xff
unsigned int crc_r(unsigned char* p,char n)
{
  unsigned int crc=0xffff;
  for(char i=0;i<(n-2);i++){
    crc^=(*(p+i));
    for(char ii=0;ii<8;ii++){
      char flag=0;
      flag=crc&1;
      crc>>=1;
      if(flag)crc^=0xa001;
    };
  };
  return crc;
}

unsigned char is_crc(){

int crc=crc_r(encode_buff, sizeof(trxtpch));
//alt_led((crc==rxtpch.crc));
trxtpch *rxtpch=encode_buff;
return (crc==rxtpch->crc);

}

void CONV_rx(char data){
if(conv_receiving){

        if(conv_num_byte>PACK_LEN){
           conv_err_receive=1;
           conv_num_byte=0;
           conv_receiving=0;
        }
        else {
              encode_buff[conv_num_byte++ + 1]=data;
          if(conv_num_byte>PACK_LEN){
          conv_num_byte=0;
              conv_receiving=0;
              conv_is_received=1;
              //memcpy((&rxtpch.sinc+1),conv_received,(sizeof(rxtpch)-1));
              }

    }
}
else if(data==STX) {
               conv_receiving=1;
               conv_num_byte=0;
               conv_err_receive=0;
               pack_len=RX_BUFFER_SIZE0;
               //conv_timeout=0;
               }
}

conv_rxp(){

    trxtpch *rxtpch=encode_buff;
    rxtpch->sinc=STX;

    if( is_crc() && !conv_err_receive){

    conv_timeout=0;
    conv.conn_err=0;
    conv.heat_on=rxtpch->heat_on;
    conv.switch_on=rxtpch->switch_on;
    conv.err_so=rxtpch->err_so;
    conv.err_bk=rxtpch->err_bk;
    conv.mestnoe_upr=rxtpch->mestnoe_upr;
    //conv.so_on=conv.pusk_so;//rxtpch.so_on;
    conv.err=rxtpch->err;
    conv.bloked= (conv.err & !conv.heat_on & conv.command_heat_on)?1:0;
    conv.ready=!conv.err && rxtpch->switch_on && !rxtpch->mestnoe_upr;//######################
    //conv.ready=conv.command_ready;//################################
    conv.P=rxtpch->Pa*10;//############################
    //conv.P=conv.Zadanie;//##############################
    conv.fou=rxtpch->fi;//#########################
    //conv.fou=440000UL;//############################
    //static int test2;
    //conv.fou=test2++;
    conv.id= rxtpch->id;
    conv.iou= rxtpch->id;
    conv.uou= rxtpch->Ui;

    #ifdef NEW_PROTO
    conv.err_cod[0]=rxtpch->err_cod;//############################
    conv.err_cod[1]=rxtpch->err_cod;
    conv.err_cod[2]=rxtpch->err_cod;
    conv.t_max=rxtpch->t_max;
    conv.t_water=rxtpch->t_water;
    #else
    conv.err_cod[0]=0;//############################
    conv.err_cod[1]=0;
    conv.err_cod[2]=0;
    conv.t_max=0;
    conv.t_water=0;
    #endif

    }

}

void conv_txp(){

    ttxtpch *txtpch=encode_buff;
    txtpch->sinc=STX;
    txtpch->heat_on=conv.command_heat_on;// && conv.ready;
    txtpch->pusk_so=conv.command_ready;
    txtpch->zad=conv.ps/10;
    txtpch->reset=conv.command_reset;
    txtpch->reset_so=conv.command_reset;
    //conv.command_reset=0;
    txtpch->mestnoe_upr_p=0;
    txtpch->crc=crc_r(&txtpch->sinc, sizeof(ttxtpch));

    //alt_led(conv.command_heat_on);

    unsigned char *p=encode_buff;
    //SER_Putchar(0x55,1);
    for(unsigned char i=0; i < sizeof(ttxtpch); i ++) SER_Putchar(p[i],1);
    SER_Putchar(EOT,1);
    SER_Putchar(EOT,1);
    SER_Putchar(EOT,1);


}
#endif
#endif
//*********************************************************************************
void CONV_comm_poll(){
    int data;
    static char bootcntr=0;
    conv_timeout++;
    if((conv_timeout & 0x3f) == 0x3f) conv_start_tx=1;

    do{
    data=SER_Getchar();

    if(data<0) break;

    if(data==0x1b) bootcntr++;// Для загрузки от интерфейса преобразователя (если в нем  Raspberry)
        else bootcntr=0;      // нужно передать 32 'esc'-а
    CONV_rx(data);
    }
    while (data>=0);

    if(bootcntr==32){
    eeprom_update_byte (4095, 0xdf);
    for(;;);
    }

if(conv_is_received){
    conv_start_tx=1;
    conv_is_received=0;
    //if(conv.err_cod[0]==ERR_CONNECT) CONV_reset();
    conv_rxp();
}
else if(conv_timeout > CONV_TIMEOUT){
   if(!conv.mestnoe_upr && !conv.conn_err) {
        conv.conn_err=1;
        conv.err_cod[0]=ERR_CONNECT;
        CONV_err();
       }
   //else conv_timeout=0;
  }

if(conv.mestnoe_upr){
    conv_timeout=0;
    conv.conn_err=0;
}

#ifdef EMUL
    conv.heat_on=conv.command_heat_on;
    conv.ready=conv.command_ready;
    conv.err=conv.command_reset;
    conv.P=conv.ps;
    conv.id=conv.ps/5;
    conv.ud=513;
    conv.fou=8000;
    conv.so_on=conv.command_ready;
    conv.mestnoe_upr=0;
    conv_timeout=0;

    conv.t[0]=20;
    conv.t[1]=21;
    conv.t[2]=22;
    conv.t[3]=23;
    conv.t[4]=24;
    conv.t[5]=25;
    conv.t[6]=26;
    conv.t[7]=27;

    conv.t_water=15;
    conv.t_max=60;
    conv.rashod=40;

    conv.err_cod[0]=998;
    conv.err_cod[1]=222;
    conv.err_cod[2]=333;
#endif

if(conv_start_tx && !conv_receiving){
        conv_start_tx=0;
        //PMODBUS_rx();
        //PMODBUS_tx();
        conv_txp();
    }
}
//********************************************************************************
