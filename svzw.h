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
#ifndef ERR_MAX
  #define ERR_MAX 4
#endif


#ifndef T_STATUS
  #define T_STATUS 1
#endif
typedef struct {
                unsigned char   heat_on     :1,//0
                                err         :1,//1
                                reg_io      :1,//2
                                charge      :1,//3
                                ready       :1,//4
                                reg_in_limit:1,//5
                                mestnoe_upr :1,//6
                                reg_uo      :1;//7

         } tstatus;

typedef struct {
             unsigned char sinc; //0x55
             unsigned char heat_on   :1,//0
                           err       :1,//1
                           switch_on :1,//2
                           err_so    :1,//3
                           err_bk    :1,//4
                       mestnoe_upr   :1,//5
                           so_on     :1,//6
                           r7        :1;//7
             unsigned int Pa;
             unsigned int id;
             unsigned int Ui;
             unsigned int fi;
             //******************
#ifdef NEW_PROTO
             unsigned char  t_max;
             unsigned char  t_water;
             unsigned char err_cod;
#endif
             //******************
             unsigned int crc;
               } trxtpch;

typedef struct {
             unsigned char sinc; //0x55
             unsigned char heat_on   :1,//0
                           err       :1,//1
                           switch_on :1,//2
                           err_so    :1,//3
                           err_bk    :1,//4
                       mestnoe_upr   :1,//5
                           so_on     :1,//6
                           r7        :1;//7
             unsigned int Pa;
             unsigned int id;
             unsigned int Ui;
             unsigned int fi;
             //******************
//#ifdef NEW_PROTO
             unsigned char  t_max;
             unsigned char  t_water;
             unsigned char err_cod;
//#endif
             //******************
             unsigned int crc;
               } trxtpch_n;


typedef struct {
             unsigned char sinc; //0x55
             unsigned char heat_on   :1,//0
                           reset     :1,//1
                           r2        :1,//2
                           pusk_so   :1,//3
                           reset_so  :1,//4
                           r5        :1,//5
                        mestnoe_upr_p:1,//6
                           r7        :1;//7
             unsigned int zad;
             unsigned int crc;
               }ttxtpch ;

#ifndef T_POWER
   #define T_POWER 2
#endif
typedef struct {
                unsigned long power;
            } tpower;

#ifndef T_UO
  #define T_UO 3
#endif
typedef struct {
                unsigned int uo;
            } tuo;

#ifndef T_IO
  #define T_IO 4
#endif
typedef struct {
                unsigned int io;
            } tio;

#ifndef T_ID
  #define T_ID 7
#endif
typedef struct {
                unsigned int id;
            } tid;

#ifndef T_UD
  #define T_UD 9
#endif
typedef struct {
                unsigned int ud;
            } tud;

#ifndef T_FO
  #define T_FO 8
#endif
typedef struct {
                unsigned long fo;
            } tfo;

#ifndef T_ERR_CODE
  #define T_ERR_CODE 5
#endif

typedef struct {
                unsigned int err_code[ERR_MAX];
            } terr_code;

#ifndef T_TEMPERAT
  #define T_TEMPERAT 6
#endif
typedef struct {
    unsigned char  t_water;           // Температура воды на входе
    unsigned char  t[10];                // Температура 0 датчика- зависит от применения
    unsigned char  t_max;             // Максимальная Температура
    unsigned char  rashod;            // Расход воды по данным расходомера в преобразователе
            } ttemperat;

unsigned int PIN_input(char chan);
void PIN_output(unsigned int var, unsigned char chan);





