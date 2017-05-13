#include <avr/io.h>

#include "ws2812b.h"

//     bit = 0				    bit = 1
//  +----+        |        +--------+    |
//  |    |        |        |        |    |
//  |    |        |        |        |    |
//  |    |        |        |        |    |
//  |    |        |        |        |    |
//  |    +--------+        |        +----+
//  400ns   800ns			 800ns   400ns


//!!! Uwaga! Obs³uga diod WS2812B jest zaimplementowana tylko dla procesorów o taktowaniu 16MHz !!!
void ws2812b_send(void *tab, uint16_t datalen, uint8_t pin)	//Funkcja wysy³aj¹ce dane z bufora
{
	uint8_t databyte = 0;	//Zmienne potrzebne dla algorytmu
	uint8_t cnt = 0;		//--|--
	uint8_t pin_HI = pin;	//--|--
	uint8_t pin_LO = ~pin;	//--|--

	WS_DDR |= pin;	//Ustawienie pinu

	datalen = datalen * 3;	//Obliczenie iloœci bitów do wys³ania
	asm volatile("			lds %[cnt], %[ws_PORT]		\n\t"	//Wysy³anie bitów kolorów
				 "			or %[pin_HI], %[cnt]		\n\t"	//Ta czêœæ nie mog³a byæ napisana w C, poniewa¿ taka procedura zajmowa³aby zbyt wiele czasu
				 "			and %[pin_LO], %[cnt]		\n\t"	//Aczkolwiek mo¿na (ale to jest nie zalecane) napisaæ procedurê w C wys³aj¹c¹ dane do diod
				 "mPTL%=:	dec %[datalen]				\n\t"	//Lecz przy ich wiêkszej iloœci wystêpowa³yby b³êdy w transmisji
				 "			breq nPTL%=					\n\t"
				 "			ld %[databyte], X+			\n\t"
				 "			ldi %[cnt], 8				\n\t"

				 "oPTL%=:	sts %[ws_PORT], %[pin_HI]	\n\t"
				 "			lsl %[databyte]				\n\t"
				 "			rjmp .+0					\n\t"
				 "			brcs .+2					\n\t"
				 "			sts %[ws_PORT], %[pin_LO]	\n\t"
				 "			rjmp .+0					\n\t"
				 "			rjmp .+0					\n\t"
				 "			dec %[cnt]					\n\t"
			 	 "			sts %[ws_PORT], %[pin_LO]	\n\t"
				 "			breq mPTL%=					\n\t"
				 "			rjmp .+0					\n\t"
				 "			rjmp oPTL%=					\n\t"
				 "nPTL%=:								\n\t"
				:[cnt]"=&d" (cnt)
				:[pin_HI]"r" (pin_HI),
				 [pin_LO]"r" (pin_LO),
				 [datalen]"r" (datalen),
				 [ws_PORT]"I" (_SFR_MEM_ADDR(WS_PORT)),
				 [tab]"x" (tab),
				 [pin]"r" (pin),
				 [databyte]"r" (databyte)
				 );
}

void ws2812b_set_pixel_color(void* tab, uint16_t x, uint8_t red, uint8_t green, uint8_t blue)	//Funkcja wpisuj¹ca kolor i-tej diody do bufora
{
	*((uint8_t*)tab + x*3) = green;		//Wpisanie odpowiednich kolorów do bufora
	*((uint8_t*)tab + x*3+1) = red;		//--|--
	*((uint8_t*)tab + x*3+2) = blue;	//--|--
}

void ws2812b_fill(void* tab, uint16_t datalen, uint8_t red, uint8_t green, uint8_t blue)	//Funkcja wype³niaj¹ca ca³y bufor jedn¹ wartoœci¹
{
	for(uint8_t i = 0; i < datalen; i++)
	{
		*((uint8_t*)tab + i*3) = green;	//Wpisanie do i-tego bajtu wartoœci danego koloru
		*((uint8_t*)tab + i*3+1) = red;	//--|--
		*((uint8_t*)tab + i*3+2) = blue;//--|--
	}
}

void ws2812b_clear(void* tab, uint16_t datalen)	//Funkcja czyszcz¹ca bufor
{
	ws2812b_fill(tab, datalen, 0, 0, 0);	//Wype³nianie bufora zerami
}
