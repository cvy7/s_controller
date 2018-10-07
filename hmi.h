#ifndef HMI_H
#define HMI_H

typedef struct {
                void *var;
                float  preset;
                char  name[5];
                char  type;
                long  min;
                long  max;
                char  pos;
                char  lin;
                char  scr;
                float inc;
               } HMI_t_var;

typedef struct {
                void *var;
                char  name[5];
                char  type;
               } HMI_t_varr;

#define HMI_ASINC_TIME (250)
//время периодического асинхронного запуска HMI в мс
//оно и HMI_asinc_time используется в главном цикле для асинхронного запуска HMI
#define EEP_MAGIC (456987125UL)
//данные по адресу 1 в EEPROM для валидации
#define HMI_UPDATE_TIMEOUT (250)
//время автосохранения в циклах 250*250=62.5 c
#define HMI_MAX_POS_INDEX  (6)
//количество переменных в строке(на всех экранах)
#define HMI_MAX_LIN_INDEX  (16)
//количество строк переменных (на всех экранах)
#define HMI_MAX_SCR        (4)
//количество экранов

#define HMI_TERM_MAX_LINE 4
#define HMI_TERM_MAX_POS 80

extern unsigned char HMI_update_time;
extern char HMI_update_event,HMI_preset_event,HMI_asinc_event,HMI_change_event;
extern unsigned char HMI_asinc_time;

extern signed char HMI_lin_index,HMI_pos_index, HMI_scr, HMI_lin, HMI_pos;
extern signed char HMI_lin_index_old,HMI_pos_index_old;
extern signed char HMI_pos_shift;
extern void * HMI_varadr;
extern char HMI_vartype;
extern long HMI_var_min;
extern long HMI_var_max;
extern float HMI_var_inc;
extern char HMI_term_mode;
extern char HMI_term_buffer[HMI_TERM_MAX_LINE][HMI_TERM_MAX_POS] __attribute__ ((section (".noinit")));
extern char HMI_term_line,HMI_term_pos;
extern char HMI_command_start_process;


void HMI_hwo();
void HMI_poll();
void HMI_var_set();
void HMI_var_set1();
void HMI_var_get();
void CON_Print(const char  * pstr);
void CON_Putchar(char c);
void CON_PPrint(const char  * pstr);
#endif // HMI_H
