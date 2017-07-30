#define F_CPU 16000000UL	//Cz�stotliwo�� mikrokontrolera; 16MHz

#include <avr/io.h>			//Biblioteka ze zdefiniowanymi adresami wszystkich rejestr�w
#include <avr/interrupt.h>	//Biblioteka do obs�ugi przerwa� w j�zyku C
#include <stdbool.h>		//Biblioteka ze zdefiniowanym typem _Bool
#include "ws2812b.h"		//Autorska biblioteka do obs�ugi diod adresowalnych WS2812B

#define MAX_SPEED	50	//Pr�dko�� silnik�w
#define MIN_SPEED	255	//0 - pe�na moc silnik�w, 255 - wy��czenie silnik�w

#define BRIGHTNESS 255	//Jasno�� diod 0-255

#define IR_TIMEOUT 5	//Timeout(s) = 1 / (20 / IR_TIMEOUT); Timeout jest potrzebny do odczytywania danych z pilota

TWS_RGB WS_BUFF[WS_LEN];	//Definicja bufora

volatile _Bool Engine_switch = false;	//Zmienne odpowiadaj�ce za odczytywany stan z pilota
volatile _Bool IR_signal = true;		//--|--
volatile _Bool TIMERA_sig_triger = true;//Emulowany prescaler "Wy��czaj�cy" Timer w algorytmie pilota

void INT1_init(void);	//Za��czenie przerwania INT1
void TIM0_init(void);	//Za��czeniwe Timera0(PWM)
void TIM1A_init(void);	//Za��czeniwe Timera1A(CTC)
void TIM2_init(void);	//Za��czeniwe Timera2(PWM)

int main(void)
{
	uint8_t right_sensors = 0;	//Zmienne przechowuj�ce stan czujnik�w
	uint8_t left_sensors = 0;	//--|--
	
	DDRA = 0x00;	//PORTA jako wej�cie (czujniki)
	PORTA = 0xFF;	//Podci�gni�cie pin�w z wyj�ciami czujnik�w do VCC
	
	DDRC |= (1<<PORTC1) | (1<<PORTC0);	//PC0..1 do sterowania silnikami(lewy)
	DDRD |= (1<<PORTD2) | (1<<PORTD6) | (1<<PORTD7);	//PD6..2 do sterowania silnikami, PD7 do sterowania PWM(lewy)
	DDRB |= (1<<PORTB3) | (1<<PORTB0);	//PB0 jako wyj�cie danych do LED-�w | PB3(OC0) jako wyj�cie PWM (Lewy)
	DDRD &= ~(1<<PORTD3);	//PB3 jako wej�cie(INT1)
	
	PORTC |= (1<<PORTC0);	//Wystaw stan wysoki na pinie PC0
	PORTD |= (1<<PORTD6) | (1<<PORTD7) | (1<<PORTD3);	//Pinu PD6..7 s� w stanie wysokim, pin PD3 podci�gni�ty do VCC
	PORTB |= (1<<PORTB3);	//Stan wysoki na pinie PB3
	
	INT1_init();	//Za��czenie przerwania INT1
	TIM0_init();	//Za��czeniwe Timera0(PWM)
	TIM1A_init();	//Za��czeniwe Timera1A(CTC)
	TIM2_init();	//Za��czeniwe Timera2(PWM)

	sei();	//Zezwolenie na globalne przerwania
	
    while(1) 
    {
		if(Engine_switch)	//Je�li robot jest w stanie aktywnym
		{			
			right_sensors = (~PINA & 0x0F);	//Sczytaj warto�ci z czyjnik�w do zmiennych left_sensors i right_sensors
			left_sensors = (~PINA & 0xF0);
			
			if(right_sensors != 0)			//Je�eli linia lest nad czterema prawymi czujnikami
			{
				OCR2 = MIN_SPEED;	//Wy��cz prawy silnik	
			}
			else					//W przeciwnym wypadku
			{
				OCR2 = MAX_SPEED;	//W��cz prawy silnik
			}
			if(left_sensors != 0)			//Je�eli linia lest nad czterema lewymi czujnikami
			{
				OCR0 = MIN_SPEED;	//Wy��cz lewy silnik
			}
			else					//W przeciwnym wypadku
			{
				OCR0 = MAX_SPEED;	//W��cz lewy silnik
			}
			
			if(right_sensors != 0 && left_sensors != 0)	//Je�eli linia jest nad prawymi i lewymi czujnikami
			{
				OCR0 = MAX_SPEED;	//W��cz oba silniki
				OCR2 = MAX_SPEED;
			}
			right_sensors = 0;	//Wyzeruj bufory czujnik�w
			left_sensors = 0;
		}
		else	//Je�eli robot jest w stanie czuwania
		{
			OCR2 = MIN_SPEED;	//Wy��cz oba silniki
			OCR0 = MIN_SPEED;
		}
    }
}

void TIM0_init(void)	//Za��czeniwe Timera0(PWM)
{
	TCCR0 |= (1<<WGM00) | (1<<WGM01);	//Tryb Fast PWM
	TCCR0 |= (1<<COM01) | (1<<COM00);	//Inverting mode
	TCCR0 |= (1<<CS00) | (1<<CS01);		//Preskaler: 64
}

void TIM1A_init(void)	//Za��czeniwe Timera1A(CTC)
{
	TCCR1A |= (1<<COM1A0);	//"Toggle OC1A/OC1B on compare match"
	TCCR1B |= (1<<WGM12);	//Tryb CTC
	TCCR1B |= (1<<CS10) | (1<<CS11);	//Preskaler: 64
	TIMSK |= (1<<OCIE1A);	//Zezwolenie na przerwanie por�wnania Timera1A
	OCR1A = 12499;
}

void TIM2_init(void)	//Za��czeniwe Timera2(PWM)
{
	TCCR2 |= (1<<WGM20) | (1<<WGM21);	//Tryb Fast PWM
	TCCR2 |= (1<<COM20) | (1<<COM21);	//Inverting mode
	TCCR2 |= (1<<CS22);					//Preskaler: 64
}

void INT1_init(void)	//Za��czenie przerwania INT1
{
	MCUCR |= (1<<ISC11);	//Przerwanie na zboczu opadaj�cym
	GICR |= (1<<INT1);		//Zezwolenie na przerwanie zewn�trzne INT1
}


//Wykrywanie stan�w z pilota polega na "Odwiltrowaniu" sygna��w o cz�stotliwo�ci wi�kszej ni� 4Hz.
//Sygna� z odbiornika podczerwieni to kombinacja zakodowanych stan�w niskich i wysokich (o cz�stotliwo�ci na pewno wi�kszej ni� 4Hz).
//M�j program ich nie rozkodowuje tylko "Przefiltrowuje" sam sygna�.
//Je�eli od ostatniego zbocza opadaj�cego nie min�o 250ms Timer si� resetuje
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


//Jako, �e nie chcia�em u�y� w programie funkcji "_delay_ms()"
//ustawi�em cz�stotliwo�� wywo�ywania przerwania 20Hz (dla animacji) i dla odczytywania sygna�u z pilota dziel� cz�stotliwo�� do 4Hz
ISR(TIMER1_COMPA_vect)
{
	static uint8_t j;	//Zmienna dziel�ca cz�stotliwo�� Timera
	static uint8_t i;	//Zmienna iteruj�ca klatki animacji na diodach
	
	//Odczytywanie stanu z pilota. Ci�g dalszy.
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

	//Zawsze u�ywana jest ta sama animacja na ledach, ale gdy robot jest w stanie czuwania co klatk� animacji jest czyszczony bufor
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