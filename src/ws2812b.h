#ifndef WS2812B_H_
#define WS2812B_H_

#define WS_PIN (1<<PORTB0)	//Sta³e u³atwiaj¹ce pracê
#define WS_DDR DDRB
#define WS_PORT PORTB
#define WS_LEN 24


typedef struct	//Struktura bufora
{
	uint8_t g;
	uint8_t r;
	uint8_t b;
} TWS_RGB;


void ws2812b_send(void* tab, uint16_t datlen, uint8_t pin);	//Wys³anie danych z bufora do diod
void ws2812b_set_pixel_color(void* tab, uint16_t x, uint8_t red, uint8_t green, uint8_t blue);	//Ustawienie koloru i-tej diody
void ws2812b_fill(void* tab, uint16_t datalen, uint8_t red, uint8_t green, uint8_t blue);	//Funkcja do wype³nienia bufora diod
void ws2812b_clear(void* tab, uint16_t datalen);	//Wyczyœæ bufor

#endif /* WS2812B_H_ */
