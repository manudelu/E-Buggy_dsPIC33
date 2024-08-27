/*
 * File:   main.c
 * Authors: Delucchi Manuel S4803977, Matteo Cappellini S4822622
 */

#include "xc.h"
#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

volatile ControlData control_data = {25, 50, 0, 0, WaitForStart};
volatile CircularBuffer cb;

// Interrupt Service Routine for INT1 
void __attribute__((__interrupt__,__auto_psv__)) _INT1Interrupt(){  
    IFS1bits.INT1IF = 0;            
    tmr_setup_period(TIMER2, 10);   
}

// Interrupt Service Routine for Timer2
void __attribute__((__interrupt__,__auto_psv__)) _T2Interrupt(){
    IFS0bits.T2IF = 0;  
    T2CONbits.TON = 0;   
    
    // Toggle the state based on RE8 button
    if(PORTEbits.RE8 == 1){
        // If the current state is WaitForStart, set it to Moving; otherwise, set it back to WaitForStart
        control_data.state = (control_data.state == WaitForStart) ? Moving : WaitForStart;
    }
}

// Interrupt handler for the char recevied on UART2
void __attribute__((__interrupt__, __auto_psv__)) _U2RXInterrupt() {
    IFS1bits.U2RXIF = 0;         // Reset UART2 Receiver Interrupt Flag Status bit
    char receivedChar = U2RXREG; // Get char from UART2 received REG
    cb_push(&cb, receivedChar);  // Push it to the circular buffer  
}

int main(void) {
    // Disable analog inputs
    ANSELA = ANSELB = ANSELC = ANSELD = ANSELE = ANSELG = 0x0000;
    
    // Set up peripherals
    LigthsSetup();
    PWMsetup(10000);  // Set up PWM at 10kHz
    ADCsetup();
    UARTsetup();
    
    char readChar;     // Keep track of the received characters
    int i = 0;         // For reading payloads from parser
    
    // Initialize CircularBuffer
    cb.head = 0;
    cb.tail = 0; 
    cb.to_read = 0;
    
    // Initialize ParserState
    parser_state pstate = {
        .state = STATE_DOLLAR,
        .index_type = 0,
        .index_payload = 0
    };
    
    // Configure INT1 (mapped to RE8)
    TRISEbits.TRISE8 = 1;     // Set RE8 as input
    RPINR0bits.INT1R = 0x58;  // Remap RE8 to INT1 (RPI88 -> 0x58)
    INTCON2bits.GIE = 1;      // Enable global interrupts
    IFS1bits.INT1IF = 0;      // Clear INT1 interrupt flag
    
    // Enable Interrupts
    IEC1bits.INT1IE = 1;      // Enable INT1 interrupt
    IEC0bits.T2IE = 1;        // Enable Timer2 Interrupt 
    IEC1bits.U2RXIE = 1;      // enable interrupt for UART    
     
    // Scheduler configuration
    heartbeat schedInfo[MAX_TASKS] = {
        // LedA0 Blinking Task
        { .n = 0, .N = 1000, .f = task_blinkA0, .params = NULL, .enable = 1 },
        
        // Left and Right Indicators Blinking Task
        { .n = 0, .N = 1000, .f = task_blink_indicators, .params = &control_data, .enable = 1 },
        
        // Send Battery Task
        { .n = 0, .N = 1000, .f = task_send_battery, .params = NULL, .enable = 1 },
        
        // Send Distance Task
        { .n = 0, .N = 10000, .f = task_send_distance, .params = NULL, .enable = 1 },
        
        // Send Duty Cycle Task
        { .n = 0, .N = 10000, .f = task_send_dutycycle, .params = NULL, .enable = 1 }
    };
    
    // Control loop frequency = 1 kHz (1 ms period)
    tmr_setup_period(TIMER1, 1); 
    
    while(1) {             
        // ADC sampling
        do{
            if (AD1CON1bits.SAMP == 0)
                AD1CON1bits.SAMP = 1;      // Start sampling
        } while(!AD1CON1bits.DONE);        // Wait for sampling completion
        
        float distance = getMeasurements(DISTANCE);
        
        if (cb.to_read > 0) {          
            IEC1bits.U2RXIE = 0;                // Disable UART2 Receiver Interrupt
            int read = cb_pop(&cb, &readChar);  // Pop data from buffer
            IEC1bits.U2RXIE = 1;                // Enable UART2 Receiver Interrupt

            if (read == 1) {  
                while (U2STAbits.UTXBF == 1); // Wait until the Transmit Buffer is not full 
                                
                int ret = parse_byte(&pstate, readChar);
                if (ret == NEW_MESSAGE) {                    
                    if (strcmp(pstate.msg_type, "PCTH") == 0) { 
                        control_data.MINTH = extract_integer(pstate.msg_payload);
                        i = next_value(pstate.msg_payload, i);
                        control_data.MAXTH = extract_integer(pstate.msg_payload + i); 
                        // Optionally send confirmation over UART
                        //send_uart("OK");                           
                    } else {
                        // Optionally send error over UART
                        //send_uart("ERR");     
                    }
                }                
            }
        }
        
        // State machine handling
        switch(control_data.state) {
            case WaitForStart:
                PWMstop();  // Stop motors when waiting for start
                
                // Reset the lights indicators
                LATAbits.LATA7 = 0;  // Beam lights on
                LATFbits.LATF0 = 0;  // Brakes off
                LATGbits.LATG1 = 0;  // Low intensity lights off
                break;
            
            case Moving:
                if (distance < control_data.MINTH) {
                    control_data.surge = 0;
                    control_data.yaw_rate = 100;
                } else if (distance > control_data.MAXTH) {
                    control_data.surge = 100;
                    control_data.yaw_rate = 0;
                } else {
                    // Proportional control parameters
                    const int surgeGain = 2, yawScale = 500;

                    control_data.surge = distance * surgeGain; 
                    control_data.yaw_rate = yawScale / distance;
                }
                
                // Lights control
                if (control_data.surge > 50) {
                    LATAbits.LATA7 = 1;  // Beam lights on
                    LATFbits.LATF0 = 0;  // Brakes off
                    LATGbits.LATG1 = 0;  // Low intensity lights off
                } else {
                    LATAbits.LATA7 = 0;  // Beam lights off
                    LATFbits.LATF0 = 1;  // Brakes on
                    LATGbits.LATG1 = 1;  // Low intensity lights on
                }
                
                if (control_data.yaw_rate > 15) {
                    LATBbits.LATB8 = 0;  // Left indicator off
                } else {
                    LATBbits.LATB8 = 0;  // Left indicator off
                    LATFbits.LATF1 = 0;  // Right indicator off
                }
                
                // Set PWM duty cycle based on surge and yaw rate
                PWMstart(&control_data);
                break;
        }
        
        scheduler(schedInfo, MAX_TASKS);       
        tmr_wait_period(TIMER1);
    }
    
    return 0;
}