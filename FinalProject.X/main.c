#include "xc.h"
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Define the state machine states
typedef enum {
    WaitForStart,
    Moving
} State;

// Initialize state variable
volatile State state = WaitForStart;

// Create the CircularBuffer object
volatile CircularBuffer cb;

// Interrupt Service Routine for INT1 
void __attribute__((__interrupt__,__auto_psv__)) _INT1Interrupt(){  
    IFS1bits.INT1IF = 0;            // Clear interrupt flag
    tmr_setup_period(TIMER2, 10);   
}

// Interrupt Service Routine for Timer2
void __attribute__((__interrupt__,__auto_psv__)) _T2Interrupt(){
    IFS0bits.T2IF = 0;   // Clear Timer2 interrupt flag
    T2CONbits.TON = 0;   // Stop timer 2
    
    // Toggle the state based on RE8 button
    if(PORTEbits.RE8 == 1){
        state = (state == WaitForStart) ? Moving : WaitForStart;
    }
}

// Interrupt handler for the char recevied on UART2
void __attribute__((__interrupt__, __auto_psv__)) _U2RXInterrupt() {
    IFS1bits.U2RXIF = 0;         // Reset UART2 Receiver Interrupt Flag Status bit
    char receivedChar = U2RXREG; // Get char from UART2 received REG
    cb_push(&cb, receivedChar);  // When a new char is received, push it to the circular buffer   
}

int main(void) {
    // Disable analog inputs
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    
    // Initialize Circular Buffer Variables
    cb.head = 0;
    cb.tail = 0;    
    cb.to_read = 0;
    
    char byte;     // Keep track of the received characters
    int payload;
    
    parser_state pstate;
	pstate.state = STATE_DOLLAR;
	pstate.index_type = 0; 
	pstate.index_payload = 0;
    
    int surge = 0, yaw_rate = 0;
    float distance;
    
    int blink_timer_buggy = 0;  // Timer for buggy's LED blinking
    int blink_timer_board = 0;  // Timer for board's LED blinking
    int uart_timer = 0;
    
    int choice = 0;
    
    // Set up peripherals
    PWMsetup(10000);      // Set up the PWM to work at 10kHz
    LigthsSetup();
    ADCsetup();
    UARTsetup();
    
    // Configure INT1 (mapped to RE8)
    TRISEbits.TRISE8 = 1;     // Set RE8 as input
    RPINR0bits.INT1R = 0x58;  // Remap RE8 to INT1 (RPI88 -> 0x58)
    INTCON2bits.GIE = 1;      // Enable global interrupts
    IFS1bits.INT1IF = 0;      // Clear INT1 interrupt flag
    IEC1bits.INT1IE = 1;      // Enable INT1 interrupt
    IEC0bits.T2IE = 1;        // Enable Timer2 Interrupt (?)
    
    // Control loop frequency = 1 kHz (1 ms period)
    tmr_setup_period(TIMER1, 1); 
    
    while(1) {       
        distance = getDistance();
        
        // ADC sampling
        do{
            if (AD1CON1bits.SAMP == 0)
                AD1CON1bits.SAMP = 1;      // Start sampling
        }while(!AD1CON1bits.DONE);         // Wait for sampling completion
         
        // State machine handling
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
            blink_timer_buggy++;
        }

        else if (state == Moving) {
            // Calculate surge and yaw_rate based on distance
            if (distance < MINTH) {
                surge = 0;
                yaw_rate = 100;
            } else if (distance > MAXTH) {
                surge = 100;
                yaw_rate = 0;
            } else {
                // Set proportional control parameters
                const int s = 2, y = 1000;
                surge = distance * s; 
                yaw_rate = y / distance;
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
                blink_timer_buggy++;
            }
            else{
                LATBbits.LATB8 = 0;
                LATFbits.LATF1 = 0;
                blink_timer_buggy = 0;
            }
        }
        
        // Board LED A0 1Hz blinking
        if (blink_timer_board == 1000){
            LATAbits.LATA0 = !LATAbits.LATA0;  // Toggle Led1
            sendBattery();
            blink_timer_board = 0;
        }
        blink_timer_board++;
        
        if (uart_timer == 10000){
            sendDistance();
            sendDC();
            uart_timer = 0;
        }
        uart_timer++;
        
        tmr_wait_period(TIMER1);
    }
    
    return 0;
}