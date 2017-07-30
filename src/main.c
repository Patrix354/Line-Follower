#define F_CPU 16000000UL	//Czêstotliwoœæ mikrokontrolera; 16MHz

#include <avr/io.h>			//Biblioteka ze zdefiniowanymi adresami wszystkich rejestrów
#include <avr/interrupt.h>	//Biblioteka do obs³ugi przerwañ w jêzyku C
#include <stdbool.h>		//Biblioteka ze zdefiniowanym typem _Bool
#include "ws2812b.h"		//Autorska biblioteka do obs³ugi diod adresowalnych WS2812B

#define MAX_SPEED	50	//Prêdkoœæ silników
#define MIN_SPEED	255	//0 - pe³na moc silników, 255 - wy³¹czenie silników

#define BRIGHTNESS 255	//Jasnoœæ diod 0-255

#define IR_TIMEOUT 5	//Timeout(s) = 1 / (20 / IR_TIMEOUT); Timeout jest potrzebny do odczytywania danych z pilota

TWS_RGB WS_BUFF[WS_LEN];	//Definicja bufora

volatile _Bool Engine_switch = false;	//Zmienne odpowiadaj¹ce za odczytywany stan z pilota
volatile _Bool IR_signal = true;		//--|--
volatile _Bool TIMERA_sig_triger = true;//Emulowany prescaler "Wy³¹czaj¹cy" Timer w algorytmie pilota

void INT1_init(void);	//Za³¹czenie przerwania INT1
void TIM0_init(void);	//Za³¹czeniwe Timera0(PWM)
void TIM1A_init(void);	//Za³¹czeniwe Timera1A(CTC)
void TIM2_init(void);	//Za³¹czeniwe Timera2(PWM)

int main(void)
{
	uint8_t right_sensors = 0;	//Zmienne przechowuj¹ce stan czujników
	uint8_t left_sensors = 0;	//--|--
	
	DDRA = 0x00;	//PORTA jako wejœcie (czujniki)
	PORTA = 0xFF;	//Podci¹gniêcie pinów z wyjœciami czujników do VCC
	
	DDRC |= (1<<PORTC1) | (1<<PORTC0);	//PC0..1 do sterowania silnikami(lewy)
	DDRD |= (1<<PORTD2) | (1<<PORTD6) | (1<<PORTD7);	//PD6..2 do sterowania silnikami, PD7 do sterowania PWM(lewy)
	DDRB |= (1<<PORTB3) | (1<<PORTB0);	//PB0 jako wyjœcie danych do LED-ów | PB3(OC0) jako wyjœcie PWM (Lewy)
	DDRD &= ~(1<<PORTD3);	//PB3 jako wejœcie(INT1)
	
	PORTC |= (1<<PORTC0);	//Wystaw stan wysoki na pinie PC0
	PORTD |= (1<<PORTD6) | (1<<PORTD7) | (1<<PORTD3);	//Pinu PD6..7 s¹ w stanie wysokim, pin PD3 podci¹gniêty do VCC
	PORTB |= (1<<PORTB3);	//Stan wysoki na pinie PB3
	
	INT1_init();	//Za³¹czenie przerwania INT1
	TIM0_init();	//Za³¹czeniwe Timera0(PWM)
	TIM1A_init();	//Za³¹czeniwe Timera1A(CTC)
	TIM2_init();	//Za³¹czeniwe Timera2(PWM)

	sei();	//Zezwolenie na globalne przerwania
	
    while(1) 
    {
		if(Engine_switch)	//Jeœli robot jest w stanie aktywnym
		{			
			right_sensors = (~PINA & 0x0F);	//Sczytaj wartoœci z czyjników do zmiennych left_sensors i right_sensors
			left_sensors = (~PINA & 0xF0);
			
			if(right_sensors != 0)			//Je¿eli linia lest nad czterema prawymi czujnikami
			{
				OCR2 = MIN_SPEED;	//Wy³¹cz prawy silnik	
			}
			else					//W przeciwnym wypadku
			{
				OCR2 = MAX_SPEED;	//W³¹cz prawy silnik
			}
			if(left_sensors != 0)			//Je¿eli linia lest nad czterema lewymi czujnikami
			{
				OCR0 = MIN_SPEED;	//Wy³¹cz lewy silnik
			}
			else					//W przeciwnym wypadku
			{
				OCR0 = MAX_SPEED;	//W³¹cz lewy silnik
			}
			
			if(right_sensors != 0 && left_sensors != 0)	//Je¿eli linia jest nad prawymi i lewymi czujnikami
			{
				OCR0 = MAX_SPEED;	//W³¹cz oba silniki
				OCR2 = MAX_SPEED;
			}
			right_sensors = 0;	//Wyzeruj bufory czujników
			left_sensors = 0;
		}
		else	//Je¿eli robot jest w stanie czuwania
		{
			OCR2 = MIN_SPEED;	//Wy³¹cz oba silniki
			OCR0 = MIN_SPEED;
		}
    }
}

void TIM0_init(void)	//Za³¹czeniwe Timera0(PWM)
{
	TCCR0 |= (1<<WGM00) | (1<<WGM01);	//Tryb Fast PWM
	TCCR0 |= (1<<COM01) | (1<<COM00);	//Inverting mode
	TCCR0 |= (1<<CS00) | (1<<CS01);		//Preskaler: 64
}

void TIM1A_init(void)	//Za³¹czeniwe Timera1A(CTC)
{
	TCCR1A |= (1<<COM1A0);	//"Toggle OC1A/OC1B on compare match"
	TCCR1B |= (1<<WGM12);	//Tryb CTC
	TCCR1B |= (1<<CS10) | (1<<CS11);	//Preskaler: 64
	TIMSK |= (1<<OCIE1A);	//Zezwolenie na przerwanie porównania Timera1A
	OCR1A = 12499;
}

void TIM2_init(void)	//Za³¹czeniwe Timera2(PWM)
{
	TCCR2 |= (1<<WGM20) | (1<<WGM21);	//Tryb Fast PWM
	TCCR2 |= (1<<COM20) | (1<<COM21);	//Inverting mode
	TCCR2 |= (1<<CS22);					//Preskaler: 64
}

void INT1_init(void)	//Za³¹czenie przerwania INT1
{
	MCUCR |= (1<<ISC11);	//Przerwanie na zboczu opadaj¹cym
	GICR |= (1<<INT1);		//Zezwolenie na przerwanie zewnêtrzne INT1
}


//Wykrywanie stanów z pilota polega na "Odwiltrowaniu" sygna³ów o czêstotliwoœci wiêkszej ni¿ 4Hz.
//Sygna³ z odbiornika podczerwieni to kombinacja zakodowanych stanów niskich i wysokich (o czêstotliwoœci na pewno wiêkszej ni¿ 4Hz).
//Mój program ich nie rozkodowuje tylko "Przefiltrowuje" sam sygna³.
//Je¿eli od ostatniego zbocza opadaj¹cego nie minê³o 250ms Timer siê resetuje
ISR(INT1_vect)
{
	if(IR_signal == true)
	{
		Engine_switch == true ? Engine_switch = false : true;
		TIMERA_sig_triger = true;
	}

	IR_signal = false;

	TCNT1 = 0;
}


//Jako, ¿e nie chcia³em u¿yæ w programie funkcji "_delay_ms()"
//ustawi³em czêstotliwoœæ wywo³ywania przerwania 20Hz (dla animacji) i dla odczytywania sygna³u z pilota dzielê czêstotliwoœæ do 4Hz
ISR(TIMER1_COMPA_vect)
{
	static uint8_t j;	//Zmienna dziel¹ca czêstotliwoœæ Timera
	static uint8_t i;	//Zmienna iteruj¹ca klatki animacji na diodach
	
	//Odczytywanie stanu z pilota. Ci¹g dalszy.
	if (TIMERA_sig_triger && j == IR_TIMEOUT)
	{
		IR_signal = true;
		j = 0;
	}
	
	if(j == IR_TIMEOUT)
	{
		TIMERA_sig_triger = false;
	}
	
/////////////////////////////////////////////

	//Zawsze u¿ywana jest ta sama animacja na ledach, ale gdy robot jest w stanie czuwania co klatkê animacji jest czyszczony bufor
	if (i == WS_LEN)
	{
		i = 0;
	}
	
	if (!Engine_switch)
	{
		ws2812b_clear(WS_BUFF, WS_LEN);
	}
	ws2812b_set_pixel_color(WS_BUFF,i , 0, 0, BRIGHTNESS);
	ws2812b_set_pixel_color(WS_BUFF,23-i, BRIGHTNESS, 0, 0);

	if(i < 12)
	{
		ws2812b_set_pixel_color(WS_BUFF,i+12, 0, BRIGHTNESS, 0);
	}
	else
	{
		ws2812b_set_pixel_color(WS_BUFF,i-12 ,0, BRIGHTNESS, 0);
	}

	ws2812b_send(WS_BUFF, WS_LEN, WS_PIN);
	
	i++;
	j++;
}