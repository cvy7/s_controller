#ifndef KBD_H
#define KBD_H
//Время периодического вызова опроса клавиатуры в мкс
//для 15,625 МГц
//244*64/15,625=1mc
//#define KBD_TIME (244)
#define KBD_TIME (125)

extern int KBD_time;
extern char KBD_event;
void KBD_enc_poll();
char KBD_getchar();
void KBD_poll();
void KBD_init();

#endif // KBD_H
