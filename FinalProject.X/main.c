#include "xc.h"
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

volatile int button_pressed = 0;
volatile int state = 0;

//Interrupt service Routine associated to INT1 flag
void __attribute__((__interrupt__,__auto_psv__)) _INT1Interrupt(){  
    IFS1bits.INT1IF = 0;          // Reset interrupt flag
    tmr_setup_period(TIMER2, 10);   
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
    
    int surge = 0; 
    int yaw_rate = 0;
    float ADCValue, V, distance;
    char buffer[16];
    
    // setting proportional parameter
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
    
    // Remapping INT1 to RE8 (RPI88)
    RPINR0bits.INT1R = 0x58;  // RPI88 -> 0x58 in hex
    INTCON2bits.GIE = 1;      // Set global interrupt enable
    IFS1bits.INT1IF = 0;      // Clear INT1 interrupt flag
    IEC1bits.INT1IE = 1;      // Enable INT1 interrupt
    IEC0bits.T2IE = 1;        // Enable T2 Interrupt (?)
    
    U2TXREG = 'c'; // vedi quando la scheda è online
    
    while(1) {
        do{
            if (AD1CON1bits.SAMP == 0)
                AD1CON1bits.SAMP = 1;      // Start sampling
        }while(!AD1CON1bits.DONE);         // Wait for sampling completion
        ADCValue = (float)ADC1BUF0;        // Get ADC value (ADC1BUF0 is the ADC buffer in which conversions are stored))
        V = ADCValue * 3.3/(float)1024.0;  // 3.3V mapped in 10bit
        distance = 100*(2.34 - 4.74 * V + 4.06 * powf(V,2) - 1.60 * powf(V,3) + 0.24 * powf(V,4));

        if (state == WaitForStart) {
   
            // Set PWM DC of all the motors to 0
            PWMstop();
                
            // LED A0 starts blinking at 1Hz frequency
            tmr_wait_ms(TIMER1, 500);         
            LATAbits.LATA0 = !LATAbits.LATA0;  
            tmr_wait_ms(TIMER1, 500);          
            LATAbits.LATA0 = !LATAbits.LATA0;  
                
            // Whenever the button RE8 is pressed change state
            if (button_pressed) {
                state = Moving;
                button_pressed = 0;
            }
        }

        else if (state == Moving) {
            
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
                
            // LED A0 starts blinking at 1Hz frequency
            tmr_wait_ms(TIMER1, 500);          // 500ms delay
            LATAbits.LATA0 = !LATAbits.LATA0;  // Toggle Led1
            LATBbits.LATB8 = !LATBbits.LATB8;  // Toggle Left Side Light
            LATFbits.LATF1 = !LATFbits.LATF1;  // Toggle Right Side Light
            tmr_wait_ms(TIMER1, 500);          
            LATAbits.LATA0 = !LATAbits.LATA0;  
            LATBbits.LATB8 = !LATBbits.LATB8;  
            LATFbits.LATF1 = !LATFbits.LATF1;  
                
            // Whenever the button RE8 is pressed change state
            if (button_pressed) {
                state = WaitForStart;
                button_pressed = 0;
            }
        }
    }
    return 0;
}