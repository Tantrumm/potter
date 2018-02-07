#ifndef F_CPU
  #define F_CPU 8000000UL
#endif
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <stdbool.h>
#include <avr/interrupt.h>
 
#include "ws2812.h"
#include "twi.h"
#include "RTC.h"
#include "usart.h"
 
// ����, �������� � ������ ����������� (����, ��� ��� �����) ������������ � ws2812.h
 
#define LED_MIDDLE 137 //��������� ������� �������, ����� ����� ����� �������� ������� � �����
#define LED_COUNT 276 // ����� ���������� �����������
#define LED_BRIGHTNESS 5 // ������� ������� (1 - 255)
#define DELAY_START 6
#define DELAY_TREMBLE 10
#define OFFSET_TREMBLE 5
 
#define GREEN_G 100.0
#define GREEN_R 30.0
#define GREEN_B 0
 
#define RED_R 100.0
#define RED_G 15.0
#define RED_B 5.0
 
#define LED_DLITELNOST_STOLKNOVENIA 210 //7 sec ���������� ������ ������������ ����
#define LED_BEFORE_DAMBLDOR 300 //10 sec ��������� pzr � 50 �� 75
 
#define PROHOZHDENIE_SHAHMAT_AUDIO_DLINA 570 //19sec*30 (30inter. = 1sec)
#define PAUSA_PERED_KRASNOY_KNOPKOY 480 //9sec audio + 7 sec lenta
#define PAUSA_PERED_SINEY_KNOPKOY 420 //7 sec audio + 7 sec lenta
#define PAUSA_PERED_POBEDOI 480 //8 sec audio + 8 sec lenta
#define FINAL 510 //18sec audio Volana (����� �������, ����� ����������� ���������)

ws2812_rec leds[LED_COUNT]; 
unsigned int count=0;
double currentPzv = -1; // ������� ������� ������� ����� (���)
bool led_started = false; // 
bool led_miganie_active = false; // true, ���� ����� ��������� � ������ �������
int led_count_qarter;
int led_count_half;
double red_middle, green_middle;
int period_ot_starta_do_podskazki;

int red_r, red_g, red_b, green_r, green_g; // �������� ������������ ����� (RGB) ������� � ������� �����. ������� ���� ������ ��������� �� LED_BRIGHTNESS
 
bool button_slytherin_active = false;//����� true - ����� ������ � ������������ ������� �� ������
bool button_griffindor_active = false;
bool button_kogtevran_active = false;
bool button_slytherin_pressed = false;//����� true - ������ ���, ��� ������� ����������� ����� ������� ������ ������
bool button_griffindor_pressed = false;
bool button_kogtevran_pressed = false;
 
bool boi_activated = false;//false ���� ����� ��������� � ����������� ������ �� ����������
int gercon_time_to_start = 1810; //1750; //38sec (20 sec - ����� �� ����������� ������ �� ������ ������������ ����� boi_start + 18 sec - ������������ ����� �����)
bool boi_started = false;//true ����� ������ ���������� ��������
bool user_nachal_boi = false, user_nachal_boi_2 = false; //��� ��������� count ����� ���� ������ ��� (������� �������� ������� � �����)
int podskazka_dambldora_zakonchena= 870; //29��� (17� ��������� ����� + 12� ����� �����)
bool boi_final = false; //true, ����� ��� ������� � ����� �������� ��������� ����, ����� ����� ����� false
bool boi_final_while = false; //true, ����� ��� ������� � ����� �������� ��������� ����, �� ����� ����� ����� false
bool palochka_u_gerkona = true; // true, ���� ���� ������ ����� � �������, ����� - false
bool miganie_do_boia = false;
bool miganie_do_boia2 = false;
bool boi_idet = false;
bool green = false;
bool red = false;
bool blue = false;
 
unsigned char min,sec,hour,day,date,month,year;
 
void timer0_init(void) {
    TCCR0=(1<<CS00)|(1<<CS02); //1024 ������������
    TIMSK=(1<<TOIE0); // ��������� ������
    //asm("sei");
    sei(); 
}
 
void preset(void){
    //���������� �������: white 1-7, blue 8; blue 9 - GND
    DDRD &= ~((1<<PIND1)|(1<<PIND2)|(1<<PIND3)|(1<<PIND4));//�� ����: ������ ������� � 3 ��������� ������
    DDRD |= (1<<PIND5)|(1<<PIND6);//�� �����: 2 � 3 ����������
    PORTD &= ~((1<<PIND5)|(1<<PIND6));
   
    DDRB |= (1<<PINB0)|(1<<PINB1)|(1<<PINB2)|(1<<PINB4)|(1<<PINB5)|(1<<PINB6)|(1<<PINB7);//�� �����: �������� ���, ������ ������ ���, ���������� ���, 2 ��������, ����������� � 1 ���������
    PORTB &= ~((1<<PINB0)|(1<<PINB1)|(1<<PINB2)|(1<<PINB4)|(1<<PINB5)|(1<<PINB6)|(1<<PINB7));
   
    DDRC |= (1<<PINC0)|(1<<PINC1)|(1<<PINC2)|(1<<PINC3)|(1<<PINC4)|(1<<PINC5);
    PORTC &= ~((1<<PINC0)|(1<<PINC1)|(1<<PINC2)|(1<<PINC3)|(1<<PINC4)|(1<<PINC5));
}
 
void presetLedValues(int brightness) {
	red_r = (int) ((double) brightness * RED_R / 100.0);
	red_g = (int) ((double) brightness * RED_G / 100.0);
	red_b = (int) ((double) brightness * RED_B / 100.0);
	green_r = (int) ((double) brightness * GREEN_R / 100.0);
	green_g = (int) ((double) brightness * GREEN_G / 100.0);
	led_count_qarter = LED_COUNT/4;
	led_count_half = LED_COUNT/2;
	red_middle = (double) (RED_R + GREEN_R) / 2.0;
	green_middle = (double) (RED_G + GREEN_G) / 2.0;
	period_ot_starta_do_podskazki = LED_DLITELNOST_STOLKNOVENIA + LED_BEFORE_DAMBLDOR;
}
 
void onClear(ws2812_rec* led) {
    led->r = 0;
    led->g = 0;
    led->b = 0;
}
 
void onRedBr(ws2812_rec* led, int brightness) {
    led->r = red_r;
    led->g = red_g;
    led->b = red_b;
}
 
void onGreenBr(ws2812_rec* led, int brightness) {
    led->g = green_g;
    led->r = green_r;
    led->b = 0;
}
 
void onRed(ws2812_rec* led) {
    onRedBr(led, LED_BRIGHTNESS);
}
 
void onGreen(ws2812_rec* led) {
    onGreenBr(led, LED_BRIGHTNESS);
}
 
void clear() {
    for (int i=0; i<LED_COUNT; i++) {
        onClear(&leds[i]);
    }
	led_data_out(&leds[0], LED_COUNT);
}
 
void start() {
    for (int i=0; i<led_count_qarter; i++) {
        onGreen(&leds[i]);
        onGreen(&leds[LED_COUNT-i-1]);
        onRed(&leds[led_count_half-i-1]);
        onRed(&leds[led_count_half+i]);
       
        led_data_out(&leds[0], LED_COUNT);
        _delay_ms(DELAY_START);
    }
}
 
// % of red
void move(double persent) {
    int maxRed = led_count_half * persent * 0.01;
    for (int i=0; i<led_count_half; i++) {
       
        if (maxRed - i < OFFSET_TREMBLE && maxRed - i > 0) {
            double parts = (double)(OFFSET_TREMBLE - (maxRed - i)) / (double) OFFSET_TREMBLE;
            leds[LED_COUNT-i-1].r = leds[i].r =  red_middle* parts;
            leds[LED_COUNT-i-1].g = leds[i].g = green_middle * parts;
            leds[LED_COUNT-i-1].b = leds[i].b = 0;
        } else if (i - maxRed < OFFSET_TREMBLE && i - maxRed > 0) {
            double parts = (double)(OFFSET_TREMBLE - (i - maxRed)) / (double) OFFSET_TREMBLE;
            leds[LED_COUNT-i-1].r = leds[i].r = red_middle * parts;
            leds[LED_COUNT-i-1].g = leds[i].g = green_middle * parts;
            leds[LED_COUNT-i-1].b = leds[i].b = 0;
        } else if (i == maxRed) {
            leds[LED_COUNT-i-1].r = leds[i].r = red_middle;
            leds[LED_COUNT-i-1].g = leds[i].g = green_middle;
            leds[LED_COUNT-i-1].b = leds[i].b = 0;
        } else {
            if (i < maxRed) {
                onGreen(&leds[i]);
                onGreen(&leds[LED_COUNT-i-1]);
            } else {
                onRed(&leds[i]);
                onRed(&leds[LED_COUNT-i-1]);
            }
        }
    }
    led_data_out(&leds[0], LED_COUNT);
}
 
void move_tremble(double persent) {
    move(persent);
    _delay_ms(DELAY_TREMBLE);
    for (int i=0; i<OFFSET_TREMBLE; i++) {
        if (persent - i >= 0) {
            move(persent-i);
            _delay_ms(DELAY_TREMBLE);  
        }
    }
    for (int i=0; i<OFFSET_TREMBLE*2; i++) {
        if (persent - OFFSET_TREMBLE + i <= 100) {
            move(persent-OFFSET_TREMBLE+i);
            _delay_ms(DELAY_TREMBLE);
        }
    }
    for (int i=0; i<OFFSET_TREMBLE; i++) {
        if (persent + OFFSET_TREMBLE - i >= 0) {
            move(persent+OFFSET_TREMBLE-i);
            _delay_ms(DELAY_TREMBLE);
        }
    }
}

void final_check(int count) {
	if (boi_final) {
		if (count > FINAL){
			//�������� ��� ��������
			PORTB |= (1<<PINB0);
			boi_final = false;
		}
	}
}
 
ISR (TIMER0_OVF_vect) {
	
	count++;
	
	bool time_half_sec = count % 15 == 0;
	bool time_one_sec = count % 30 == 0;
   
    //���� ���� ��������� ���� ��������� ����������� ������
    if (!boi_activated && PROHOZHDENIE_SHAHMAT_AUDIO_DLINA < count){
        count = 0;
        boi_activated = true;
        return;
    }
    //����� �� ��������, ������ ����� �������� ����
   
    //����, ���� ���������� ���������� ������� ������������ ����� boi_start + ���� ���� ���� �����������
    if (!boi_started){
        if(--gercon_time_to_start == 0) {
            boi_started = true;
			led_miganie_active = true;
        } else {
            return;
        }
    }
    //����� �� ��������, ������ ����� �������� ���� ������ � ������ ���
   
    //������ �����������, ���� ����� ������ ���, �� �� ����, ��� �����
	if (boi_started){
		// ���� �� ������ ������� ����� �������
		if (PIND & (1<<PIND1)) {
			boi_idet = false;
			// ��������� miganie_do_boia2 false, ������� ���� ����� �������� �� ���� �������, ���� ���� �� �������� �����
			if (!miganie_do_boia2) {
				miganie_do_boia = true;
			}
			palochka_u_gerkona = false;
			led_miganie_active = true;
			if (boi_final) {
				final_check(count); // ���� ��� ��������, �� ����, ���� ����� �������� ���������� ����
			} else {
				count--; // ��� �������� ��������������� �� �����, count �������� ���������� �� ��� ����� ������� ���
			}
			return;
		} else {
			boi_idet = true;
			miganie_do_boia = false;
			miganie_do_boia2 = true; // ��������� � true, ����� miganie_do_boia �������� �������� false
			// ���� ������ ������� ����� �������
			palochka_u_gerkona = true;
			//������� �������� ����������
			led_miganie_active = false;
           
			// ��� ������ ���� ������� ������ ������� � ������� - count ����������, ��� �������
			if (!user_nachal_boi && !user_nachal_boi_2){
				count = 0;
				user_nachal_boi = true;
				user_nachal_boi_2 = true; // ��������������� ����������, ����� ������� ������ �� ����� � ���� ��
			}     
		}
       
		// ������� � ���� ��, ������ ����� ����, ��� ���� ������� ������ ������� � �������
		// ���������� ��������, ���� �� ���������� ��������� ���������
		if (user_nachal_boi) {
			// LENTA
			// PZV: 50 (������������)
			if (!led_started) {
				currentPzv = 50;
				led_started = true; // ���� ��� WHILE, ���������� ������ �� �������� ����� � �������� �����
			}
			
			if (count > LED_DLITELNOST_STOLKNOVENIA && count <= period_ot_starta_do_podskazki) {
				// LENTA
				// PZV: 50 -> 75 (����� ���������)
				if (time_one_sec) {
					currentPzv += 2.5; // (75 - 50) / 10 sec
				}
			}
       
			// ����, ���� ����������� ���������� ��������� ���������
			if (--podskazka_dambldora_zakonchena == 0){
				// ������� ������ ���������� ��������
				button_slytherin_active = true;
				user_nachal_boi = false; // �������� ����, ����� �� �������� � ��
			}
		}
       
		if (button_slytherin_active) {
			if (time_half_sec == 0){
				PORTD ^= (1<<PIND5);//������� ����������� ��� �������� (��������)
			}
			if (!(PIND & (1<<PIND3))){//2 ������ ��� ��������
				PORTC |= (1<<PINC2);//������ �� ����������� ����� ��������
				PORTD |= (1<<PIND5);//������� ��������� ������ ��������
				button_slytherin_active = false;
				count = 0;
				button_slytherin_pressed = true;
				green = true;
			}
		}
   
		if (button_slytherin_pressed){
			//LENTA
			// PZV: 75 -> 55 (����� ��� ��� ���������, ����� ����������� �������)
			if (time_one_sec) {
				currentPzv -= 1.25; //(75 - 55) / 16 [9 + 7 sec]
			}
       
			if (count > PAUSA_PERED_KRASNOY_KNOPKOY){
				button_griffindor_active = true;
				button_slytherin_pressed = false;
				red = true;
			}
		}
   
		if (button_griffindor_active){
			if (time_half_sec){
				PORTB ^= (1<<PINB7);//������� LED ��� �������. ����: �������
			}
			if (!(PIND & (1<<PIND2))){//���� ���� �� ������ ��� �������
				PORTC |= (1<<PINC3);//������ �� ����������� ����� ����������
				PORTB |= (1<<PINB7);//����������� �������� LED ��� �������
				button_griffindor_active = false;
				count = 0;
				button_griffindor_pressed = true;
			}
		}
   
		if (button_griffindor_pressed){
			//LENTA
			// PZV: 55 -> 35 (����� ���������)
			if (time_one_sec) {
				currentPzv -= 1.43; //(55 - 35) / 14 [7 + 7 sec]
			}
 
			if (count > PAUSA_PERED_SINEY_KNOPKOY){
				button_kogtevran_active = true;
				button_griffindor_pressed = false;
			}
		}
   
		if (button_kogtevran_active){
			if (time_half_sec){
				PORTD ^= (1<<PIND6);//LED ��� ������������. ����: �����
			}
			if (!(PIND & (1<<PIND4))){//���� ���� ������ �� ������ ��� ������������
				PORTD |= (1<<PIND6);//����������� �������� LED ��� ������������. ����: �����
				PORTC |= (1<<PINC1);//������ �� ����������� ����� ���������
				button_kogtevran_active = false;
				count = 0;
				button_kogtevran_pressed = true;
				blue = true;
			}
		}
   
		if (button_kogtevran_pressed){
			//LENTA
			// PZV: 35 -> 0 (����� ���������)
			if (time_one_sec) {
				currentPzv -= 2.1875; // (35 - 0) / 16 [8 + 8 sec]
			}
 
			if (count > PAUSA_PERED_POBEDOI){
				button_kogtevran_pressed = false;
				count = 0;
				boi_final = true;
				boi_final_while = true;
				//LENTA
				currentPzv = -1; // � WHILE ����� ����������
				//��������� ����������
				PORTD &= ~((1<<PIND5)|(1<<PIND6));
				PORTB &= ~(1<<PINB7);
				//����������� ���������
				PORTB &= ~(1<<PINB6);//����������� ����
			}
		}
   
		final_check(count);
	}
}
 
int main(void)
{
	preset();
	presetLedValues(LED_BRIGHTNESS);
	
	_delay_ms(500);
	led_data_init();
	clear();
  
	timer0_init(); 
  
	bool last_led_started = led_started;
	int last_current_pzv = currentPzv;
	int while_count = 0;
	long need_to_send_signal_counter = 0;
	long need_to_send_vozobn_counter = 0;
	int next_led_await = 30; // ���� 3 �������� �������� ����� (�� ��������, ��� � ���� ������� ������ ������ 20� �������� ��������, 20 �������� = 1 ��������)
	// ����� ���� �����/����������� ����
	bool miganie_finished = false;
	bool led_miganie_running = true;
	
	while(1) {   
	
		if (boi_final_while) {
			continue;
		}
	
		// ������������� ������������ ������� ���������� ���� ������ � ������
		if (while_count++ % 20 != 0) {
			continue;
		} else if (while_count >= 1000000) {
			// � ������, �� ����� ����� �� ������� ������������� �������� int, ��� ��������������
			while_count = 0;
			continue;
		}
		
		if (!palochka_u_gerkona || miganie_do_boia) {
			// ���� ����� ������� ��� ��� �� ������� (����� ����� ������ ������������ ����� ������������� �����)
			need_to_send_signal_counter++;
			need_to_send_vozobn_counter = 0;
		} 
		if (palochka_u_gerkona) {
			need_to_send_signal_counter = 0;
		}
		if (boi_idet) {
			need_to_send_vozobn_counter++;
		}
		if (need_to_send_signal_counter == 1) {
			// ���� ����� �������, ���� �� ��������� 1 ��� �� ���������� ���
			PORTB &= ~(1<<PINB6);//����������� ����			
			PORTB |= (1<<PINB2); // ������ �� ����������� ����� � ���������� ���
			PORTB &= ~(1<<PINB1);//������� ������ � ������/������������� ��� 
			PORTD &= ~((1<<PIND5)|(1<<PIND6));
			PORTB &= ~(1<<PINB7);//��������� ����������
		}
		if (need_to_send_vozobn_counter == 1) {
			// ���� ������ �������, ���� �� ��������� 1 ��� 
			PORTB |= (1<<PINB6);//����������� ���
			PORTB |= (1<<PINB1); // ������ �� ����������� ����� � ������ ���
			PORTB &= ~(1<<PINB2);//������� ������ � ���������� ���
			
			//���� ��������� �������
			if (green){
				PORTD |= (1<<PIND5);
			}
			//���� ��������� �������
			if (red){
				PORTB |= (1<<PINB7);
			}
			//���� ��������� �����
			if (blue) {
				PORTD |= (1<<PIND6);
			}
		}
	
		// �������
		if (led_miganie_active) {
			if (!miganie_finished
				clear(); // ������� ������ ��������, ������� ��� �����
				miganie_finished = true; // ���� ���� ��� ����������� ����� ����, ��� ������� ����������
			}
			
			if (next_led_await == 30) {
				if (led_miganie_running) {
					// ���� ��������� ������ ��������
					onRed(&leds[LED_MIDDLE]);
					led_data_out(&leds[0], LED_COUNT); 
				} else {
					clear();
				}
				led_miganie_running = !led_miganie_running;
			} 
			if (--next_led_await == 0) {
				next_led_await = 30;
			}
			
			continue; // ���� �� � ������ �������, �� �������� � ������ �� ����
		}
		
		if (miganie_finished) {
			miganie_finished = false;
			clear(); // ������� �����������, ������� ����� ����� ������� � ���	
		}
		
		
		
		
		
		
		// �����
		// ������� � ������� ����� �������� ����������
		// PZV: 50 (�����)
		if (last_led_started != led_started && led_started) {
			PORTB |= (1<<PINB1); // ������ �� ����������� ����� � ������ ��� 
			PORTB &= ~(1<<PINB2);
			start(); // ������� � ������� ����� �������� ����������
			last_led_started = led_started;
		}
		
		// ���� �������� pzv ���������� ��� ����� ���������� ������� led ����� �� ����� (pzv � ���������� ��������)
		if (last_current_pzv != currentPzv || (currentPzv > -1 && currentPzv <= 100)) {
			if (currentPzv == -1) {
				// ��� ����������
				clear();
			} else {
				if (palochka_u_gerkona) {
					move_tremble(currentPzv); 
				} else {
					move(currentPzv); 
				}
			}
			last_current_pzv = currentPzv;
		}
	}
}
 
/*
#ifdef LED_DATA2
led_data_out2(&leds[0], &leds[LED_COUNT / 2], LED_COUNT / 2);
#else
led_data_out(&leds[0], LED_COUNT);
#endif
*/
