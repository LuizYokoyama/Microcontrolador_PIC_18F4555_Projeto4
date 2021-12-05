/* 
 * File:   mainE6.c
 * Author: Luiz Felix
 *
 * Created on 17 de Dezembro de 2020, 22:56
 */

#include <stdio.h>
#include <string.h> 
#include <stdlib.h>

#define _XTAL_FREQ 4000000

#pragma config FOSC = HS        // Oscilador externo
#pragma config WDT = OFF        // Watchdog Timer desligado
#pragma config MCLRE = OFF      // Master Clear desabilitado
#pragma config PBADEN = OFF     // Desabilita conversor AD da porta B

#include <xc.h>

long contagem = 0;  // Variável auxiliar
float valor;        // Guarda o valor em porcentagem do potenciômetro

void inicializa_Serial() {
    RCSTA = 0x90; // Habilita porta serial; recepção 8 bits(SPEN = 1; CREN = 1)
    TXSTA = 0x24; // assíncrono, 8 bits(TX9=0, TXEN=1), alta velocidade(BRGH=1)
    SPBRG = 25; // Baud rate de 9600 bps; modo de alta velocidade.
}

void setupADC(void) {
    TRISA = 0b00000010;         // Habilita pino AN1 entrada

    ADCON2bits.ADCS = 0b100;    // Clock do AD: Fosc/4
    ADCON2bits.ACQT = 0b010;    // Tempo de aquisição: 4 Tad
    ADCON2bits.ADFM = 0b1;      // Formato: à direita
    ADCON1bits.VCFG = 0b00;     // Tensões de referência: Vss e Vdd
    
    ADCON0bits.CHS = 0b0001;    // Seleciona o canal AN1   
    ADCON0bits.ADON = 1;        // Liga o AD
}

void setupInt(void) {
    GIE = 1;        // Habilita interrupção global
    PEIE = 1;       // Habilita interrupção de periféricos
    ADIE = 1;       // Habilita interrupção do ADC   
}

/* Configuração para enviar a mensagem via serial*/
void envia_serial (char valor) {
    TXIF = 0;           // Limpa flag que sinaliza envio completo.
    TXREG = valor;      // Envia caractere desejado à porta serial
    while(TXIF ==0);    // Espera caractere ser enviado
}

void envia_texto_serial (const char frase[]) {
    char indice = 0;              // índice da cadeia de caracteres
    char tamanho = strlen(frase); // tamanho total da cadeia a ser impressa
    while(indice < tamanho ) {    // verifica se todos foram impressos
        envia_serial (frase[indice]); // Chama rotina que escreve o caractere
        indice++; // incrementa índice
    }
}

/*                Configurações para ligar o motor
 *  A velocidade depende do tempo gasto em cada passo. Essa função recebe
 *  o numero de vezes que __delay_ms deve ser repetido, para aumentar ou
 *  ou diminuir a velocidade, de acordo com o valor no potenciômetro */ 
void ligaMotor (int n) {
    int cont, i = 0;          // Variáveis auxiliares para os loops   
    LATB = 0b10000000;        // Ativa o pino RB7
    while (i <= 72) {         // Motor rotaciona 1 volta completa, pois
                              // cada passo tem 5° ->  5*72 = 360°                      
        if (LATB == 0b00001000) {// Verifica se passou do pino RB4
            LATB = 0b10000000;   // Se sim volta para o pino RB7
        }
        for(cont = 0; cont < n; cont++){// Atraso de tempo para cada passo
            __delay_ms(5);              // Repete n vezes o tempo de 5 ms
        }
        LATB = LATB >> 1;     // Rotaciona o valor nos pinos do motor de passo
            i = i + 1;
    }
}

/* Acende os leds um a um conforme vai aumentando a velocidade do motor
 * Como tem 8 leds, o valor no potenciômetro foi dividido em 8 faixas
 * de 12.5% cada uma */

void acendeLeds(float valor) {
       if(valor >=0 && valor <= 12.5) {// Se o valor no Potenciômetro estiver
           LATD = 0b00000001;          // entre 0 e 12.5%, acende o LED0
           envia_texto_serial("Velocidade Minima \r");
           ligaMotor(30);              // e aciona o motor na velocidade mínima
       } else if(valor > 12.5 && valor <= 25) {
           LATD = 0b00000011; 
           ligaMotor(26);              
       } else if(valor > 25 && valor <= 37.5) {
           LATD = 0b00000111; 
           ligaMotor(23);
       } else if(valor > 37.5 && valor <= 50) {
           LATD = 0b00001111; 
           ligaMotor(20);
       } else if(valor > 50 && valor <= 62.5) {
           LATD = 0b00011111; 
           ligaMotor(18);
       } else if(valor > 62.5 && valor <= 75) {
           LATD = 0b00111111;
           ligaMotor(15);
       } else if(valor > 75 && valor <= 87.5) {
           LATD = 0b01111111; 
           ligaMotor(13);
       } else if(valor > 87.5) {
           LATD = 0b11111111;
           envia_texto_serial("Velocidade Maxima \r");
           ligaMotor(10);       // Gera um atraso de tempo de 50 ms
       }
}

void interrupt interrupcao() {
    if (ADIF) {
       contagem = (ADRESH * 0x100) + ADRESL;   // Transfere a leitura do AD 
       contagem = contagem >> 2;               // para a variavel 'contagem'
       
       valor = (contagem*100.0)/255.0;        // Calcula o valor em porcentagem
                                              // que está no potenciômetro      
       acendeLeds(valor);              // Chama função que acende os leds
       
       ADIF = 0;                       // Desmarca flag da interrupção ADC               
       ADCON0bits.GO = 1;              // Dispara ADC     
    }
}


void main(void) {
    TRISD = 0;				// Define porta D inteira como saída
    TRISB = 0b00000010;     // Define porta B como saída, menos RB1
    
    TRISCbits.TRISC6 = 1;   // Pino RC6 definido como entrada
    TRISCbits.TRISC7 = 1;   // Pino RC7 definido como entrada
    inicializa_Serial();    // modo de alta velocidade

    setupADC();             // Configuração do ADC
    setupInt();             // Configuração da Interrupção
  
    ADCON0bits.GO = 1;      // Dispara ADC
    
    while(1) {              // Loop infinito esperando a interrupção
    }
    return;
} 



