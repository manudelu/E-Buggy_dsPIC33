#include "xc.h"
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_TASKS 4

// Define the state machine states
typedef enum {
    WaitForStart,
    Moving
} State;

// Initialize state variable
volatile State state = WaitForStart;

volatile int yaw_rate = 0;
volatile int surge = 0;

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

// Task function for controlling lights and indicators
void task_control_lights(void *params) {
    static int blink_state = 0;
    LATAbits.LATA0 = blink_state;
    
    if (state == WaitForStart) {
        LATBbits.LATB8 = blink_state; // Left indicator
        LATFbits.LATF1 = blink_state; // Right indicator
        
        // Turn off other lights
        LATAbits.LATA7 = 0;  // Beam lights off
        LATFbits.LATF0 = 0;  // Brakes off
        LATGbits.LATG1 = 0;  // Low intensity lights off
    } else if (state == Moving) {        
        // Indicator control
        if (yaw_rate > 15) {
            LATBbits.LATB8 = 0;           // Left indicator off
            LATFbits.LATF1 = blink_state; // Right indicator blinking
        } else {
            LATBbits.LATB8 = 0;           // Left indicator off
            LATFbits.LATF1 = 0;           // Right indicator off
        }

        // Lights control
        if (surge > 50) {
            LATAbits.LATA7 = 1;  // Beam lights on
            LATFbits.LATF0 = 0;  // Brakes off
            LATGbits.LATG1 = 0;  // Low intensity lights off
        } else {
            LATAbits.LATA7 = 0;  // Beam lights off
            LATFbits.LATF0 = 1;  // Brakes on
            LATGbits.LATG1 = 1;  // Low intensity lights on
        }
    }

    // Toggle the blink state for the next cycle
    blink_state = !blink_state;
}

int main(void) {
    // Disable analog inputs
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    
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
    
    
    // scheduler configuration
    heartbeat schedInfo[MAX_TASKS];
    
    // Lights Control Task
    schedInfo[0].n = 0; 
    schedInfo[0].N = 1000; // 1 Hz
    schedInfo[0].f = task_control_lights;
    schedInfo[0].params = NULL;
    schedInfo[0].enable = 1;
    // Send Battery Task
    schedInfo[1].n = 0;
    schedInfo[1].N = 1000; // 1 Hz
    schedInfo[1].f = task_send_battery;
    schedInfo[1].params = NULL;
    schedInfo[1].enable = 1; 
    // Send Distance Task
    schedInfo[2].n = 0;
    schedInfo[2].N = 10000; // 10 Hz
    schedInfo[2].f = task_send_distance;
    schedInfo[2].params = NULL;
    schedInfo[2].enable = 1;   
    // Send Duty Cycle Task
    schedInfo[3].n = 0;
    schedInfo[3].N = 10000; // 10 Hz
    schedInfo[3].f = task_send_dutycycle;
    schedInfo[3].params = NULL;
    schedInfo[3].enable = 1; 
    
    // Control loop frequency = 1 kHz (1 ms period)
    tmr_setup_period(TIMER1, 1); 
    
    while(1) {       
        float distance = getDistance();
        
        // ADC sampling
        do{
            if (AD1CON1bits.SAMP == 0)
                AD1CON1bits.SAMP = 1;      // Start sampling
        }while(!AD1CON1bits.DONE);         // Wait for sampling completion
         
        // State machine handling
        if (state == WaitForStart) {
            // Set PWM DC of all the motors to 0
            PWMstop();
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
        }
        
        scheduler(schedInfo, MAX_TASKS);       
        tmr_wait_period(TIMER1);
    }
    
    return 0;
}