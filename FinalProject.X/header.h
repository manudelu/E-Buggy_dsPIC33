#ifndef HEADER_H
#define	HEADER_H

#include <xc.h> 

#define TIMER1 1
#define TIMER2 2
#define FOSC 144000000  // Fosc = Fin*(M/(N1*N2)) = 8Mhz*(72/(2*2)) = 144MHz
#define FCY (FOSC / 2)  // Fcy = Fosc/2 = 72MHz
#define MaxInt 65535    // Max number that can be represented with 16bit register 
#define MaxTime 200     // Maximum time period that the timer can handle in one go
                        // Tecnhically the max amount possible is 233ms: Tmax = MaxInt*Presc/Fcy, where Presc = 256

#define MINTH 25      // Minimum distance threshold
#define MAXTH 70      // Maximum distance threshold

// LED related functions
void LigthsSetup();

// Timer related functions
void tmr_setup_period(int timer, int ms); 
void tmr_wait_period(int timer);
void tmr_wait_ms(int timer, int ms);

// ADC related functions
void ADCsetup();

// UART related functions
void UARTsetup();

// PWM related functions
void PWMsetup(int PWM_freq);
void PWMstop();
void PWMstart(int surge, int yaw_rate);

#endif	