#include "xc.h"
#include "header.h"
#include <math.h>

// Set up the timer with a specified period in milliseconds
void tmr_setup_period(int timer, int ms){   
    long steps = FCY * (ms / 1000.0); // Calculate number of clock cycles
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
   
    // Configure the specified timer
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

// Wait for a timer period to complete
void tmr_wait_period(int timer){
    switch(timer){
        case TIMER1:
            while(IFS0bits.T1IF == 0){};
            IFS0bits.T1IF = 0; // Reset Timer1 interrupt flag
            break;
            
        case TIMER2:
            while(IFS0bits.T2IF == 0){};
            IFS0bits.T2IF = 0; // Reset Timer2 interrupt flag
            break;
    }
}

// Wait for a specified number of milliseconds using a timer
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
    // IR Sensor analog configuration AN15 (mounted on front left mikroBUS)
    TRISBbits.TRISB15 = 1;
    ANSELBbits.ANSB15 = 1; 
    
    TRISAbits.TRISA3 = 0; // IR distance sensor enable line  
    LATAbits.LATA3 = 1; // enable EN pin
    
    // Enable Battery Sensor analog configuration AN11
    TRISBbits.TRISB11 = 1;
    ANSELBbits.ANSB11 = 1;
    
    AD1CON3bits.ADCS = 14;  // Tad = 8*Tcy = 8/72MHz = 111.11ns
    AD1CON1bits.ASAM = 0;   // Manual sampling
    AD1CON1bits.SSRC = 7;   // Automatic conversion
    AD1CON3bits.SAMC = 16;  // Sampling lasts 16 Tad 
    AD1CON2bits.CHPS = 0;   // Use 1-channel (CH0) mode
    AD1CON1bits.SIMSAM = 0; // Sequential sampling
    
    AD1CON2bits.CSCNA = 1;  // Scan mode enabled
    AD1CSSLbits.CSS15 = 1;  // Scan for AN14 IR sensor
    AD1CSSLbits.CSS11 = 1;  // Scan for AN11 Battery sensor
    AD1CON2bits.SMPI = 1;   // N-1 channels
    
    AD1CON1bits.ADON = 1;   // Turn on the ADC module
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
        left_pwm *= 0.9;
        right_pwm *= 0.9;
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

// Read IR or Battery sensor tension
float getMeasurements(int flag){
    float ADCValue, V;
    if(flag){
        ADCValue = ADC1BUF1;        // Get ADC value 
        V = ADCValue * 3.3/1024.0;  // Convert to voltage
        float distance = 100 * (2.34 - 4.74 * V + 4.06 * powf(V,2) - 1.60 * powf(V,3) + 0.24 * powf(V,4));
        return distance;
    }
    else{
        ADCValue = ADC1BUF0;
        V = ADCValue * 3.3/1024.0;
        const float R49 = 100.0, R51 = 100.0, R54 = 100.0;
        float Rs = R49 + R51;
        float battery = V * (Rs + R54) / R54;
        return battery;
    }
   
}

void scheduler(heartbeat schedInfo[], int nTasks) 
{
    int i;
    for (i = 0; i < nTasks; i++) {
        schedInfo[i].n++;
        if (schedInfo[i].enable == 1 && schedInfo[i].n >= schedInfo[i].N) {
            schedInfo[i].f(schedInfo[i].params);            
            schedInfo[i].n = 0;
        }
    }
}

void task_send_distance(void* param){
    float distance = getMeasurements(DISTANCE);
    
    char buffer[16];
    sprintf(buffer, "MDIST,%d*\n", (int)distance); 
    send_uart(buffer);
}

void task_send_battery(void* param){
    float battery = getMeasurements(BATTERY);
    
    char buffer[16];
    sprintf(buffer, "$MBATT,%.2f*\n", battery);
    send_uart(buffer);
}

void task_send_dutycycle(void* param){
    // OCxR - Sets the time the signal is high
    // OCxRS - Sets the period of the PWM signal
    int dc1 = (OC1R * 100) / OC1RS;  
    int dc2 = (OC2R * 100) / OC2RS;
    int dc3 = (OC3R * 100) / OC3RS;
    int dc4 = (OC4R * 100) / OC4RS;
    
    char buffer[16];
    sprintf(buffer, "$MPWM,%d,%d,%d,%d*\n", dc1, dc2, dc3, dc4);
    send_uart(buffer);
}

// Utility function to send data over UART
void send_uart(char* data) {
    for (int i = 0; i < strlen(data); i++){
        while (U2STAbits.UTXBF == 1); // Wait until the Transmit Buffer is not full
        U2TXREG = data[i];
    }
}

int parse_byte(parser_state* ps, char byte) {
    switch (ps->state) {
        case STATE_DOLLAR:
            if (byte == '$') {
                ps->state = STATE_TYPE;
                ps->index_type = 0;
            }
            break;
        case STATE_TYPE:
            if (byte == ',') {
                ps->state = STATE_PAYLOAD;
                ps->msg_type[ps->index_type] = '\0';
                ps->index_payload = 0; // initialize properly the index
            } else if (ps->index_type == 6) { // error! 
                ps->state = STATE_DOLLAR;
                ps->index_type = 0;
			} else if (byte == '*') {
				ps->state = STATE_DOLLAR; // get ready for a new message
                ps->msg_type[ps->index_type] = '\0';
				ps->msg_payload[0] = '\0'; // no payload
                return NEW_MESSAGE;
            } else {
                ps->msg_type[ps->index_type] = byte; // ok!
                ps->index_type++; // increment for the next time;
            }
            break;
        case STATE_PAYLOAD:
            if (byte == '*') {
                ps->state = STATE_DOLLAR; // get ready for a new message
                ps->msg_payload[ps->index_payload] = '\0';
                return NEW_MESSAGE;
            } else if (ps->index_payload == 100) { // error
                ps->state = STATE_DOLLAR;
                ps->index_payload = 0;
            } else {
                ps->msg_payload[ps->index_payload] = byte; // ok!
                ps->index_payload++; // increment for the next time;
            }
            break;
    }
    return NO_MESSAGE;
}

int extract_integer(const char* str) {
	int i = 0, number = 0, sign = 1;
	
	if (str[i] == '-') {
		sign = -1;
		i++;
	}
	else if (str[i] == '+') {
		sign = 1;
		i++;
	}
	while (str[i] != ',' && str[i] != '\0') {
		number *= 10; // multiply the current number by 10;
		number += str[i] - '0'; // converting character to decimal number
		i++;
	}
	return sign*number;
}		

int next_value(const char* msg, int i) {
	while (msg[i] != ',' && msg[i] != '\0') { i++; }
	if (msg[i] == ',')
		i++;
	return i;
}

// Function to push data into the circular buffer
void cb_push(volatile CircularBuffer *cb, char data) {
    
    cb->buffer[cb->head] = data;  // Load the data into the buffer at the current head position
    cb->head++;                   // Move the head to the next data offset
    cb->to_read++;                // Increment the variable keeping track of the char available for reading
    
    // Wrap around to the beginning if we've reached the end of the buffer
    if (cb->head == BUFFER_SIZE)
        cb->head = 0;             
}


