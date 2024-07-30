/*
 * File:   main.c
 * Author: manu
 *
 * Created on 16 luglio 2024, 17.55
 */

//Blink LD2 led at 2.5 Hz frequency (200ms time on, 200ms off)
//without using interrupts; every time the button T2 is pressed,
//toggle the led LD2 using interrupts

#include "xc.h"
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

volatile int ledState = 0;   // Variable to keep track of the LED state

//Interrupt service Routine associated to RE8 button
void __attribute__ ((__interrupt__ , __auto_psv__)) _INT1Interrupt() {
    IFS1bits.INT1IF = 0;          // Reset interrupt flag
    
    tmr_setup_period(TIMER2, 10); // Start TIMER2 to avoid bouncing
}

void __attribute__((__interrupt__,__auto_psv__)) _T2Interrupt(){
    IFS0bits.T2IF = 0;  // Clear TIMER2 Interrupt Flag
    T2CONbits.TON = 0;  // Stop the timer

    if(PORTEbits.RE8 == 1){        // Check if the button is still pressed
        ledState = !ledState;      // Toggle the LED state
        LATGbits.LATG9 = ledState; // Update the LED based on the state
    }        
}

int main(void) {
     // Disable all analog inputs
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    
    // Led1 setup
    TRISAbits.TRISA0 = 0;  // Set A0 pin (Led1) as output
    LATAbits.LATA0 = 0;    // Ensure LED A0 is initially off
    
    // Led2 setup
    TRISGbits.TRISG9 = 0;  // Set G9 pin (Led2) as output 
    LATGbits.LATG9 = 0;    // Ensure LED G9 is initially off
    
    // Button setup 
    TRISEbits.TRISE8 = 1;  // Set RE8 (button) as input

    // Remapping INT1 to RE8 (RPI88)
    RPINR0bits.INT1R = 0x58;  // RPI88 -> 0x58 in hex
    INTCON2bits.GIE = 1;      // Set global interrupt enable
    IFS1bits.INT1IF = 0;      // Clear INT1 interrupt flag
    IEC1bits.INT1IE = 1;      // Enable INT1 interrupt
    IEC0bits.T2IE = 1; 
    
    while(1) {
        // Blink LED1 at 2.5 Hz
        LATAbits.LATA0 = 1;
        tmr_wait_ms(TIMER1, 200);
        LATAbits.LATA0 = 0;
        tmr_wait_ms(TIMER1, 200);
    }
    return 0;
}