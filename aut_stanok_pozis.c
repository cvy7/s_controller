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
#include "ports.h"

//�����
#define S1 usRegDiscBuf[0]
#define S2 usRegDiscBuf[1]
#define S3 usRegDiscBuf[2]
#define S4 usRegDiscBuf[3]
#define S5 usRegDiscBuf[4]
#define S6 usRegDiscBuf[5]
#define S7 usRegDiscBuf[6]
#define S_VR_ELEV usRegDiscBuf[7]

#define Z2 usRegDiscBuf[8]
//��������� �� 2 ������� �� ���������� (��� ������� 1 ���������)
#define Z3 usRegDiscBuf[9]
//��������� �  3 ������� �� ���������� (����� �������� 2 ���������)
#define Z4 usRegDiscBuf[10]
//��������� �  4 ������� �� ���������� (��� �������� 3 ���������)

//������
#define AVOST   usRegCoilsBuf[27]
#define VIGR    usRegCoilsBuf[28]
#define REV     usRegCoilsBuf[26]
#define CICL    usRegCoilsBuf[29]
#define SERV    usRegCoilsBuf[31]

#define P1      usRegCoilsBuf[0]
#define P2      usRegCoilsBuf[1]
#define P3      usRegCoilsBuf[2]
#define M3      usRegCoilsBuf[3]
#define M2      usRegCoilsBuf[4]
#define M1      usRegCoilsBuf[5]
#define M3_REV  usRegCoilsBuf[6]
#define LMP     usRegCoilsBuf[7]


//��������� ������ S1 (�������� ��� �������)
#define disS1 usRegCoilsBuf[8]
#define disS2 usRegCoilsBuf[9]
#define disS3 usRegCoilsBuf[9]
#define disS4 usRegCoilsBuf[11]

#define invS6 usRegCoilsBuf[13]
#define disS7 usRegCoilsBuf[14]
#define disS8 usRegCoilsBuf[15]


//InputRegisters
#define AUT_STATEA     usRegInputBuf[0]
#define AUT_STATEB     usRegInputBuf[1]
//������� � �����������, ������� �������� � ���������
#define AUT_Z          usRegInputBuf[12]
//���������� ����� ����� �����������
#define AUT_T_CICLI    usRegInputBuf[13]
//��� ������
#define AUT_ERR        usRegInputBuf[15]

//HoldingRegisters
#define TF1            usRegHoldingBuf[12]
//������������� ����� �������� (������)
#define AUT_T_CICL     usRegHoldingBuf[13]
//����� �����
#define AUT_NCICL_F0   usRegHoldingBuf[14]
//������������ ���������� ������� ��������� ��������� ���������� P1, ����� �������� ������� ��������� � ������
#define TF0            usRegHoldingBuf[15]
//������������ ����� �������� ���������������

#define AUT_ERR_S1  (1)
//������ �� ������� �� ��������� 1- �������� 1 �� ��������, ���� ��� ��������� ����� ���
#define AUT_ERR_S2  (2)
//������ �� ������� �� ��������� 2- �������� 2 �� ��������, ���� ���������� ������
#define AUT_ERR_S3  (3)
//������ �� ������� �� ��������� 3- �������� 3 �� ��������, ���� ���������� ������
#define AUT_ERR_S4  (4)
//������ �� ������� 4 (�������)- ��������� ��������, ���� ���������� ������
#define AUT_ERR_VR  (6)
//6 ������ �������� �� �������� � ������
#define AUT_ERR_AVOST (-1)



extern unsigned int time65;

int AUT_t_a,AUT_t_b;
int AUT_t_cicl, AUT_t_cicl_i;

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

char AUT_cicl,AUT_cicl_old;
int  AUT_cicl_f0;

signed char AUT_state_a=0;
signed char AUT_state_b=0;
signed char AUT_state_c=0;

char AUT_next_slab;

#define AUT_STATE_ERR      (-1)
#define AUT_STATE_STOP     (0)
#define AUT_STATE_START    (1)
#define AUT_STATE_F0       (10)
#define AUT_STATE_F1       (11)
#define AUT_STATE_F2       (12)
#define AUT_STATE_F3       (13)
#define AUT_STATE_F4       (14)
#define AUT_STATE_F5       (15)
#define AUT_STATE_F6       (16)
#define AUT_STATE_F7       (17)

#define AUT_STATE_CICL     (32)
#define AUT_STATE_VIGR     (33)
#define AUT_STATE_REV      (34)

void AUT_poll(){

AUT_hwo();
if(SERV) return;

//AUT_cicl=CICL;

/*if(!AUT_cicl){
  AUT_state_a= AUT_STATE_STOP;
  AUT_state_b= AUT_STATE_STOP;
}*/

if(AUT_t_cicl     ) AUT_t_cicl--;
if(AUT_t_a        ) AUT_t_a--;
if(AUT_t_b        ) AUT_t_b--;
if(AUT_t_cicl_i<32767) AUT_t_cicl_i++;
/*
if(AUT_cicl != AUT_cicl_old){
    //AUT_state_a=AUT_STATE_F0;
    //AUT_state_b=AUT_STATE_F0;
    AUT_cicl_old=AUT_cicl;
}
*/


switch (AUT_state_a)
{
 case AUT_STATE_ERR:
    P1=0;P2=0; M1=0;
    if(!AUT_cicl) AUT_state_a= AUT_STATE_STOP;
    break;

 case AUT_STATE_STOP:
    P1=0;P2=0; M1=0;
    AUT_ERR=0;
    if(AUT_cicl) AUT_state_a= AUT_STATE_START;
    break;

 case AUT_STATE_START:
    AUT_cicl_f0=AUT_NCICL_F0;
    AUT_state_a=AUT_STATE_F0;
    M1=1;//�������� ��������
    break;

 case AUT_STATE_F0:         //�������� 1 ��������� ���������
    P1=1; AUT_t_a=TF0;
    AUT_state_a=AUT_STATE_F1;
    break;

 case AUT_STATE_F1:         //� ������� ������� t_f0
    if(!AUT_t_a) {
        P1=0; AUT_t_a=TF0;   //� ���� ����
        AUT_state_a=AUT_STATE_F2;
        Z3=Z2;// ��������� ������������� � 3 ������� (����� �������� �2)
        }
    break;

 case AUT_STATE_F2:
    if(!AUT_t_a) {
        if(S1 && ! disS1) {//���� �������� ������- ����� �� �����, ��� ��������� ���
            if(AUT_cicl_f0--){       if(AUT_cicl) AUT_state_a=AUT_STATE_F0;//����������� �������� N_CICL ���
                                     else         AUT_state_a=AUT_STATE_STOP;}
            else{AUT_ERR=AUT_ERR_S1; AUT_state_a=AUT_STATE_ERR;}//����� ������
                              Z2=0;//��������� �� ������ ������� ���
            }
        else {                AUT_state_a=AUT_STATE_F3;//���� ������ �� �������� ���� ��������
                              Z2=1;//��������� �� ������ ������� ����
             }
    }

case AUT_STATE_F3:   //������� �� �������� ������ ������ �������� ������
    if(/*AUT_state_b==AUT_STATE_F5 && Z4==0*/ AUT_next_slab) {//������ �� ��������� P3 �����������-> ����� ��������� ���������
        AUT_next_slab=0;
        P2=1; AUT_t_a=TF0;                 //��������� � ����� ������� ������ ����������� � 4 ������� � ��������
        AUT_state_a=AUT_STATE_F4;           //���� �� �����������-�� ��������� ���������
    }
    break;

case AUT_STATE_F4:
   if(S4 && !disS4) {           //���� ������� ��������� ���� ������� S4
       P2=0;AUT_t_a=TF0;
       AUT_state_a=AUT_STATE_F5;//��������� ��������� ������� S4 - ����� �������� ������� P2
       Z4=1;
   }

   if(!AUT_t_a) {                //������� ����� ������� � �� ��������� ������� ������� S4 �� ����� ������� ������ P2 ���� ������ S4 ��������
       P2=0;AUT_t_a=TF0;          //��� ����� �������� ������� P2
       AUT_state_a=AUT_STATE_F6;//
       if(S2 && !disS2) {AUT_ERR=AUT_ERR_S2; AUT_state_a=AUT_STATE_ERR;}//������� P2 �� ��������� ����� �� ��������
   }
   break;

case AUT_STATE_F5://��������� ��������� ������� S4 - ����� �������� ������� P2
   if(!AUT_t_a) {
       if(S4 && !disS4) {AUT_ERR=AUT_ERR_S4;  AUT_state_a=AUT_STATE_ERR;}//���� �� ����� ��������� ���� ������ P2 ����� �� ���� � ������� S4-> ������ ��������-> ������
       else {if(AUT_cicl) AUT_state_a=AUT_STATE_START;//����� ��� ������- ������ � �����������-> ��������� ������
             else         AUT_state_a=AUT_STATE_STOP;}

       if(!S2 && !disS2) {AUT_ERR=AUT_ERR_S2; AUT_state_a=AUT_STATE_ERR;}//������� P2 �� ��������� ����� �� ��������
   }
   break;

case AUT_STATE_F6://�� ��������� ������� ������� S4 �� ����� ������� ������ P2 ���� ������ S4 ��������
   if(S4 && !disS4) {//��������� ����������� �� ����� ��������� ������ P2
       Z4=1;
       AUT_t_a=TF0;
       AUT_state_a=AUT_STATE_F5;//��������� ��������� ������� S4
   }
   if(!AUT_t_a) {// �� ���� ���������  ��� �������� ������ S4
       if(disS4) Z4=1;//���� ������ ��������- �������, ��� ��������� ����
       else      Z4=0;//����� ��� ���������, ��� ��� ��������, ���� ���� � 3 �������

       if(AUT_cicl) AUT_state_a=AUT_STATE_START;
       else         AUT_state_a=AUT_STATE_STOP;

       if(!S2 && !disS2) {AUT_ERR=AUT_ERR_S2; AUT_state_a=AUT_STATE_ERR;}//������� P2 �� ��������� ����� �� ��������
   }
   break;
}


switch (AUT_state_b){
case AUT_STATE_ERR://����� ����������� , �������������� P3 �������� ��� ����
    M2=0;
    M3=0;
    M3_REV=0;
    if(!AUT_cicl) AUT_state_b= AUT_STATE_STOP;
    break;

case AUT_STATE_STOP:
    //M2=0;
    //M3=0;
    P3=0;
    AUT_ERR=0;
    if(AUT_cicl) AUT_state_b=AUT_STATE_START;
    break;

case AUT_STATE_START:
    AUT_state_b=AUT_STATE_F0;
    M2=1;
    if(!S3 && !disS3) {AUT_ERR=AUT_ERR_S3; AUT_state_b=AUT_STATE_ERR;}//������� P3 �� ��������� ����� �� ��������
    break;

case AUT_STATE_F0:
    if((AUT_state_a>=AUT_STATE_STOP) && (AUT_state_a<=AUT_STATE_F3))  P3=1;
    AUT_t_b=TF0;
    AUT_state_b=AUT_STATE_F1;
    break;

case AUT_STATE_F1:
    if(!AUT_t_b) {
        AUT_state_b=AUT_STATE_F2;
        AUT_t_b=TF1;
        }
    break;

case AUT_STATE_F2:
    M3=1;
    if((S6 && invS6)  || (!S6 && !invS6))AUT_state_b=AUT_STATE_F3;
    //if(!AUT_t_b){AUT_state_b=AUT_STATE_ERR; AUT_ERR=AUT_ERR_VR;}
    if(!AUT_t_b) {AUT_t_b=TF0;AUT_state_b=AUT_STATE_F6; M3=0;}//0.5 c �������
    break;

case AUT_STATE_F3:
    M3=1;
    if((!S6 && invS6) || (S6 && !invS6 )) AUT_state_b=AUT_STATE_F4;
    //if(!AUT_t_b){AUT_state_b=AUT_STATE_ERR; AUT_ERR=AUT_ERR_VR;}
    if(!AUT_t_b) {AUT_t_b=TF0;AUT_state_b=AUT_STATE_F6; M3=0;}//0.5 c �������
    break;

case AUT_STATE_F4:
    AUT_Z<<1;//���� ����� �� ����� �����- ����������� ���������- �������
    AUT_state_b=AUT_STATE_F5;
    if(P3) { //���� ������ P3 ����������
        if(S3 && !disS3) {AUT_ERR=AUT_ERR_S3; AUT_state_b=AUT_STATE_ERR;}//������� P3 �� ��������� ����� �� ��������
        else{
            SET2(AUT_Z,1,Z4);//��������� �� 4 ������� ������ � 5- �� � �������
            Z4=0;            //��� �� �����
            AUT_next_slab=1; //��������� !!!
        }
    }
    P3=0;
    M3=0;
    AUT_t_b=TF0;
    break;

case AUT_STATE_F5:
    AUT_next_slab=0;
    if(!AUT_t_b && !AUT_t_cicl) {
        if(AUT_cicl) AUT_state_b=AUT_STATE_START;
        else        {AUT_state_b=AUT_STATE_STOP; M2=0;M3=0;}
        AUT_t_cicl=AUT_T_CICL;
        AUT_T_CICLI=AUT_t_cicl_i;
        AUT_t_cicl_i=0;
        }
    //AUT_T_CICLI=AUT_t_cicl;
    break;

case AUT_STATE_F6:
        if(!AUT_t_b){AUT_t_b=TF0;AUT_state_b=AUT_STATE_F7; M3_REV=1;}
    break;

case AUT_STATE_F7:
        if(!AUT_t_b){AUT_state_b=AUT_STATE_ERR; AUT_ERR=AUT_ERR_VR;}
    break;
}



switch (AUT_state_c){
case AUT_STATE_ERR:
    M1=0;M2=0;M3=0;M3_REV=0;
    if(AVOST) AUT_state_c=AUT_STATE_STOP;
    break;

case AUT_STATE_STOP:
    AUT_ERR=0;
    if(CICL) AUT_state_c=AUT_STATE_CICL;
    if(VIGR) AUT_state_c=AUT_STATE_VIGR;
    if(REV ) AUT_state_c=AUT_STATE_REV;
    break;

case AUT_STATE_CICL:
    if(CICL) AUT_cicl=1;
    else    {AUT_cicl=0;AUT_state_c=AUT_STATE_STOP;}
    if(!AVOST) {AUT_state_a=AUT_STATE_ERR;AUT_state_b=AUT_STATE_ERR;AUT_state_c=AUT_STATE_ERR;AUT_ERR=AUT_ERR_AVOST;}
    break;

case AUT_STATE_VIGR:
    if(VIGR) {M2=1;M3=1;}
    else     {M2=0;M3=0;AUT_state_c=AUT_STATE_STOP;}
    if(!AVOST) {AUT_state_c=AUT_STATE_ERR;AUT_ERR=AUT_ERR_AVOST;}
    break;

case AUT_STATE_REV:
    if(REV)  {M3_REV=1;}
    else     {M3_REV=0;AUT_state_c=AUT_STATE_STOP;}
    if(!AVOST) {AUT_state_c=AUT_STATE_ERR;AUT_ERR=AUT_ERR_AVOST;}
    break;
}

AUT_STATEA=AUT_state_a;
AUT_STATEB=AUT_state_b;
}
