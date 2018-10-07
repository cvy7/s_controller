#ifndef PIN_H
#define PIN_H

#define ANA0  100
#define ANA1  101
#define ANA2  102
#define ANA3  103
#define ANA4  104
#define ANA5  105
#define ANA6  106
#define ANA7  107
#define ANA8  108
#define ANA9  109
#define ANA10  110
#define ANA11  111
#define ANA12  112
#define ANA13  113
#define ANA14  114
#define ANA15  115

#define DS0    0
#define DS1    1
#define DS2    2
#define DS3    3
#define DS4    4
#define DS5    5
#define DS6    6
#define DS7    7
#define DS8    8
#define DS9    9
#define DS10   10
#define DS11   11
#define DS12   12
#define DS13   13
#define DS14   14
#define DS15   15

#define Sp0   0
#define Sp1   1

#define GREEN 1
#define RED 2
#define GREEN_TOGGLE 3
#define YELL 4
#define RED_TOGGLE 5

#define ADC_MAX_CH  2

extern  int adc_in[9];
extern  int dac_out[9];
//long adc_in[ADC_MAX_CH];
//int PIN_ADC[ADC_MAX_CH];
void PIN_poll();
void PIN_Init();
void PIN_output(unsigned int var, unsigned char chan);
unsigned int PIN_input(char chan);

#endif // ADC_H
