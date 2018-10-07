#ifndef CONV_H
#define CONV_H

#define ERR_MAX 4
typedef struct {
              unsigned int  heat_on         :1,// Нагрев включен
                            command_heat_on :1,// Команда включить нагрев
                            ready           :1,// Готовность преобразователя
                            command_ready   :1,// Команда привести в готовность преобразователь- можно выдать сразу- тогда ППЧ сразу включит контакторы
                                               // ТПЧ - возможно взводиьть автомат вручную- если без привода
                            err             :1,// Ошибка преобразователя (для ППЧ по сути предупреждение- преобразователь работает при этом)
                            bloked          :1,// Фатальная ошибка преобразователя- отключен
                            conn_err        :1,// Ошибка связи с преобразователем
                            command_reset   :1,// Команда сбросить ошибки- аналогична команде ready
                  //--------------------------------------

                            switch_on       :1,// Автомат включен- заполняется ТПЧ/ для ППЧ- это charge
                            err_so          :1,// Ошибка Станции охлаждения - заполняется ТПЧ, если датчики протока подключены к ТПЧ, а не напрямую к Сименсу0
                            err_bk          :1,// Ошибка по датчикам Блока Компенсации - аналогично ошибке СО
                            mestnoe_upr     :1,// Ключ на преобразователе переведен в местный режим
                            so_on           :1,// Станция охлаждения включена- опять же если используется канал Сименс->ТПЧ->СО, если напрямую- то напрямую
                            reg_not_in_limit :1,// Команда включить станцию охлаждения
                            reg_uo           :1,//reg_u
                            reg_io           :1;//reg_i

              unsigned int P;                 // Измеренная мощность
              unsigned int ps;                // Заданная мощность
              unsigned long fou;               // Частота выходного напряжения (либо результат анализа нагрузки, если ошибка по частоте ППЧ)
              unsigned int  id;                // Выпрямленный ток в А
              unsigned int  iou;               // Выходной ток в А- в ТПЧ равен выпрямленному
              unsigned int  is;
              unsigned int  uou;               // Напряжение Выходное преобразователя
              unsigned int  us;
              unsigned int  ud;
              unsigned char  t_water;          // Температура воды на входе
              unsigned char  t[10];             // Температура 0 датчика- зависит от применения
              unsigned char  t_max;            // Максимальная Температура
              unsigned char  rashod;            // Расход воды по данным расходомера в преобразователе
              unsigned char  t_maxp;
              unsigned int  err_cod[ERR_MAX];  // Код ошибки
              } tconv;





void CONV_poll(void);
void CONV_err_print(void);
void CONV_reset(void);
void CONV_reset0(void);
void CONV_heat_on(void);
void CONV_heat_off(void);
void CONV_ready(void);
void CONV_off(void);
extern unsigned char conv_timeout;
extern unsigned char conv_start_tx;
extern tconv conv;
extern signed char CONV_state;
//extern tdiags diags;
//extern tplst plst;

#define CONV_ERR_OFF  (-1)
#define CONV_OFF      (0 )
#define CONV_CHARGE   (5 )
#define CONV_CHARGE1  (6 )
#define CONV_CHARGE2  (7 )
#define CONV_ERR_ON   (8 )
#define CONV_READY    (10)
#define CONV_HEAT_ON1 (18)
#define CONV_HEAT_ON2 (19)
#define CONV_HEAT_ON  (20)

#endif // CONV_H
