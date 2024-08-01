#include "xc.h"
#include "header.h"
#include <math.h>

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

// Set all buggy's lights as output
void LigthsSetup(void) {
    TRISAbits.TRISA0 = 0; // Led1 (RA0) 
    TRISBbits.TRISB8 = 0; // Left Side Lights (RB8)
    TRISFbits.TRISF1 = 0; // Right Side Lights (RF1)
    TRISAbits.TRISA7 = 0; // Beam Headlights (RA7)
    TRISFbits.TRISF0 = 0; // Brakes (RF0)
    TRISGbits.TRISG1 = 0; // Low Intensity Lights (RG1)
}

// Function to setup the ADC
void ADCsetup(void) {
    // Enable IR sensor on the digital I/O on RB9
    TRISBbits.TRISB9 = 0;
    LATBbits.LATB9 = 1;  
    // IR Sensor analog configuration AN14 (mounted on front right mikroBUS)
    TRISBbits.TRISB14 = 1;
    ANSELBbits.ANSB14 = 1;  
    
    // Enable Battery Sensor analog configuration AN11
    TRISBbits.TRISB11 = 1;
    ANSELBbits.ANSB11 = 1;
    
    AD1CON3bits.ADCS = 14;   // Tad = 8*Tcy = 8/72MHz = 111.11ns
    AD1CON1bits.ASAM = 0;   // Manual sampling
    AD1CON1bits.SSRC = 7;   // Automatic conversion
    AD1CON3bits.SAMC = 16;  // Sampling lasts 16 Tad 
    AD1CON2bits.CHPS = 0;   // Use 1-channel (CH0) mode
    AD1CON1bits.SIMSAM = 0; // Sequential sampling
    
    AD1CON2bits.CSCNA = 1;  // Scan mode enabled
    AD1CSSLbits.CSS14 = 1;  // Scan for AN14 IR sensor
    AD1CSSLbits.CSS11 = 1;  // Scan for AN14 IR sensor
    AD1CON2bits.SMPI = 1;   // N-1 channels
    
    AD1CON1bits.ADON = 1;   // Turn ADC on
}

// Function to setup the UART (mounted on mikroBUS2, 9600 bps)
void UARTsetup(void){
    // From documentation (pag.9 datasheet):
    // RD0(RP64) -> TX
    // RD11(RPI75) -> RX
    // For Transitter: Find 'OUTPUT SELECTION FOR REMAPPABLE PINS' in datasheet
    // Under U2TX -> 000011 -> 0x03 in hex
    // For Receiver: Find 'INPUT PIN SELECTION FOR SELECTABLE INPUT SOURCES' in datasheet
    // Under RPI75 -> 1001011 -> 0x4B
    
    RPOR0bits.RP64R = 0x03;   //Map UART2 TX to pin RD0 which is RP64
    RPINR19bits.U2RXR = 0x4B; //Map UART2 RX to pin RD11 which is RPI75
    
    U2BRG = FCY / (16L * 9600) - 1;
    U2MODEbits.UARTEN = 1;    // Enable UART 
    U2STAbits.UTXEN = 1;      // Enable Transmission (must be after UARTEN)
}

// Function to set the PWM with Output Compare module
void PWMsetup(int PWM_freq){
    
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
    OC1RS = OC2RS = OC3RS = OC4RS = FCY/PWM_freq; // 100% of TCY/OC_Freq 
    
    // Setup OCxR to 0 at start
    OC1R = OC2R = OC3R = OC4R = 0;
}

// Function to stop PWM
void PWMstop(void){
    OC1R = OC2R = OC3R = OC4R = 0;
}

// Function to start PWM and control motor direction
void PWMstart(int surge, int yaw_rate){  
    // Calculate left and right PWM values
    float left_pwm = surge + yaw_rate;
    float right_pwm = surge - yaw_rate;
    
    // Normalize PWM values to ensure they are within the range -100% to 100%
    float max_pwm = fmax(fabs(left_pwm), fabs(right_pwm));
    if (max_pwm > 100) {
        left_pwm /= max_pwm;
        right_pwm /= max_pwm;
    }
    
    // Generate PWM signals for the wheels
    if (left_pwm > 0) {
        OC1R = 0; 
        OC2R = left_pwm/100 * OC2RS; 
    } else {
        OC1R = (-left_pwm/100) * OC1RS; 
        OC2R = 0; 
    }
    
    if (right_pwm > 0) {
        OC3R = 0; 
        OC4R = right_pwm/100 * OC4RS;
    } else {
        OC3R = (-right_pwm/100) * OC3RS;
        OC4R = 0; 
    }
}