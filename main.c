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

// Using multiplier != 1 is useful for oversampling
// If no oversampling is requires, use multiplier = 1, equivalent to an arithmetic mean
uint32_t
getADC(uint16_t iter, uint16_t multiplier)
{	
	uint32_t ADCVal = 0;
	for(uint32_t i = 0; i < iter; i++)
	{
		ADCSRA |= _BV(ADSC);
		loop_until_bit_is_clear(ADCSRA, ADSC);
		ADCVal += ADCW;
	}

	return (ADCVal/(iter/multiplier));
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

	uint16_t temp_coeff_zero = 544; // ADC value at 0°C
	uint8_t temp_coeff_dx = 50; // Unit of -0.01°C per ADC unit
	// Two-point calibration: measure ADC values V1, V2 on temperatures T1, T2 respectively
	// temp_coeff_dx = (T1 - T2)/(V2 - V1)
	// temp_coeff_zero = (V1*T2 - V2*T1)/(T2 - T1)

	while(1)
	{
		// unit(temp_coeff_zero) = ADC_unit
		// unit(getADC(..., multiplier)) = ADC_unit/multiplier
		// So we need to multiply temp_coeff_zero by multiplier before taking the difference

		// unit(temp_coeff_dx) = 0.01 K/ADC_unit
		// unit( (multiplier*temp_coeff_zero - getADC(..., multiplier)) * temp_coeff_dx ) = 0.01K / multiplier
		// And we need to divide temp by 10*multiplier to get 0.1 K

		// Seems contradictory, but what happens is that physically we want to multiply by 1 (as we don't want
		// to change the physical value), so, for example, when we multiply temp_coeff_zero by "multiplier",
		// the units of that constant is 1/multiplier, equivalent to a physical 1 (multiplicative identity)

		uint16_t temp = ( 10*temp_coeff_zero - getADC(1000, 10) ) * temp_coeff_dx/100; // unit: 0.1°C

		uart_print("Temp: ");
		uart_print_long_dec(temp, 3);
		uart_print("°C\r\n");
	}
}
