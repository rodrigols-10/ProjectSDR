/*
 * main.c
 *
 * Created: 8/12/2021 7:54:20 PM
 *  Author: rodri
 */ 

#include <xc.h>
#include <math.h>

float Ts = 0.000016;
float t = 0;
float pi = 3.14159;
int fp = 100;
int		adsignal = 0;
uint8_t	input=0;
uint8_t	output = 0;
uint8_t offset = 0;

int main(void)
{
	DDRD = 0b11111111;
	DDRB = 0b11111111;

	DDRC = 0b00000000;
	ADMUX = 0b01000000;// Vcc como tens�o de refer�ncia para os pinos anal�gicos, PC0 para convers�o
	ADCSRB = 0b00000000;
	DIDR0 = 0b00111110;
	ADCSRA = 0b10000111; //liga o conversor, prescale = 128
	
    while(1)
    {
        ADCSRA |= 1<<ADSC;    //Come�a a convers�o
        while(!(ADCSRA & 0b00010000)); //Espera o fim da convers�o (Flag ADIF)
        //adsignal = 511*cos(t)+511; //Simula��o de uma entrada cosenoidal com amplitude 2.5V e offset 2.5V na resolu��o do Conversor AD.
        adsignal = ADC; // Sinal recebido pelo conversor AD do ATmega328P (Est� em auto-trigger, ou seja, est� constantemente convertendo e jogando o resultado em ADC)
        input = adsignal / 4;	   //de 10 bits para 8 bits.
		
		output =(input - offset) * cos(2*pi*fp*t) + offset;
		
		PORTD = output;
		t += Ts;
    }
}