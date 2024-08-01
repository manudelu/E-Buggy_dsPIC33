#include "xc.h"
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef enum {
    WaitForStart,
    Moving
} State;

volatile State state = WaitForStart;

// Interrupt service routine for INT1 
void __attribute__((__interrupt__,__auto_psv__)) _INT1Interrupt(){  
    IFS1bits.INT1IF = 0;          // Reset interrupt flag
    tmr_setup_period(TIMER2, 10);   
}

// Interrupt service routine for Timer2
void __attribute__((__interrupt__,__auto_psv__)) _T2Interrupt(){
    IFS0bits.T2IF = 0;  // Reset Interrupt Flag 
    T2CONbits.TON = 0;  // Stop timer 2
    
    if(PORTEbits.RE8 == 1){
        state = (state == WaitForStart) ? Moving : WaitForStart;
    }
}

int main(void) {
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    
    int surge = 0; 
    int yaw_rate = 0;
    float ADCValue, V, distance;
    char buffer[16];
    int blink_timer_buggy = 0;  // Timer for buggy's LED blinking
    int blink_timer_board = 0;  // Timer for board's LED blinking
    
    int choice = 0;
    
    // Setting proportional parameter
    int s = 2 ;
    int y = 500;
    
    int flag = 0;
    int turn_count = 0;
    
    // Control Loop Frequency = 1kHz  -> T = 1/f = 1ms
    tmr_setup_period(TIMER1, 1); 
    
    // Set up the PWM to work at 10kHz
    PWMsetup(10000);  
     
    LigthsSetup();
    ADCsetup();
    UARTsetup();
    
    // Remapping INT1 to RE8 (RPI88)
    RPINR0bits.INT1R = 0x58;  // RPI88 -> 0x58 in hex
    INTCON2bits.GIE = 1;      // Set global interrupt enable
    IFS1bits.INT1IF = 0;      // Clear INT1 interrupt flag
    IEC1bits.INT1IE = 1;      // Enable INT1 interrupt
    IEC0bits.T2IE = 1;        // Enable T2 Interrupt (?)
    
    while(1) {       
        // ADC sampling
        do{
            if (AD1CON1bits.SAMP == 0)
                AD1CON1bits.SAMP = 1;      // Start sampling
        }while(!AD1CON1bits.DONE);         // Wait for sampling completion
        
        choice = 0;  // placeholder per distinguire tra batteria (1) e ir (0)
        if (choice == 0){
            ADCValue = (float)ADC1BUF1;        // Get ADC value 
            V = ADCValue * 3.3/1024.0;         // Convert to voltage
            distance = 100*(2.34 - 4.74 * V + 4.06 * powf(V,2) - 1.60 * powf(V,3) + 0.24 * powf(V,4));
            sprintf(buffer, "d:%.2f ", distance);
            for (int i=0; i < strlen(buffer); i++) {
                while (U2STAbits.UTXBF == 1);  // Wait until the Transmit Buffer is not full 
                U2TXREG = buffer[i];   
            }
        }
        else if (choice == 1){
            const float R49 = 100.0, R51 = 100.0, R54 = 100.0;
            ADCValue = (float)ADC1BUF0;
            V = ADCValue * 3.3/1024.0;
            float Rs = R49 + R51;
            float battery = V * (Rs + R54) / R54;
            sprintf(buffer, "$MBATT,%.2f ", battery);
            for (int i = 0; i < strlen(buffer); i++){
                while (U2STAbits.UTXBF == 1); // Wait until the Transmit Buffer is not full
                U2TXREG = buffer[i];
            }
        }
        
        // Board LED blinking
        if (blink_timer_board == 1000){
            LATAbits.LATA0 = !LATAbits.LATA0;  // Toggle Led1
            blink_timer_board = 0;
        }
        
        // State handling
        if (state == WaitForStart) {
            // Set PWM DC of all the motors to 0
            PWMstop();
            
            // Turn off lights
            LATAbits.LATA7 = 0; // Beam Headlights 
            LATFbits.LATF0 = 0; // Brakes 
            LATGbits.LATG1 = 0; // Low Intensity Lights 
            
            // Buggy LED blinking
            if (blink_timer_buggy == 1000) {
                LATBbits.LATB8 = !LATBbits.LATB8;  // Toggle Left Side Light
                LATFbits.LATF1 = !LATFbits.LATF1;  // Toggle Right Side Light
                blink_timer_buggy = 0;
            } 
        }

        else if (state == Moving) {
            // Handle turning
            if (flag && distance > MAXTH && turn_count < 500){
                turn_count++;
            }
            else{
                turn_count = 0;
                // Calculate surge and yaw_rate based on distance
                if (distance < MINTH) {
                    surge = 0;
                    yaw_rate = 100;
                    flag = 1;
                } else if (distance > MAXTH) {
                    surge = 100;
                    yaw_rate = 0;
                    flag = 0;
                } else {
                    surge = distance * s; 
                    yaw_rate = y / distance;
                }
            }
                
            // Set PWM DC of all the motors based on the surge and yaw rate
            PWMstart(surge, yaw_rate);
            
            // Control lights based on surge
            if (surge > 50){
                LATAbits.LATA7 = 1; // Beam Headlights 
                LATFbits.LATF0 = 0; // Brakes 
                LATGbits.LATG1 = 0; // Low Intensity Lights 
            }
            else{
                LATAbits.LATA7 = 0; // Beam Headlights 
                LATFbits.LATF0 = 1; // Brakes 
                LATGbits.LATG1 = 1; // Low Intensity Lights 
            }
            
            // Control side lights based on yaw_rate
            if (yaw_rate > 15){
                LATBbits.LATB8 = 0;
                if (blink_timer_buggy == 1000){
                    LATFbits.LATF1 = !LATFbits.LATF1;
                    blink_timer_buggy = 0;
                }
            }
            else{
                LATBbits.LATB8 = 0;
                LATFbits.LATF1 = 0;
            }
        }
        
        blink_timer_board++;
        blink_timer_buggy++;
        tmr_wait_period(TIMER1);
    }
    return 0;
}
