/*
 * File:   main.c
 * Author: manu
 *
 * Created on 16 luglio 2024, 17.55
 */


#include "xc.h"
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(void) {
    
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    
    float ADCValue, V, distance;
    char buffer[16];
    
    UARTsetup();
    
    TRISAbits.TRISA0 = 0;  // Set A0 pin (Led1) as output
    LATAbits.LATA0 = 0;    // Ensure LED A0 is initially off
    
    // IR Sensor setup (mounted on front right mikroBUS))
    TRISBbits.TRISB9 = 0;
    LATBbits.LATB9 = 1;     // Enable EN pin 
    TRISBbits.TRISB14 = 1;
    ANSELBbits.ANSB14 = 1;
    
    ADCsetup();
    
    U2TXREG = 'c'; // vedi quando la scheda è online
    
    while(1) {
        AD1CON1bits.SAMP = 1;       // Start sampling
        while(!AD1CON1bits.DONE);    // Wait for sampling completion
        LATAbits.LATA0 = 1;
        ADCValue = ADC1BUF0;        // Get ADC value (ADC1BUF0 is the ADC buffer in which conversions are stored))
        V = ADCValue * 3.3/1024.0;
        distance = 2.34 - 4.74 * V + 4.06 * powf(V,2) - 1.60 * powf(V,3) + 0.24 * powf(V,4); // distance in m
        sprintf(buffer, "%.2f", distance);
        for (int i=0; i < strlen(buffer); i++) {
        while (U2STAbits.UTXBF == 1);    // Wait until the Transmit Buffer is not full 
            U2TXREG = buffer[i];   
        }
        LATAbits.LATA0 = 0;
        tmr_wait_ms(TIMER1, 1000);
    }
    return 0;
}
