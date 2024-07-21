/*
 * File:   main2.c
 * Author: manu
 *
 * Created on 20 luglio 2024, 16.56
 */

//Plug the IR Distance Sensor and the RS232 UART shield in the
//MIKRObus sockets of the Clicker board. Use the interrupt of
//button E8 to trigger an ISR that performs manual sampling and
//conversion, then send the result on the UART. Hint: check the
//Interrupt and UART tutorials for the Clicker 2 board.

#include "xc.h"
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

volatile int button_pressed = 0;

//Interrupt service Routine associated to INT1 flag
void __attribute__((__interrupt__,__auto_psv__)) _INT1Interrupt(){  
    IFS1bits.INT1IF = 0;          // Reset interrupt flag
    tmr_setup_period(TIMER2, 20);   
}

void __attribute__((__interrupt__,__auto_psv__)) _T2Interrupt(){
    IFS0bits.T2IF = 0;  // Reset External Interrupt 2 Flag Status bit
    T2CONbits.TON = 0;  // Stop timer 2
    
    if(PORTEbits.RE8 == 1){
      button_pressed = 1;
    }
}

int main(void) {
    
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    
    float ADCValue, V, distance;
    char buffer[16];
    
    TRISAbits.TRISA0 = 0;  // Set A0 pin (Led1) as output
    LATAbits.LATA0 = 0;    // Ensure LED A0 is initially off
    
    TRISEbits.TRISE8 = 1;  // Set RE8 Button as input
    
    // Remapping INT1 to RE8
    // Find RE8 in datasheet -> it is tied to RPI88 -> 
    // Find RPI88 -> in 'INPUT PIN SELECTION FOR SELECTABLE INPUT SOURCES' -> 1011000 -> 0x58 in hex
    RPINR0bits.INT1R = 0x58;  // 0x58 is 88 in hex
    INTCON2bits.GIE = 1;      // Set global interrupt enable
    INTCON2bits.INT1EP = 0;
    IFS1bits.INT1IF = 0;      // Clear interrupt flag
    IEC1bits.INT1IE = 1;      // Eneble Interrupt
    IEC0bits.T2IE = 1;
    
    UARTsetup();
    
    // IR Sensor setup (mounted on front right mikroBUS)
    TRISBbits.TRISB9 = 0;
    LATBbits.LATB9 = 1;     // Enable EN pin 
    TRISBbits.TRISB14 = 1;
    ANSELBbits.ANSB14 = 1;
    
    ADCsetup();
    
    U2TXREG = 'c'; // vedi quando la scheda è online
    
    while(1) {
        if (button_pressed == 1){
            AD1CON1bits.SAMP = 1;         // Start sampling
            while(!AD1CON1bits.DONE);     // Wait for sampling completion
            LATAbits.LATA0 = 1;
            ADCValue = ADC1BUF0;          // Get ADC value (ADC1BUF0 is the ADC buffer in which conversions are stored))
            V = ADCValue * 3.3/1024.0;
            distance = 2.34 - 4.74 * V + 4.06 * powf(V,2) - 1.60 * powf(V,3) + 0.24 * powf(V,4); // distance in m
            sprintf(buffer, "%.2f", distance);
            U2TXREG = 'd';
            U2TXREG = ':';
            for (int i=0; i < strlen(buffer); i++) {
            while (U2STAbits.UTXBF == 1);  // Wait until the Transmit Buffer is not full 
                U2TXREG = buffer[i];   
            }
            U2TXREG = ' ';
            LATAbits.LATA0 = 0; 
            button_pressed = 0;
        }
    }
    return 0;
}