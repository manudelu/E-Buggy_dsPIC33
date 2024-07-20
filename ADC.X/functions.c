/*
 * File:   functions.c
 * Author: manu
 */


#include "xc.h"
#include "header.h"

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
    
    // Non ho capito 0x03 e 0x4B
    RPOR0bits.RP64R = 0x03;   //Map UART1 TX to pin RD0 which is RP64
    RPINR19bits.U2RXR = 0x4B; //Map UART1 RX to pin RD11 which is RPI75(0x4B = 75)
    
    U2BRG = FCY / (16 * 9600) - 1;
    U2MODEbits.UARTEN = 1;    // Enable UART 
    U2STAbits.UTXEN = 1;      // Enable Transmission (must be after UARTEN)
}