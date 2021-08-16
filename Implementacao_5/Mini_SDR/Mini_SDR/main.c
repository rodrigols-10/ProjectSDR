/*
 * main.c
 *
 * Created: 8/12/2021 7:54:20 PM
 *  Author: rodrigo & Marcelo & Joao Matheus Bernardo Resende & Wesley Brito & Anny Pinheiro
 */ 

//#define F_CPU 16000000UL	//freq de trabalho da CPU

//#include <util/delay.h>
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
uint8_t Ap = 127;			//Amplitude da portadora em 2.5V
uint8_t new_msg = 0;		//Mensagem
char msg_bin[8] = {'0','0','0','0','0','0','0','0'}; //Mensagem Binaria

//VARIAVEIS DE AJUSTES
uint16_t adc_value = 0;
int mod=1; //Inicia em AM
int un = 0;
int de = 0;
int ce = 0;

char VL = '0';
char VH = '0';
int cont_bit=0;
int contou=0;

float Tin=0;
int Fin=0;

//LISTA DE FUNCOES
void modulacao();
void portadora();
void adc_initiate();
uint16_t freq_limit (uint16_t adc_value);
void mod_select (uint16_t adc_value);

void delay_1();
void lcd_cmd(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_adress(unsigned char adress);
void lcd_init();
void lcd_default();
void lcd_mod(int mod);
void lcd_off_cursor();
void lcd_on_cursor();
void lcd_port(uint16_t n);
void lcd_number(int n);
void separate_digit(uint16_t n);
void lcd_calc();
void lcd_R_analog();
void lcd_R_digit(char n[8], int fb);

ISR(ADC_vect){
	adc_value = ADC;
	//conta quantos Ts passaram pra um clock
	if(mod<=2){ //Para modulações Analogicas. mod=1 (AM), mod=2 (FM)
		if (adc_value <= 511)
		{
			Tin += Ts;
			contou = 1;
		} else {
			//Verificando frequencia de entrada
			if (contou == 1)
			{
				if(Tin!=0){
					Fin = 2/Tin;
				} else {
					Fin=0;
				}
				contou = 0;
				Tin=0;
			}
			
		}
	}
	if(mod>=3){ //Para modulações Digitais. mod=3 (ASK), mod=4 (FSK)
		
		Tin+=Ts; //Conta o periodo da taxa de bits para calcular a frequencia
		
		if(!(PINC & (1<<PINC5))){ // Se o clock da entrada estiver em LOW
			if (adc_value <= 10)	  // Sinal de entrada com valor muito baixo (usar 0 em circuito real poderia não funcionar com ruído)
			{
				VL = '0';
			} else{
				VL = '1';
			}
			contou=1;
		} else {				// Se o clock da entrada estiver em HIGH
			VH = VL;
			if(contou==1){
				msg_bin[cont_bit] = VH;
				cont_bit++;
				contou=0;
				if(cont_bit==8)cont_bit=0;
				
				//Verificando taxa de bits
				if(Tin!=0){
					Fin = 1/Tin;
				} else {
					Fin=0;
				}
				Tin=0;
			}
		}
	}
}

int main(void)
{
	DDRD = 0b11111111;
	DDRB = 0b00111111;

	DDRC = 0b00000000;
	PORTC = 0b00000000;
	PORTB = 0b00000000;
	
	cli();			//DESABILITA INTERRUPCAO GLOBAL -- Necessario ao iniciar, pois força o bit correspondente para zero.
	adc_initiate();		
	sei();			// HABILITA INTERRUPCAO GLOBAL
	
	lcd_init();								//Init LCD
	lcd_off_cursor();
	//lcd_on_cursor();
	lcd_default();
	lcd_mod(1);
	

    while(1)
    {
        // O valor de adc_value esta sendo coletado 
        input = adc_value / 4;	   //de 10 bits para 8 bits.
		c = fp * t;
		
		// -----------------------------
		//       MODULAÇÃO AM
		// -----------------------------	
		
		if(mod == 1)	output = ((input/2) * cos(2*pi*fp*t)) + offset;
		
		// -----------------------------
		//       MODULAÇÃO FM
		// -----------------------------
		
		if(mod == 2)	output =  Ap * cos(2*pi*t*(fp + input))+offset;
		
		// -----------------------------
		//       MODULAÇÃO ASK
		// -----------------------------
		
		if(mod == 3){
			if(input < 10) output = offset; //Significa que a entrada está em zero. (caso o valor de zero fique com ruído)
			else {
			output = (input/2) * cos(2*pi*fp*t) + offset;
			}
		}
		
		// -----------------------------
		//       MODULAÇÃO FSK
		// -----------------------------
		
		if(mod == 4){
			if(input < 10) output = Ap * cos(2*pi*fp*t) + offset; //Significa que a entrada está em zero. (caso o valor de zero fique com ruído)
			else {
			output = Ap * cos(4*pi*fp*t) + offset; //Dobra a frequência da portadora
			}
		}
		
		//SOMENTE TESTES DA PORTADORA
		//output = offset* cos(2*pi*fp*t) + offset ; Somente a portadora na saída
		
		// -----------------------------
		//       SAIDA
		// -----------------------------
		PORTD = output;
		t += Ts;
		if(c >= 1) t = 0; //Quando a portadora completar 1 período, reiniciamos o t, para não haver contagem infinita.
		
		if(mod<=2){ //mod=1 (AM) ou mod=2 (FM)
		lcd_R_analog(Fin);
		//new_msg = input*(50/255);
		//separate_digit(new_msg);
		}
		if(mod>=3){ //mod=3 (ASK) ou mod=4 (FSK)
			lcd_R_digit(msg_bin,Fin);
		}
		
		
		// -----------------------------
		//       BOTAO M
		// -----------------------------		
		
		if(PINC & (1<<PINC2)){
//		<<< pre-configuracao de modulacao aqui >>>
			ADMUX |= 0x01;
			while (!(PINC & (1<<PINC3))) //Botão P
			{
				modulacao();
			}
//		<<< pre-configuracao de portadora aqui >>>
			while (!(PINC & (1<<PINC4))) //Botão R
			{
				portadora();
			}
			lcd_mod(mod);
			lcd_calc();
			ADMUX &= 0xFE;	//Muda o canal do ADC para o canal 0
		}
		
    }
}

void modulacao()
{
	mod_select (adc_value);
}

void portadora()
{
	fp = freq_limit (adc_value);
	lcd_port(fp);
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
	
	//DIDR0 = 0b00111100;		// HABILITA PINO PC0 COMO ENTRADA DO ADC0
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
		mod = 1;
		lcd_mod(1);
	}
	else if(adc_value >= 256 && adc_value <= 510){
		mod = 2;
		lcd_mod(2);
	}
	else if(adc_value >= 511 && adc_value <= 765){
		mod = 3;
		lcd_mod(3);
	}
	else if(adc_value >= 766 && adc_value <= 1023){
		mod = 4;
		lcd_mod(4);
	}
}

//----------------------------------------------------------
//						DISPLAY LCD
//----------------------------------------------------------

void delay_1(){
	for(int i = 0;i<200;i++){}
}

void lcd_cmd(unsigned char cmd){
	
	PORTB &= 0xF0;								// Mask preservation 4 LSBs
	PORTB |= cmd;								// Add data command
	
	PORTB &= ~(1<<PORTB5);						// Set RS = 0
	PORTB |= (1<<PORTB4);						// Set E = 1
	delay_1();
	PORTB &= ~(1<<PORTB4);						// Set E = 0

}

void lcd_data(unsigned char data){
	
	PORTB |= (1<<PORTB5);						// Set RS = 1
	
	for(int i=0; i<2 ;i++){						// Send two data (2x 4 bits)
		if(i==0){
			
			char MSB_data = data >> 4;			// Shift right 4 Bits MSB >> LSB
			PORTB &= 0xF0;						// Mask preservation 4 MSBs
			PORTB |= MSB_data;					// Add data
			
			PORTB |= (1<<PORTB4);				// Set E = 1
			delay_1();
			PORTB &= ~(1<<PORTB4);				// Set E = 0
		}
		
		else{
			PORTB &= 0xF0;						// Mask preservation 4 LSBs
			PORTB |= data;						// Add data
			
			PORTB |= (1<<PORTB4);				// Set E = 1
			delay_1();
			PORTB &= ~(1<<PORTB4);				// Set E = 0
		}
	}
	PORTB &= ~(1<<PORTB5);						// Set RS = 0

}
void lcd_adress(unsigned char adress){
	
	PORTB &= ~(1<<PORTB5);						// Set RS = 0
	
	for(int i=0; i<2 ;i++){						// Send two data (2x 4 bits)
		if(i==0){
			
			char MSB_adress = adress >> 4;		// Shift right 4 Bits MSB >> LSB
			PORTB &= 0xF0;						// Mask preservation 4 MSBs
			PORTB |= MSB_adress;				// Add data
			
			PORTB |= (1<<PORTB4);				// Set E = 1
			delay_1();
			PORTB &= ~(1<<PORTB4);				// Set E = 0
		}
		
		else{
			PORTB &= 0xF0;						// Mask preservation 4 LSBs
			PORTB |= adress;						// Add data
			
			PORTB |= (1<<PORTB4);				// Set E = 1
			delay_1();
			PORTB &= ~(1<<PORTB4);				// Set E = 0
		}
	}
	PORTB &= ~(1<<PORTB5);					// Set RS = 0
}
void lcd_init(){
	
	lcd_cmd(0x02);							//RS = 0 D7:D4 = 0010
	lcd_cmd(0x02);							//RS = 0 D7:D4 = 0010
	lcd_cmd(0x08);							//RS = 0 D7:D4 = 1000
	
}

void lcd_off_cursor(){

	lcd_cmd(0x00);							//RS = 0 D7:D4 = 0000
	lcd_cmd(0x0C);							//RS = 0 D7:D4 = 1100
}

void lcd_on_cursor(){
	
	lcd_cmd(0x00);							//RS = 0 D7:D4 = 0000
	lcd_cmd(0x0F);							//RS = 0 D7:D4 = 1100
}

void lcd_default(){
	
	lcd_adress(0x80);
	lcd_data(0x4D);							//M
	lcd_adress(0x81);
	lcd_data(0x6F);							//o
	lcd_adress(0x82);
	lcd_data(0x64);							//d
	lcd_adress(0x83);
	lcd_data(0x3A);							//:
	
	lcd_adress(0xC0);
	lcd_data(0x4D);							//M
	lcd_adress(0xC1);
	lcd_data(0x73);							//s
	lcd_adress(0xC2);
	lcd_data(0x67);							//g
	lcd_adress(0xC3);
	lcd_data(0x3A);							//:
}

void lcd_mod(int mod){
	switch(mod){
		case 1:								//AM
		lcd_adress(0x85);
		lcd_data(0x41);
		
		lcd_adress(0x86);
		lcd_data(0x4D);
		
		lcd_adress(0x87);
		lcd_data(0x01);
		
		lcd_adress(0x89);
		lcd_data(0x46);					//F
		lcd_adress(0x8A);
		lcd_data(0x3A);					//:
		
		lcd_adress(0X8D);
		lcd_data(0xB0);					//-
		lcd_adress(0X8C);
		lcd_data(0xB0);					//-
		lcd_adress(0X8B);
		lcd_data(0xB0);					//-
		
		lcd_adress(0x8E);
		lcd_data(0x48);					//H
		lcd_adress(0x8F);
		lcd_data(0x7A);					//z
		
		lcd_adress(0XC5);
		lcd_data(0xB0);					//-
		lcd_adress(0XC6);
		lcd_data(0xB0);					//-
		lcd_adress(0XC7);
		lcd_data(0xB0);					//-
		lcd_adress(0XC8);
		lcd_data(0x01);					//
		lcd_adress(0XC9);
		lcd_data(0x01);					//
		lcd_adress(0XCA);
		lcd_data(0x01);					//
		lcd_adress(0XCB);
		lcd_data(0x01);					//
		lcd_adress(0XCC);
		lcd_data(0x01);					//
		lcd_adress(0XCD);
		lcd_data(0x01);					//
		lcd_adress(0XCE);
		lcd_data(0x01);					//
		lcd_adress(0XCF);
		lcd_data(0x01);					//
		
		break;
		
		case 2:								//FM
		lcd_adress(0x85);
		lcd_data(0x46);
		
		lcd_adress(0x86);
		lcd_data(0x4D);
		
		lcd_adress(0x87);
		lcd_data(0x01);
		lcd_adress(0x89);
		
		lcd_adress(0x89);
		lcd_data(0x46);					//F
		lcd_adress(0x8A);
		lcd_data(0x3A);					//:
		
		lcd_adress(0X8D);
		lcd_data(0xB0);					//-
		lcd_adress(0X8C);
		lcd_data(0xB0);					//-
		lcd_adress(0X8B);
		lcd_data(0xB0);					//-
		
		lcd_adress(0x8E);
		lcd_data(0x48);					//H
		lcd_adress(0x8F);
		lcd_data(0x7A);					//z
		
		lcd_adress(0XC5);
		lcd_data(0xB0);					//-
		lcd_adress(0XC6);
		lcd_data(0xB0);					//-
		lcd_adress(0XC7);
		lcd_data(0xB0);					//-
		lcd_adress(0XC8);
		lcd_data(0x01);					//
		lcd_adress(0XC9);
		lcd_data(0x01);					//
		lcd_adress(0XCA);
		lcd_data(0x01);					//
		lcd_adress(0XCB);
		lcd_data(0x01);					//
		lcd_adress(0XCC);
		lcd_data(0x01);					//
		lcd_adress(0XCD);
		lcd_data(0x01);					//
		lcd_adress(0XCE);
		lcd_data(0x01);					//
		lcd_adress(0XCF);
		lcd_data(0x01);					//
		break;
		
		case 3:								//ASK
		lcd_adress(0x85);
		lcd_data(0x41);
		
		lcd_adress(0x86);
		lcd_data(0x53);
		
		lcd_adress(0x87);
		lcd_data(0x4B);
		
		lcd_adress(0x89);
		lcd_data(0x54);					//T
		lcd_adress(0x8A);
		lcd_data(0x3A);					//:
		
		lcd_adress(0X8D);
		lcd_data(0xB0);					//-
		lcd_adress(0X8C);
		lcd_data(0xB0);					//-
		lcd_adress(0X8B);
		lcd_data(0xB0);					//-
		
		lcd_adress(0x8E);
		lcd_data(0x62);					//b
		lcd_adress(0x8F);
		lcd_data(0x73);					//s
		
		lcd_adress(0XC5);
		lcd_data(0xB0);					//-
		lcd_adress(0XC6);
		lcd_data(0xB0);					//-
		lcd_adress(0XC7);
		lcd_data(0x2E);					//.
		lcd_adress(0XC8);
		lcd_data(0xB0);					//-
		lcd_adress(0XC9);
		lcd_data(0xB0);					//-
		lcd_adress(0XCA);
		lcd_data(0x2E);					//.
		lcd_adress(0XCB);
		lcd_data(0xB0);					//-
		lcd_adress(0XCC);
		lcd_data(0xB0);					//-
		lcd_adress(0XCD);
		lcd_data(0x2E);					//.
		lcd_adress(0XCE);
		lcd_data(0xB0);					//-
		lcd_adress(0XCF);
		lcd_data(0xB0);					//-
		break;
		
		case 4:								//FSK
		lcd_adress(0x85);
		lcd_data(0x46);
		
		lcd_adress(0x86);
		lcd_data(0x53);
		
		lcd_adress(0x87);
		lcd_data(0x4B);
		
		lcd_adress(0x89);
		lcd_data(0x54);					//T
		lcd_adress(0x8A);
		lcd_data(0x3A);					//:
		
		lcd_adress(0X8D);
		lcd_data(0xB0);					//-
		lcd_adress(0X8C);
		lcd_data(0xB0);					//-
		lcd_adress(0X8B);
		lcd_data(0xB0);					//-
		
		lcd_adress(0x8E);
		lcd_data(0x62);					//b
		lcd_adress(0x8F);
		lcd_data(0x73);					//s
		
		lcd_adress(0XC5);
		lcd_data(0xB0);					//-
		lcd_adress(0XC6);
		lcd_data(0xB0);					//-
		lcd_adress(0XC7);
		lcd_data(0x2E);					//.
		lcd_adress(0XC8);
		lcd_data(0xB0);					//-
		lcd_adress(0XC9);
		lcd_data(0xB0);					//-
		lcd_adress(0XCA);
		lcd_data(0x2E);					//.
		lcd_adress(0XCB);
		lcd_data(0xB0);					//-
		lcd_adress(0XCC);
		lcd_data(0xB0);					//-
		lcd_adress(0XCD);
		lcd_data(0x2E);					//.
		lcd_adress(0XCE);
		lcd_data(0xB0);					//-
		lcd_adress(0XCF);
		lcd_data(0xB0);					//-
		break;
	}
}

void lcd_port(uint16_t n){
	separate_digit(n);
	
	lcd_adress(0x89);
	lcd_data(0x50);					//P
	lcd_adress(0x8A);
	lcd_data(0x3A);					//:
	lcd_adress(0x8E);
	lcd_data(0x48);					//H
	lcd_adress(0x8F);
	lcd_data(0x7A);					//z
	lcd_adress(0x8B);
	lcd_number(ce);
	lcd_adress(0x8C);
	lcd_number(de);
	lcd_adress(0x8D);
	lcd_number(un);
}

void lcd_number(int n){
	switch(n){
		case 0:
		lcd_data(0x30);
		break;
		case 1:
		lcd_data(0x31);
		break;
		case 2:
		lcd_data(0x32);
		break;
		case 3:
		lcd_data(0x33);
		break;
		case 4:
		lcd_data(0x34);
		break;
		case 5:
		lcd_data(0x35);
		break;
		case 6:
		lcd_data(0x36);
		break;
		case 7:
		lcd_data(0x37);
		break;
		case 8:
		lcd_data(0x38);
		break;
		case 9:
		lcd_data(0x39);
		break;
	}
}

void lcd_calc(){
	lcd_adress(0XC5);
	lcd_data(0x43);					//C
	lcd_adress(0XC6);
	lcd_data(0x41);					//A
	lcd_adress(0XC7);
	lcd_data(0x4C);					//L
	lcd_adress(0XC8);
	lcd_data(0x43);					//C
	lcd_adress(0XC9);
	lcd_data(0x55);					//U
	lcd_adress(0XCA);
	lcd_data(0x4C);					//L
	lcd_adress(0XCB);
	lcd_data(0x41);					//A
	lcd_adress(0XCC);
	lcd_data(0x4E);					//N
	lcd_adress(0XCD);
	lcd_data(0x44);					//D
	lcd_adress(0XCE);
	lcd_data(0x4F);					//O
	lcd_adress(0XCF);
	lcd_data(0x01);					//null
}

void lcd_R_analog(int fin){
	
	separate_digit(fin);
	
	lcd_adress(0xC5);
	lcd_number(ce);
	
	lcd_adress(0xC6);
	lcd_number(de);
	
	lcd_adress(0xC7);
	lcd_number(un);
	
	lcd_adress(0XC8);
	lcd_data(0x01);					//null
	
	lcd_adress(0XC9);
	lcd_data(0x01);					//null
	
	lcd_adress(0XCA);
	lcd_data(0x01);					//null
	
	lcd_adress(0XCB);
	lcd_data(0x01);					//null
	
	lcd_adress(0XCC);
	lcd_data(0x01);					//null
	
	lcd_adress(0XCD);
	lcd_data(0x01);					//null
	
	lcd_adress(0XCE);
	lcd_data(0x01);					//null
	
	lcd_adress(0XCF);
	lcd_data(0x01);					//null
	
	lcd_adress(0x89);
	lcd_data('F');					//F
	lcd_adress(0x8A);
	lcd_data(0x3A);					//:
	
	lcd_adress(0x8B);
	lcd_number(ce);
	
	lcd_adress(0x8C);
	lcd_number(de);
	
	lcd_adress(0x8D);
	lcd_number(un);
}

void lcd_R_digit(char n[8], int fin){
	separate_digit(fin);
	lcd_adress(0XC5);
	lcd_data(n[0]);					//-
	
	lcd_adress(0XC6);
	lcd_data(n[1]);					//-
	
	lcd_adress(0XC7);
	lcd_data(0x2E);					//.
	
	lcd_adress(0XC8);
	lcd_data(n[2]);					//-
	
	lcd_adress(0XC9);
	lcd_data(n[3]);					//-
	
	lcd_adress(0XCA);
	lcd_data(0x2E);					//.
	
	lcd_adress(0XCB);
	lcd_data(n[4]);					//-
	
	lcd_adress(0XCC);
	lcd_data(n[5]);					//-
	
	lcd_adress(0XCD);
	lcd_data(0x2E);					//.
	
	lcd_adress(0XCE);
	lcd_data(n[6]);					//-
	
	lcd_adress(0XCF);
	lcd_data(n[7]);					//-
	
	lcd_adress(0x89);
	lcd_data(0x54);					//T
	lcd_adress(0x8A);
	lcd_data(0x3A);					//:
	
	lcd_adress(0x8B);
	lcd_number(ce);
	
	lcd_adress(0x8C);
	lcd_number(de);
	
	lcd_adress(0x8D);
	lcd_number(un);
	
	
	
}

void separate_digit(uint16_t n){
	un = 0;
	de = 0;
	ce = 0;
	int number = n;

	while (1){
		if (number<100){
			break;
		}
		number = number-100;
		ce++;
	}
	
	while (1){
		if (number<10){
			break;
		}
		number = number-10;
		de++;
	}
	un = number;
}