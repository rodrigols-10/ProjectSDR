/*
 * main.c
 *
 * Created: 8/12/2021 7:54:20 PM
 *  Author: rodrigo
 */ 

#include <xc.h>
#include <avr/io.h>
#include <math.h>

float Ts = 0.00168; //testar Ts = 0.000216 ---> como cada amostra demora 13 clocks do ADC, testar isso. prescale = 0.000016
float t = 0;    // N�O TENHO IDEIA DO QUE EST� ACONTECENDO
float pi = 3.14159;
int fp = 200;
float c=0;
int		adsignal = 0;
uint8_t	input=0;
uint8_t	output = 0;
uint8_t offset = 127; //2.5V

int main(void)
{
	DDRD = 0b11111111;
	DDRB = 0b11111111;

	DDRC = 0b00000000;
	ADMUX = 0b01000000;// Vcc como tens�o de refer�ncia para os pinos anal�gicos, PC0 para convers�o
	DIDR0 = 0b00111110;
	ADCSRB = 0b00000000;
	ADCSRA = 0b10000111; //liga o conversor, prescale = 128
	ADCSRA |= 1<<ADSC;    //Come�a a convers�o
	while(!(ADCSRA & 0b00010000)); //Espera o fim da convers�o (Flag ADIF)
    while(1)
    {
		ADCSRA |= 1<<ADSC;    //Come�a a convers�o
        
		adsignal = ADC; // Sinal recebido pelo conversor AD do ATmega328P (Est� em auto-trigger, ou seja, est� constantemente convertendo e jogando o resultado em ADC)
        input = adsignal / 4;	   //de 10 bits para 8 bits.
		c = fp * t;
				
		// output = (input - offset) * cos(2*pi*c) + offset;
		
		output = ((input/2) * cos(2*pi*fp*t)) + offset; // ESSE AQUI!!! AM	
		
		//output = ((offset) * cos(2*pi*fp*t)) + offset; 
		
		//output = offset* cos(2*pi*fp*t) + offset ; Somente a portadora na sa�da
		
		
		PORTD = output;
		
		t += Ts;
		if(c >= 1) t = 0; //Quando a portadora completar 1 per�odo, reiniciamos o t, para n�o haver contagem infinita.
	
		while(!(ADCSRA & 0b00010000)); //Espera o fim da convers�o (Flag ADIF)
		ADCSRA &= 0b11101111;
		
		
    }
}