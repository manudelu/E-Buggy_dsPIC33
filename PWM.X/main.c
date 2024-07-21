/*
 * File:   main.c
 * Author: manu
 *
 * Created on 16 luglio 2024, 17.55
 */

//When button E8 is pressed, start generating a PWM signal with
//the Output Compare peripheral at a 10 kHz frequency to make
//the buggy move forward. When the same button E8 is pressed
//again, stop generating the PWM signal.
//Hint : the duty cycle range to enable motion is between 40% and 100%

#include "xc.h"
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

volatile int button_pressed = 0;

//Interrupt service Routine associated to INT2 flag
void __attribute__ ((__interrupt__ , __auto_psv__)) _INT2Interrupt() {
    IFS1bits.INT2IF = 0; // reset interrupt flag
    IEC1bits.INT2IE = 0; // disable interrupt for INT2
    T2CONbits.TON = 1; // Starts the timer
    IEC0bits.T2IE = 1; // enable T2 interrupt
}

void __attribute__((__interrupt__,__auto_psv__)) _T2Interrupt(){
    IFS0bits.T2IF = 0;  // Reset External Interrupt 2 Flag Status bit
    
    if(PORTEbits.RE8 == 1){
        if(button_pressed == 0)
            PWMstart();
        else
            PWMstop();
        button_pressed = !button_pressed;
    }
    
    T2CONbits.TON = 0; // Stops the timer
    TMR2 = 0; // Reset timer counter
    IFS1bits.INT2IF = 0; // reset INT2 interrupt flag
    IEC1bits.INT2IE = 1;  // enable interrupt for INT2
}

int main(void) {
    
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    
    PWMsetup();
    
    TRISAbits.TRISA0 = 0;  // Set A0 pin (Led1) as output
    LATAbits.LATA0 = 0;    // Ensure LED A0 is initially off
    TRISGbits.TRISG9 = 0; // set led2 as output  
    LATGbits.LATG9 = 0;    // Ensure LED A0 is initially off
    
    TRISEbits.TRISE8 = 1;  // Set RE8 Button as input
    
    // Remapping INT1 to RE8
    // Find RE8 in datasheet -> it is tied to RPI88 -> 
    // Find RPI88 -> in 'INPUT PIN SELECTION FOR SELECTABLE INPUT SOURCES' -> 1011000 -> 0x58 in hex
    RPINR1bits.INT2R = 0x58;  // 0x58 is 88 in hex
    INTCON2bits.GIE = 1;      // Set global interrupt enable
    IFS1bits.INT2IF = 0;      // Clear interrupt flag
    IEC1bits.INT2IE = 1;      // Eneble Interrupt
    
    while(1) {
        LATGbits.LATG9 = 1;
        tmr_wait_period(TIMER2);
    }
    return 0;
}