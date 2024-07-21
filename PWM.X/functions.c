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

// Function to set the PWM with Output Compare module
void PWMsetup(void){
    
    //OC1 010000 RPn tied to Output Compare 1 Output
    //OC2 010001 RPn tied to Output Compare 2 Output
    //OC3 010010 RPn tied to Output Compare 3 Output
    //OC4 010011 RPn tied to Output Compare 4 Output
    
    RPOR0bits.RP65R = 0x10; // RD1 is RP65 (OC1: 010000 -> 0x10 in hex)
    RPOR1bits.RP66R = 0x11; // RD2 is RP66 (OC2: 010001 -> 0x11 in hex)
    RPOR1bits.RP67R = 0x12; // RD3 is RP67 (OC3: 010010 -> 0x12 in hex)
    RPOR2bits.RP68R = 0x13; // RD4 is RP68 (OC4: 010011 -> 0x13 in hex)
    
    // OC1: left wheels anticlockwise
    OC1CON1bits.OCTSEL = 7;     // Selects the peripheral clock (111 = 7) as the clock source to the OC module
    OC1CON1bits.OCM = 6;        // Configures the OC module in Edge-Aligned PWM mode (110 = 6)
    OC1CON2bits.SYNCSEL = 0x1F; // Specifies that the OC module is synchronized to itself, meaning it operates independently without external synchronization: Tpwm = Tcy (11111 = 1f)
    
    // OC2: left wheels clockwise
    OC2CON1bits.OCTSEL = 7;
    OC2CON1bits.OCM = 6; 
    OC2CON2bits.SYNCSEL = 0x1F;

    // OC3: right wheels anticlockwise
    OC3CON1bits.OCTSEL = 7;
    OC3CON1bits.OCM = 6;
    OC3CON2bits.SYNCSEL = 0x1F;

    // OC4: right wheels clockwise
    OC4CON1bits.OCTSEL = 7;
    OC4CON1bits.OCM = 6;
    OC4CON2bits.SYNCSEL = 0x1F;
    
    // OCxRS define the max period
    OC1RS = OC2RS = OC3RS = OC4RS = DutyCycleMax;
    
    // Setup OCxR to 0 at start
    OC1R = OC2R = OC3R = OC4R = 0;
}

// Function to stop PWM
void PWMstop(void){
    OC1R = OC2R = OC3R = OC4R = 0;
}

// Function to move forward
void PWMstart(void){
    OC1R = 0;
    OC2R = DutyCycleMax;
    OC3R = 0;
    OC4R = DutyCycleMax; // Multiply by 0.75 to make the buggy with right while going forward
}