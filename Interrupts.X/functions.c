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
