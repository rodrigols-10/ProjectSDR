/*
 * main.c
 *
 * Created: 8/21/2021 7:51:24 PM
 *  Author: rodri
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <xc.h>


//uint8_t fim = 0;

void UART_config(unsigned int ubrr)
{
	//Baud Rate de 9600bps para um cristal de 16MHz (Datasheet)
	//UBRR0 = 103;
	
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	
	//Habilita os pinos TX e RX
	UCSR0B =  (1<<RXEN0) | (1<<TXEN0) ;
	
	//Configura a UART com 8 bits de dados
	UCSR0C =  (1<<UCSZ01) | (1<<UCSZ00);
}

void USART_Transmit( unsigned char data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) );
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

void UART_enviaString(char *s)
{
	unsigned int i=0;
	while (s[i] != '\x0')
	{
		USART_Transmit(s[i++]);
	};
}

void Timer(){
	OCR1A = 62500;//1952;
	TIMSK1 = (1 << OCIE1A);				// Habilita interrupcao pelo match com o comparador A
	TCCR1B = 0x0C;						// Modo: 4 CTC , Prescale: 256
}

ISR(TIMER1_COMPA_vect){
	   PORTB ^= 0X01;
}

int main(void)
{
	DDRB = 0X01;
	//unsigned char data;
	char s[] = "a porra ta seria";
	int baud = 103;
	UART_config(baud);
	cli();
	Timer();
	sei();
	UART_enviaString(s);
	
	
    while(1)
    {
    }
}