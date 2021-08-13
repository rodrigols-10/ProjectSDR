/*
 * main.c
 *
 * Created: 8/12/2021 7:54:20 PM
 *  Author: rodrigo
 */ 

#include <xc.h>
#include <avr/io.h>
#include <math.h>

float Ts = 0.000216; //testar Ts = 0.000216 ---> como cada amostra demora 13 clocks do ADC, testar isso. prescale = 0.000016
float t = 0;
float pi = 3.14159;
int fp = 100;
float Tp;
int		adsignal = 0;
uint8_t	input=0;
uint8_t	output = 0;
uint8_t offset = 0;

int main(void)
{
	 Tp = 1/fp;
	DDRD = 0b11111111;
	DDRB = 0b11111111;

	DDRC = 0b00000000;
	ADMUX = 0b01000000;// Vcc como tensão de referência para os pinos analógicos, PC0 para conversão
	DIDR0 = 0b00111110;
	ADCSRB = 0b00000000;
	ADCSRA = 0b11100111; //liga o conversor, habilita auto-trigger, prescale = 128
	
    while(1)
    {
        //ADCSRA |= 1<<ADSC;    //Começa a conversão
        while(!(ADCSRA & 0b00010000)); //Espera o fim da conversão (Flag ADIF)
        adsignal = ADC; // Sinal recebido pelo conversor AD do ATmega328P (Está em auto-trigger, ou seja, está constantemente convertendo e jogando o resultado em ADC)
        input = adsignal / 4;	   //de 10 bits para 8 bits.
		
		output =(input - offset) * cos(2*pi*fp*t) + offset;
		
		PORTD = output;
		t += Ts;
		
		if(t >= Tp) t = 0; //Quando a portadora completar 1 período, reiniciamos o t, para não haver contagem infinita.
    }
}