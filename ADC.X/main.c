/*
 * File:   main.c
 * Author: manu
 *
 * Created on 20 luglio 2024, 18.07
 */


#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TIMER1 1
#define TIMER2 2
#define FOSC 144000000  // Fosc = Fin*(M/(N1*N2)) = 8Mhz*(72/(2*2)) = 144MHz
#define FCY (FOSC / 2)  // Fcy = Fosc/2 = 72MHz
#define MaxInt 65535    // Max number that can be represented with 16bit register 
#define MaxTime 200 // tecnhically max amount possible = 233

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

// Function that setups the timer to count for the specified amount of ms
void tmr_setup_period(int timer, int ms){
    
    long steps = FCY * (ms / 1000.0); // Number of clock cycles (steps)
    int presc, t;                     
    
    // Find the smallest prescaler such that the number of required 
    // steps fits within the MaxInt value of the timer
    if (steps <= MaxInt) {           // Prescaler 1:1
        presc = 1;
        t = 0; // 00  
    }
    else if (steps/8 <= MaxInt) {    // Prescaler 1:8
        presc = 8;
        t = 1; // 01
    }
    else if (steps/64 <= MaxInt) {   // Prescaler 1:64
        presc = 64;
        t = 2; // 10
    }
    else if (steps/256 <= MaxInt) {  // Prescaler 1:256
        presc = 256;
        t = 3; // 11
    }
    else {
        // Error: the desired period is too long even with the largest prescaler
        return;
    }
    
    switch(timer) {
        case TIMER1: 
            TMR1 = 0;               // Reset timer counter
            T1CONbits.TON = 0;      // Stops the timer
            T1CONbits.TCKPS = t;    // Set the prescale value
            PR1 = steps/presc;      // Set the number of clock step of the counter
            T1CONbits.TON = 1;      // Starts the timer
            break;
            
        case TIMER2: 
            TMR2 = 0;              
            T2CONbits.TON = 0;      
            T2CONbits.TCKPS = t;   
            PR2 = steps/presc;      
            T2CONbits.TON = 1;      
            break;
    }
}

// Function to wait for the completion of a timer period
void tmr_wait_period(int timer){
    switch(timer){
        case TIMER1:
            while(IFS0bits.T1IF == 0){};
            IFS0bits.T1IF = 0; // Reset timer1 interrupt flag
            break;
            
        case TIMER2:
            while(IFS0bits.T2IF == 0){};
            IFS0bits.T2IF = 0;
            break;
    }
}

// Function to wait for a specified number of milliseconds using a timer
void tmr_wait_ms(int timer, int ms){
    while (ms > MaxTime) {
        tmr_setup_period(timer, MaxTime); 
        tmr_wait_period(timer);
        ms -= MaxTime;
    }
    tmr_setup_period(timer, ms); 
    tmr_wait_period(timer); 
}

// Function to setup the ADC
void ADCsetup(void) {
    AD1CON3bits.ADCS = 8;   // Tad = 8*Tcy = 8/72MHz = 111.11ns
    AD1CON1bits.ASAM = 0;   // Manual sampling
    AD1CON1bits.SSRC = 7;   // Automatic conversion
    AD1CON3bits.SAMC = 16;  // Sampling lasts 16 Tad 
    AD1CON2bits.CHPS = 0;   // use 1-channel (CH0) mode
    AD1CHS0bits.CH0SA = 14; // Remapping: IR senson is on AN14 (vedi TRISB14)  
    AD1CON1bits.ADON = 1;   // Turn ADC on
}

// Function to setup the UART (mounted on mikroBUS2, 9600 bps)
void UARTsetup(void){
    // From documentation (pag.9 datasheet):
    // RD0(RP64) -> TX
    // RD11(RPI75) -> RX
    // For Transitter: Find 'OUTPUT SELECTION FOR REMAPPABLE PINS' in datasheet
    // Under U2TX -> 000011 -> 0x03 in hex
    // For Receiver: Find 'INPUT PIN SELECTION FOR SELECTABLE INPUT SOURCES' in datasheet
    // Under RPI75 -> 1001011 -> 0x4B
    
    RPOR0bits.RP64R = 0x03;   //Map UART2 TX to pin RD0 which is RP64
    RPINR19bits.U2RXR = 0x4B; //Map UART2 RX to pin RD11 which is RPI75
    
    U2BRG = FCY / (16 * 9600) - 1;
    U2MODEbits.UARTEN = 1;    // Enable UART 
    U2STAbits.UTXEN = 1;      // Enable Transmission (must be after UARTEN)
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