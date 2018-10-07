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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>

#include "mb.h"
#include "mbport.h"
#include "conv.h"
#include "defs.h"

/* ----------------------- Defines ------------------------------------------*/
#define RTU
#define PMODBUS_TIMEOUT 250
// 0.8c

#define REG_INPUT_START         (1)
//(1000+1)
#define REG_INPUT_NREGS         (32)

#define REG_HOLDING_START       (1)
//(2000+1)
#define REG_HOLDING_NREGS       (32)

#define REG_COILS_START         (1)
//(3000+1)
#define REG_COILS_NREGS         (32)
#define REG_COILS_BYTES REG_COILS_NREGS
// /8

#define REG_DISC_START          (1)
//(4000+1)
#define REG_DISC_NREGS          (32)
#define REG_DISC_BYTES REG_DISC_NREGS
// /8

#define SLAVE_ID                (11)

/* ----------------------- Static variables ---------------------------------*/
static USHORT   usRegInputStart = REG_INPUT_START;
/*static*/ USHORT   usRegInputBuf[REG_INPUT_NREGS];//={0x0123,0x4567,0x89AB,0xCDEF};
static USHORT   usRegHoldingStart = REG_HOLDING_START;
/*static*/ USHORT   usRegHoldingBuf[REG_HOLDING_NREGS];//={0x0123,0x4567,0x89AB,0xCDEF,0xDEAD,0xBEEF,0xDEAD,0xBEEF,0xDEAD,0xBEEF};
static USHORT    usRegCoilsStart = REG_COILS_START;
/*static*/ UCHAR     usRegCoilsBuf[REG_COILS_BYTES];//={1,0,0,0,1};
static USHORT    usRegDiscStart = REG_DISC_START;
/*static*/ UCHAR     usRegDiscBuf[REG_DISC_BYTES];//={0,1,0,0,0,0,1};

unsigned char PMODBUS_timeout;
unsigned char booting=0;

/* ----------------------- Start implementation -----------------------------*/

void MB_WriteInput(int ir,int adr){
   usRegInputBuf[adr]=ir;
}

void MB_WriteHolding(int hr,int adr){
   usRegHoldingBuf[adr]=hr;
}

int  MB_ReadHolding(int adr){
   return usRegHoldingBuf[adr];
}

void MB_WriteCoils(char c,int adr){
   usRegCoilsBuf[adr]=c;
}

char  MB_ReadCoils(int adr){
   return usRegCoilsBuf[adr];
}

void MB_WriteDisc(char c,int adr){
   usRegDiscBuf[adr]=c;
}
//--------------------------------------------------------------------------
eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ = ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ = ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
        //PMODBUS_timeout=PMODBUS_TIMEOUT;
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if(usAddress==255 && usNRegs==1 && pucRegBuffer[0]==0xde && eMode==MB_REG_WRITE ) {
        eeprom_update_byte (4095, 0xde);/*for(;;);*/
        booting=0xde;
        return eStatus;
        } //bootloader trap

    if( ( usAddress >= REG_HOLDING_START ) &&
        ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegHoldingStart );
        switch ( eMode )
        {
            /* Pass current register values to the protocol stack. */
        case MB_REG_READ:
            while( usNRegs > 0 )
            {
                *pucRegBuffer++ = ( UCHAR ) ( usRegHoldingBuf[iRegIndex] >> 8 );
                *pucRegBuffer++ = ( UCHAR ) ( usRegHoldingBuf[iRegIndex] & 0xFF );
                iRegIndex++;
                usNRegs--;
            }
            break;

            /* Update current register values with new values from the
             * protocol stack. */
        case MB_REG_WRITE:
            while( usNRegs > 0 )
            {
                usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
                usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
                iRegIndex++;
                usNRegs--;
                PMODBUS_timeout=PMODBUS_TIMEOUT;
            }
        }
        //PMODBUS_timeout=PMODBUS_TIMEOUT;
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}



eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    if ( ( usAddress >= REG_COILS_START )
        && ( usAddress + usNCoils <= REG_COILS_START + REG_COILS_NREGS ) )
    {
        iRegIndex = ( int ) ( usAddress - usRegCoilsStart );
        //PMODBUS_timeout=PMODBUS_TIMEOUT;
        switch ( eMode )
        {
            case MB_REG_READ:
            {
                while ( usNCoils > 0 )
                {
                    UCHAR ucResult = usRegCoilsBuf[iRegIndex];//xMBUtilGetBits( usRegCoilsBuf, iRegIndex, 1 );

                    xMBUtilSetBits( pucRegBuffer, iRegIndex - ( usAddress - usRegCoilsStart ), 1, ucResult );

                    iRegIndex++;
                    usNCoils--;
                }

                break;
            }

            case MB_REG_WRITE:
            {
                while ( usNCoils > 0 )
                {
                    UCHAR ucResult = xMBUtilGetBits( pucRegBuffer, iRegIndex - ( usAddress - usRegCoilsStart ), 1 );

                    usRegCoilsBuf[iRegIndex]=ucResult; //xMBUtilSetBits( usRegCoilsBuf, iRegIndex, 1, ucResult );

                    iRegIndex++;
                    usNCoils--;
                    PMODBUS_timeout=PMODBUS_TIMEOUT;
                }

                break;
            }
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}



eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDisc )
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    if ( ( usAddress >= REG_DISC_START )
        && ( usAddress + usNDisc <= REG_DISC_START + REG_DISC_NREGS ) )
    {
        iRegIndex = ( int ) ( usAddress - usRegDiscStart );
        //PMODBUS_timeout=PMODBUS_TIMEOUT;
                while ( usNDisc > 0 )
                {
                    UCHAR ucResult =usRegDiscBuf[iRegIndex]; //xMBUtilGetBits( usRegDiscBuf, iRegIndex, 1 );

                    xMBUtilSetBits( pucRegBuffer, iRegIndex - ( usAddress - usRegDiscStart ), 1, ucResult );

                    iRegIndex++;
                    usNDisc--;
                }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}





void PMODBUS_init(){
    const UCHAR     ucSlaveID[] ="ev69_00";
        eMBErrorCode    eStatus;
     #ifdef RTU
        eStatus = eMBInit( MB_RTU, SLAVE_ID, 1, 38400, MB_PAR_NONE );
     #else
        eStatus = eMBInit( MB_ASCII, SLAVE_ID, 1, 38400, MB_PAR_NONE );
     #endif

        eStatus = eMBSetSlaveID( 96, TRUE, ucSlaveID, 20);// 96 регион типа- что еще придумать?
        sei(  );

        /* Enable the Modbus Protocol Stack. */
        eStatus = eMBEnable(  );

        //PMODBUS_timeout=PMODBUS_TIMEOUT;
};

void PMODBUS_rx(){

    usRegDiscBuf[16]=conv.err;
    usRegDiscBuf[17]=conv.switch_on;
    usRegDiscBuf[18]=conv.ready;
    usRegDiscBuf[19]=conv.heat_on;
    usRegDiscBuf[20]=conv.reg_uo;
    usRegDiscBuf[21]=conv.reg_io;
    usRegDiscBuf[22]=conv.reg_not_in_limit;
    usRegDiscBuf[23]=conv.mestnoe_upr;

    if((usRegDiscBuf[30]!=usRegCoilsBuf[30]) || usRegCoilsBuf[19] ) {
        PMODBUS_timeout=PMODBUS_TIMEOUT;
    }

    if(PMODBUS_timeout==1) { //обрыв связи по MODBUS - отключить нагрев для безопасности
        conv.command_heat_on =usRegCoilsBuf[18]=0;
    }
    if(PMODBUS_timeout) {PMODBUS_timeout--;
    conv.command_ready   =usRegCoilsBuf[17];
    conv.command_reset   =usRegCoilsBuf[16];
    conv.command_heat_on =usRegCoilsBuf[18];

    conv.ps=usRegHoldingBuf[16];
    conv.us=usRegHoldingBuf[17];
    }

    usRegInputBuf[16]=conv.P;
    usRegInputBuf[17]=conv.uou;
    usRegInputBuf[18]=conv.fou;
    usRegInputBuf[19]=conv.iou;
    usRegInputBuf[20]=conv.id;
    usRegInputBuf[21]=conv.ud;
    usRegInputBuf[22]=conv.err_cod[0];
    usRegInputBuf[23]=conv.t_max;
    if(booting==0xde) for(;;);//
}

void PMODBUS_tx(){

}

#if 0
void PMODBUS_rx(){
    tconv *pt;
    //cli();
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    pt=(tconv * )usRegHoldingBuf;//[0]

  //  conv.heartbeat = pt->heartbeat;// Возможно завести и через ТПЧ/ППЧ в случае необходимости- пока замыкается через плату  при присутствии иправных входящих пакетов от ТПЧ/ППЧ
                                   // ТЕ линия Платка-> ТПЧ/ППЧ остается под контролем только диагностики преобразователя

//    if(heartbeat != conv.heartbeat){
  //      heartbeat = conv.heartbeat;
   //     timeout_heartbeat=TIMEOUT_HEARTBEAT;
 //   }

    if(timeout_heartbeat > 1) {// один шаг для однократного сброса  команд ТПЧ после пропадания связи
        timeout_heartbeat--;

    conv.command_heat_on = pt->command_heat_on;//Битовое поле, см. коментарии к структуре conv в svzw.h
    conv.command_ready = pt->command_ready;
    conv.command_reset = pt->command_reset;
 //   conv.pusk_so = !pt->pusk_so;// Изначально вкл- кнопкой можно выключить
    //usRegHoldingBuf[1];              //Резерв, если 16 бит почему-то не хватит
    conv.ps= (unsigned long)usRegHoldingBuf[2]<<16;//Старшая пара байт мощности, В ППЧ мощность в Ваттах!
    conv.ps+= usRegHoldingBuf[3];                  //Младшая пара байт мощности

    }
    else if (timeout_heartbeat) // однократный сброс после пропадания связи- далее не мишает другим источникам команд
    {// Исчерпан таймаут по хертбиту от контроллера-> останавливаем преобразователь, контроллер сам должен обнаружить отутствие хертбита
        timeout_heartbeat--;
        conv.command_heat_on = 0;// и ошибку связи, возможно несколько раньше или позже
        conv.command_ready = 0;
        conv.command_reset = 0;
//        conv.pusk_so = 0;
    }



    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    sei();
}

void PMODBUS_tx(){
    //usRegHoldingBuf[32]=* ((int *) &conv);
    tconv *pt1;

if(conv.err_cod[0]==ERR_CONNECT){
    //cli();
    pt1=(tconv * )usRegHoldingBuf;//[0]
    pt1->err=conv.err;
    pt1->bloked=conv.bloked;
    pt1->conn_err=conv.conn_err;
    usRegHoldingBuf[33]=0;
    usRegHoldingBuf[34]=0;
    usRegHoldingBuf[35]=0;
    usRegHoldingBuf[36]=0;
    usRegHoldingBuf[37]=0;
    usRegHoldingBuf[38]=0;
    usRegHoldingBuf[39]=0;
    usRegHoldingBuf[40]=0;
    usRegHoldingBuf[41]=0;
    usRegHoldingBuf[42]=0;
    usRegHoldingBuf[43]=0;
    usRegHoldingBuf[44]=0;
    usRegHoldingBuf[45]=0;
    usRegHoldingBuf[46]=0;
    usRegHoldingBuf[47]=0;
    usRegHoldingBuf[48]=0;
    usRegHoldingBuf[49]=0;
    usRegHoldingBuf[50]=0;
    usRegHoldingBuf[51]=0;
    usRegHoldingBuf[52]=conv.err_cod[0];
    usRegHoldingBuf[53]=0;
    usRegHoldingBuf[54]=0;
}
else {
    // cli();
     usRegHoldingBuf[32]=* ((int *) &conv);//Битовое поле, см. коментарии к структуре conv в svzw.h
     usRegHoldingBuf[33]=0;                //Резерв, если 16 бит почему-то не хватит
     usRegHoldingBuf[34]=conv.P>>16;       //Старшая пара байт мощности, В ППЧ мощность в Ваттах!
     usRegHoldingBuf[35]=conv.P;           //Младшая пара байт мощности
     usRegHoldingBuf[36]=conv.fou>>16;     // Старшая пара байт частоты (она в Гц может быть 500 000 в ППЧ)
     usRegHoldingBuf[37]=conv.fou;         // мл пара частоты
     usRegHoldingBuf[38]=conv.id;          // Выпрямленный ток в А
     usRegHoldingBuf[39]=conv.iou;         // Выходной ток в А- в ТПЧ равен выпрямленному
     usRegHoldingBuf[40]=conv.uou;         // Напряжение Выходное преобразователя в В
     usRegHoldingBuf[41]=conv.t_water;     // Температура входной воды- не заполняется!
     usRegHoldingBuf[42]=conv.t[0];          // Температура 0 датчика- зависит от применения
     usRegHoldingBuf[43]=conv.t[1];          // Температура 1 датчика- зависит от применения
     usRegHoldingBuf[44]=conv.t[2];          // Температура 2 датчика- зависит от применения
     usRegHoldingBuf[45]=conv.t[3];          // Температура 3 датчика- зависит от применения
     usRegHoldingBuf[46]=conv.t[4];          // Температура 4 датчика- зависит от применения
     usRegHoldingBuf[47]=conv.t[5];          // Температура 5 датчика- зависит от применения
     usRegHoldingBuf[48]=conv.t[6];          // Температура 6 датчика- зависит от применения
     usRegHoldingBuf[49]=conv.t[7];          // Температура 7 датчика- зависит от применения
     usRegHoldingBuf[50]=conv.t_max;       // Максимальная температура
     usRegHoldingBuf[51]=conv.rashod;      // Расход воды по данным расходомера в преобразователе
     usRegHoldingBuf[52]=conv.err_cod[0];    // Код ошибки
     usRegHoldingBuf[53]=conv.err_cod[1];    // Код ошибки
     usRegHoldingBuf[54]=conv.err_cod[2];    // Код ошибки
}
    sei();
}
#endif

