#ifndef AUT_H
#define AUT_H

#define AUT_TIME        (1)
//18432кГц/65536
//281,25Гц 3,5(5)мс

extern int AUT_time;
extern char AUT_event;

extern char AUT_err;


int   AUT_timer(int time, char chan);
float AUT_pid(float set,float mes, float kp, float ki, float *integ);
float AUT_interp(int per, float * interp_data);
void  AUT_err_print(void);
void  AUT_poll(void);
void  AUT_charge(int a);

char AUT_wtr(void);
char AUT_t_bk(void);
char AUT_doors(void);
#endif // AUT_H
