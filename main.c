/*
This file is part of tempcontrol.

(c) Franco Sauvisky <francosauvisky@gmail.com>

This source file is subject to the 3-Clause BSD License that is bundled
with this source code in the file LICENSE.md
*/

#include <avr/io.h>

#define F_CPU 8000000UL
#include <util/delay.h>

void
uart_print(const char *string) // Simple function for printing a string
{
	for (; *string; string++)
	{
		loop_until_bit_is_set(UCSRA, UDRE);
		UDR = *string;
	}
}

void
uart_print_dec(uint8_t num) // Simple function for printing a 8-bit number
{
	uint8_t bcd[3];

	for(int8_t i = 2; i >= 0; i--)
	{
		bcd[i] = num % 10;
		num = num / 10;
	}

	for(uint8_t i = 0; i < 3; i++)
	{
		loop_until_bit_is_set(UCSRA, UDRE);
		UDR = '0' + bcd[i];
	}
}

void
uart_print_long_dec(uint16_t num, uint8_t sep) // Simple function for printing a 8-bit number
{
	uint8_t bcd[4];

	for(int8_t i = 3; i >= 0; i--)
	{
		bcd[i] = num % 10;
		num = num / 10;
	}

	for(uint8_t i = 0; i < 4; i++)
	{
		if(i == sep)
		{
			loop_until_bit_is_set(UCSRA, UDRE);
			UDR = '.';
		}
		loop_until_bit_is_set(UCSRA, UDRE);
		UDR = '0' + bcd[i];
	}
}

uint16_t
isqrt(uint32_t n) // Integer square root via Newton's Method
{
	uint32_t xn = n;

	for(uint8_t i = 0; i < 25; i++)
	{
		xn = (xn + n/xn)/2.0;
	}

	return (uint16_t) xn;
}

#define BAUD 19200
#include <util/setbaud.h>
void
setup_RS232(void)
{
	UBRRH = UBRRH_VALUE; // Sets UART baudrate value detemined by setbaud.h
	UBRRL = UBRRL_VALUE;
	#if USE_2X
	UCSRA |= (1 << U2X); // Use 2x prescaler if needed
	#else
	UCSRA &= ~(1 << U2X);
	#endif
	UCSRB |= _BV(TXEN); // Enables UART TX and RX
	UCSRB |= _BV(RXEN);
}

void
flush_UART_RX_buffer(void)
{
	while(bit_is_set(UCSRA, RXC))
	{
		char foo = UDR;
	}
}

uint16_t
getADC(uint16_t iter)
{	
	uint32_t ADCVal = 0;
	for(uint16_t i = 0; i < iter; i++)
	{
		ADCSRA |= _BV(ADSC);
		loop_until_bit_is_clear(ADCSRA, ADSC);
		ADCVal += ADCW;
	}

	return (uint16_t) (ADCVal/iter);
}

void
main (void)
{
	setup_RS232();
	flush_UART_RX_buffer();

	// Setting up ADC:
	ADMUX = _BV(REFS0) | _BV(REFS1); // ADC0 as input, VREF = 2.56V
	ADCSRA |= _BV(ADPS0) | _BV(ADPS2); // Clock prescaler = 1/64
	ADCSRA |= _BV(ADEN); // ADC Enable

	uint16_t temp_coeff_zero = 544; // Temperature at 0°C
	uint8_t temp_coeff_dx = 50; // Unit of -0.01°C per ADC unit
	// An simple way to calibrate: measure ADC values for ice and boiling water
	// so temp_coeff_dx = value_on_ice - value_on_boiling_water

	while(1)
	{
		uint16_t temp = (temp_coeff_zero - getADC(1000))*temp_coeff_dx/10;
		uart_print("Temp: ");
		uart_print_long_dec(temp, 3);
		uart_print("°C\r\n");
	}
}
