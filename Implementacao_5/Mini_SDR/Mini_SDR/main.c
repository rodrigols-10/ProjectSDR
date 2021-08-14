/*
 * main.c
 *
 * Created: 8/12/2021 7:54:20 PM
 *  Author: rodrigo
 */ 

#define F_CPU 16000000UL	//freq de trabalho da CPU

#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <xc.h>
#include <math.h>

float Ts = 0.00168;			//testar Ts = 0.000216 ---> como cada amostra demora 13 clocks do ADC, testar isso. prescale = 0.000016
float t = 0;
int fp = 100;
float pi = 3.14159;
float c=0;
uint8_t	input=0;			// Valor do sinal transformado em 8 bits
uint8_t	output = 0;			//Sinal de saida que sera usado no conversor D/A
uint8_t offset = 127;		//Offset em 2.5V

//VARIAVEIS DE AJUSTES
uint16_t adc_value = 0;
int AM=0, FM=0, ASK=0, FSK=0;

//LISTA DE FUNCOES
void adc_initiate();
uint16_t freq_limit (uint16_t adc_value);
void mod_select (uint16_t adc_value);

ISR(ADC_vect){
	adc_value = ADC;
}

int main(void)
{
	DDRD = 0b11111111;
	DDRB = 0b11111111;

	DDRC = 0b00000000;
	
	cli();			//DESABILITA INTERRUPCAO GLOBAL -- Necessario ao iniciar, pois força o bit correspondente para zero.
	adc_initiate();		
	sei();			// HABILITA INTERRUPCAO GLOBAL

	ASK=1;

    while(1)
    {
        // O valor de adc_value esta sendo coletado 
        input = adc_value / 4;	   //de 10 bits para 8 bits.
		c = fp * t;
		
		// -----------------------------
		//       MODULAÇÃO AM
		// -----------------------------	
		
		if(AM == 1)	output = ((input/2) * cos(2*pi*fp*t)) + offset;
		
		// -----------------------------
		//       MODULAÇÃO FM
		// -----------------------------
		
		if(FM == 1);
		
		// -----------------------------
		//       MODULAÇÃO ASK
		// -----------------------------
		
		if(ASK == 1){
			if(input < 10) output = offset; //Significa que a entrada está em zero. (caso o valor de zero fique com ruído)
			else {
			output = (input/2) * cos(2*pi*fp*t) + offset;
			}
		}
		
		// -----------------------------
		//       MODULAÇÃO FSK
		// -----------------------------
		
		if(FSK == 1);
		
		//SOMENTE TESTES DA PORTADORA
		//output = offset* cos(2*pi*fp*t) + offset ; Somente a portadora na saída
		
		// -----------------------------
		//       SAIDA
		// -----------------------------
		
		PORTD = output;
		
		t += Ts;
		if(c >= 1) t = 0; //Quando a portadora completar 1 período, reiniciamos o t, para não haver contagem infinita.
	
		//while(!(ADCSRA & 0b00010000)); //Espera o fim da conversão (Flag ADIF)
		//ADCSRA &= 0b11101111;
		
		
    }
}

void adc_initiate(){
	
	/* definir o reg de controle e status em 0b11100111
		
		ADEN=1	Enable do ADC
		ADSC=1	Permite início da conversão
		ADATE=1	Auto trigger enable bit
		ADIF=0	Flag que sinaliza fim da conversão quando em 1
		ADIE=1	Habilita interrupções CORREÇÃO DE PROJETO
		ADPS[2:0]=1	Seta o prescale em 128
	*/
	ADCSRA = 0xEF;
	ADCSRB = 0x00;		// Seleciona modo FREE RUNNING
		
	/* definir o reg de seleção do mux em 0b01000000
		
		REFS1=0	e REFS0=1		Seleciona o Vcc como Vref do conversor
		ADALAR=0			Seleciona ajuste à direita do resultado no reg de dados
		MUX[3:0]=0			Seleciona ADC0 como canal a ser lido pelo ADC
	*/	
	ADMUX = 0x40;
	
	DIDR0 = 0b00111110;		// HABILITA PINO PC0 COMO ENTRADA DO ADC0
}

//Funcao que retorna o valor do potenciometro com as limitacoes [100 <= valor <= 999]
uint16_t freq_limit (uint16_t adc_value){
	uint16_t l_freq = 100;
	uint16_t h_freq = 999;
	
	if(adc_value <= l_freq) return l_freq;
	else if (adc_value >= h_freq) return h_freq;
	else return adc_value;
}

//Funcao seletora do tipo de modulacao usando o potenciometro como parametro.
void mod_select (uint16_t adc_value){
	if(adc_value >= 0 && adc_value <= 255){
		AM = 1;
		FM = 0;
		ASK = 0;
		FSK = 0;
	}
	else if(adc_value >= 256 && adc_value <= 510){
		AM = 0;
		FM = 1;
		ASK = 0;
		FSK = 0;
	}
	else if(adc_value >= 511 && adc_value <= 765){
		AM = 0;
		FM = 0;
		ASK = 1;
		FSK = 0;
	}
	else if(adc_value >= 766 && adc_value <= 1023){
		AM = 0;
		FM = 0;
		ASK = 0;
		FSK = 1;
	}
}