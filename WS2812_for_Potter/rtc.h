#ifndef RTC_H_
#define RTC_H_
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include "twi.h"

unsigned char RTC_ConvertFromDec(unsigned char c); //������� �������-����������� ����� � ����������
unsigned char RTC_ConvertFromBinDec(unsigned char c); //������� ����������� ����� � �������-����������

#endif /* RTC_H_ */