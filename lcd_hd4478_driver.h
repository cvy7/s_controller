/******************************************************************************
Файл: LCD_Driver.h
Creted by PROTTOSS
Mail to PROTTOSS@mail.ru
Описание функции для работы с символьным ЖКИ дисплеем на основе кристалла HD44780
16.05.2006
******************************************************************************/

#ifndef LCD_HD4478_DRIVER_H
#define LCD_HD4478_DRIVER_H

/*****************************************************************************
Types definitions
******************************************************************************/
#define UCHAR	unsigned char
#define UINT	unsigned int
#define ULONG	unsigned long
#define BOOL	UCHAR

/*
typedef char    BOOL;
typedef unsigned char UCHAR;
typedef unsigned int UINT;
typedef unsigned long ULONG;
*/

#define FALSE	0
#define TRUE	1

#define	NOP()	asm("nop")
#define SEI()	asm("sei")
#define CLI()	asm("cli")
#define SWAP(data)	data = __swap_nibbles(data)

/*****************************************************************************
Описание шин управления и данных между МП и ЖКИ
******************************************************************************/

// Шина данных
#define LCD_PORTDATA	PORTC
#define LCD_PINDATA		PINC
#define LCD_DDRDATA		DDRC

// Порт управления
#define LCD_PORTCTRL	PORTG
#define LCD_PINCCTRL	PING
#define LCD_DDRCTRL		DDRG

// Линии управления LCD
#define LCD_wire_RS	(1 << PG0)
#define LCD_wire_RW	(1 << PG2)
#define LCD_wire_E	(1 << PG1)
#define LCD_wire_BL	(1 << 7) // (BackLight) подсветка

// Параметры ЖКИ
#define LCD_ROW_SIZE	20	// длина строки дисплея в символах
#define LCD_ROW_NUM		4  	// количество строк в дисплее

/******************************************************************************
Флаги драйвера
******************************************************************************/

#define LCD_BUSY_TIMEOUT_FLAG	0x01 // таймаут ожидания BF ЖКИ

//*****************************************************************************
// Параметры ЖКИ
#define LCD_LINE1_START	0x00// начальный адрес первой линии сегментов
#define LCD_LINE2_START	0x40// начальный адрес второй линии сегментов
#define LCD_LINE_SIZE	0x28// размер линии сегментов (40 dec)
#define LCD_LINE_SIZE2	(20)// количество знакомест в строке

#if (4 == LCD_ROW_NUM) 		// начальные адреса строк для 4-х строчного ЖКИ

#define LCD_ROW1_START	LCD_LINE1_START
#define LCD_ROW2_START	LCD_LINE2_START
#define LCD_ROW3_START	LCD_ROW1_START + LCD_ROW_SIZE
#define LCD_ROW4_START	LCD_ROW2_START + LCD_ROW_SIZE

#elif(2 == LCD_ROW_NUM) 	// начальные адреса строк для 2-х строчного ЖКИ

#define LCD_ROW1_START	LCD_LINE1_START
#define LCD_ROW2_START	LCD_LINE2_START

#elif(1 == LCD_ROW_NUM) 	// начальные адреса строк для 1-о строчного ЖКИ

#define LCD_ROW1_START	LCD_LINE1_START

#else						// неопределенный ЖКИ

#error Unknown LCD format

#endif//#if (4 == LCD_ROW_NUM)
//******************************************************************************

/*****************************************************************************
команды для функций
******************************************************************************/

/******************************************************************************
Определение структур
*******************************************************************************/

/******************************************************************************
Прототипы функций работы с ЖКИ
*******************************************************************************/

// Начальная инициализация
void LCD_Init(void);
void LCD_Clear(void);

// Цикл записи
void LCD_Bus_Write(UCHAR data);

// Цикл чтения
UCHAR LCD_Bus_Read(void);

 // Цикл ожидания готовности
void LCD_White(void);

// Отправка команды
void LCD_Set(UCHAR cmd);

 // Отправка данных
void LCD_Put(UCHAR data);

// Чтение данных
UCHAR LCD_Get(void);

void LCD_Set_addr();
void LCD_Print( const char  * pstr);
void LCD_PPrint(char  * pstr);
void LCD_Print_lin(char lin, const char  * pstr);
void LCD_Cursor_pos( char pos,char line);

extern char LCD_current_line;
extern char LCD_current_pos;

#endif//LCD_HD4478_DRIVER_H
