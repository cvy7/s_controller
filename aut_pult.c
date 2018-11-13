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
#include "pin.h"
#include "aut.h"
#include "conv.h"
#include "mb.h"
#include "mb_regs.h"
#include "ports.h"
#include "config.h"
//#include "conv_p1600_u650_f2400.h"
//#include "conv_p1800_u1800_f500.h"

extern unsigned int time65;
extern char PMODBUS_timeout;

#define CODE_ERR_F_LOW      8
#define CODE_ERR_F_HI       7
#define CODE_ERR_OVHEAT1    20
#define CODE_ERR_OVHEAT2    49
#define CODE_ERR_NAGR       10
#define CODE_ERR_CONNECT    999

#define INP_READY   usRegDiscBuf[0]
#define INP_HEAT    usRegDiscBuf[1]
#define INP_RESET   usRegDiscBuf[2]
#define INP_RESERV  usRegDiscBuf[3]

#define INP_P       usRegInputBuf[0]
#define INP_U       usRegInputBuf[1]

#define OUT_P       usRegHoldingBuf[0]
#define OUT_U       usRegHoldingBuf[1]
#define OUT_I       usRegHoldingBuf[2]
#define OUT_F       usRegHoldingBuf[3]

#define OUT_READY   usRegCoilsBuf[0]
#define OUT_HEAT    usRegCoilsBuf[1]
#define OUT_ERR     usRegCoilsBuf[2]
#define OUT_ERR_F_LOW usRegCoilsBuf[3]
#define OUT_ERR_F_HI  usRegCoilsBuf[4]
#define OUT_ERR_PPCH  usRegCoilsBuf[5]
#define OUT_ERR_NAGR  usRegCoilsBuf[6]
#define OUT_ERR_OVHEAT usRegCoilsBuf[7]

#define AUT_OFF usRegCoilsBuf[15]

void AUT_hwo(){

#if 0
    if(/*MAIN_err || */conv.err) {
        PIN_output(1,HMI_LAMP_ERR);
    }
    else {
    PIN_output(0,HMI_LAMP_ERR);
    }
    char mig=(TCNT1 & 0x2000)>>8;
    char mig2=(TCNT1 & 0x8000)>>8;
    if(CONV_state==CONV_HEAT_ON) PIN_output(1,HMI_LAMP_HEAT_ON);           else
    if(CONV_state==CONV_READY)  PIN_output(mig/*time65&1*/,HMI_LAMP_HEAT_ON);     else
    if(CONV_state==CONV_CHARGE) PIN_output(mig2/*8*/,HMI_LAMP_HEAT_ON); else
    PIN_output(0,HMI_LAMP_HEAT_ON);
#endif
}

int norm_out(unsigned int p,unsigned int nom){
    long tmp;
    tmp=((long)p<<15)/nom;
    if      (tmp<0)      tmp=0;
    else if (tmp>32767)  tmp=32767;

    return tmp;
}

int norm_in(int p,int nom,int deflt){
   long tmp;
    if(p>-328){
        tmp=((long)p*nom)>>15;
        if      (tmp<0)   tmp= 0;
        else if (tmp>nom) tmp= nom;
        return tmp;
    }
    else return deflt;
}

void AUT_poll(){

if(usRegCoilsBuf[15]) return;

    AUT_hwo();
    //conv.err=1;
    //conv.err_cod[0]=500;
    //conv.P=conv.ps;
    OUT_P=norm_out(conv.P,  P_NOM);
    OUT_U=norm_out(conv.uou,U_NOM);
    OUT_I=norm_out(conv.iou,I_NOM);
    OUT_F=norm_out(conv.fou,F_NOM);

    OUT_READY=conv.ready;
    OUT_HEAT=conv.heat_on;
    OUT_ERR=conv.err;

    if(conv.err){
        if(conv.err_cod[0]==        CODE_ERR_F_LOW)
            OUT_ERR_F_LOW=1;
        else {
            if(conv.err_cod[0]==    CODE_ERR_F_HI)
                OUT_ERR_F_HI=1;
            else {
                if((conv.err_cod[0]>=CODE_ERR_OVHEAT1)
                && (conv.err_cod[0]<=CODE_ERR_OVHEAT2))
                    OUT_ERR_OVHEAT=1;
                else {
                    if(conv.err_cod[0]==CODE_ERR_NAGR)
                        OUT_ERR_NAGR=1;
                    else
                        if(conv.err_cod[0] != CODE_ERR_CONNECT) OUT_ERR_PPCH=1;
                }
            }
        }
#ifdef TPCH
        if(conv.err_bk) OUT_ERR_NAGR=1;
        if(conv.err_so) OUT_ERR_OVHEAT=1;
#endif
    }
    else {
        OUT_ERR_F_LOW=0;
        OUT_ERR_F_HI=0;
        OUT_ERR_OVHEAT=0;
        OUT_ERR_NAGR=0;
        OUT_ERR_PPCH=0;
    }

    if(!PMODBUS_timeout){
        conv.command_ready   =INP_READY;
        conv.command_reset   =INP_RESET;
        conv.command_heat_on =INP_HEAT;

        conv.ps=norm_in(INP_P,P_NOM,0);
        conv.us=norm_in(INP_U,U_NOM,U_NOM);
    }

    //usRegHoldingBuf[0]=usRegInputBuf[0];
    //usRegHoldingBuf[1]=0;
    //usRegHoldingBuf[2]=usRegInputBuf[1];
    //usRegHoldingBuf[0]=0;
    //usRegHoldingBuf[1]=0;
    //usRegHoldingBuf[2]=0;
    //usRegHoldingBuf[3]=0;
    //usRegHoldingBuf[0]=32767;
    //usRegHoldingBuf[1]=32767;
    //usRegHoldingBuf[2]=32767;
    //usRegHoldingBuf[3]=32767;
}
