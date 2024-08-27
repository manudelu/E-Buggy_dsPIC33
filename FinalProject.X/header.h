/*
 * File:   headers.h
 * Authors: Delucchi Manuel S4803977, Matteo Cappellini S4822622
 */

#ifndef HEADER_H
#define	HEADER_H

#include <xc.h> 

// System Clock Configuration
#define FOSC 144000000  // Fosc = Fin*(M/(N1*N2)) = 8Mhz*(72/(2*2)) = 144MHz
#define FCY (FOSC / 2)  // Fcy = Fosc/2 = 72MHz

// Timer Configuration
#define TIMER1 1
#define TIMER2 2
#define MaxInt 65535    // Max number that can be represented with 16bit register 

// Buffer and Task Configuration
#define BUFFER_SIZE 13   
// 9.6 byte/s -> 10 is just enough, 13: final size of cb (10 + 25%(10))
#define MAX_TASKS 5

// Message Parsing States
#define STATE_DOLLAR  (1) // we discard everything until a dollar is found
#define STATE_TYPE    (2) // we are reading the type of msg until a comma is found
#define STATE_PAYLOAD (3) // we read the payload until an asterix is found
#define NEW_MESSAGE (1)   // new message received and parsed completely
#define NO_MESSAGE (0)    // no new messages

// Measurement Flags
#define DISTANCE (1)
#define BATTERY (0)

// CircularBuffer structure
typedef struct {
    char buffer[BUFFER_SIZE];
    int head;
    int tail;
    int to_read;
} CircularBuffer;

// Parser State Structure
typedef struct { 
	int state;
	char msg_type[6];       // Type is 5 chars + string terminator
	char msg_payload[100];  // Payload cannot exceed 100 chars
	int index_type;
	int index_payload;
} parser_state;

// Heartbeat Structure
typedef struct {
    int n;
    int N;
    int enable;
    void (*f)(void *);
    void* params;
} heartbeat;

// State Machine States
typedef enum {
    WaitForStart,
    Moving
} State;

extern volatile State state;
extern volatile int yaw_rate;

// LED related functions
void LigthsSetup();

// Timer related functions
void tmr_setup_period(int timer, int ms); 
void tmr_wait_period(int timer);

// ADC related functions
void ADCsetup();
float getMeasurements(int flag);

// PWM related functions
void PWMsetup(int PWM_freq);
void PWMstop();
void PWMstart(int surge, int yaw_rate);

// UART related functions
void UARTsetup();
void send_uart(char* data);

// Circular Buffer related functions
void cb_push(volatile CircularBuffer *cb, char data);
int cb_pop(volatile CircularBuffer *cb, char *data);

// Parser related functions
int parse_byte(parser_state* ps, char byte);
int extract_integer(const char* str);
int next_value(const char* msg, int i);

// Scheduler related functions
void scheduler(heartbeat schedInfo[], int nTasks);
void task_blinkA0 (void* param);
void task_blink_indicators (void* param);
void task_send_distance(void* param);
void task_send_battery(void* param);
void task_send_dutycycle(void* param);

#endif	