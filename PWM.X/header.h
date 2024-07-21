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
#define DutyCycleMax (FCY / 10000)         // 100% of TCY/OC_Freq -> 72000000/10000 = 72000 (Note: The frequency is assigned as a specific)

// Definition of Timer related functions
void tmr_setup_period(int timer, int ms); 
void tmr_wait_period(int timer);
void tmr_wait_ms(int timer, int ms);

// Definition of PWM related functions
void PWMsetup();
void PWMstop();
void PWMstart();

#endif	