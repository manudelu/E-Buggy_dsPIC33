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
#define MAXTH 50      // Maximum distance threshold

#define STATE_DOLLAR  (1) // we discard everything until a dollar is found
#define STATE_TYPE    (2) // we are reading the type of msg until a comma is found
#define STATE_PAYLOAD (3) // we read the payload until an asterix is found
#define NEW_MESSAGE (1)   // new message received and parsed completely
#define NO_MESSAGE (0)    // no new messages

#define BUFFER_SIZE 13   //9.6 byte/s -> 10 is just enough, 13: final size of cb (10 + 25%(10))

// Define the CircularBuffer structure
typedef struct {
    char buffer[BUFFER_SIZE];
    int head;
    int tail;
    int to_read;
} CircularBuffer;

typedef struct { 
	int state;
	char msg_type[6];       // type is 5 chars + string terminator
	char msg_payload[100];  // assume payload cannot be longer than 100 chars
	int index_type;
	int index_payload;
} parser_state;

typedef struct {
    int n;
    int N;
    int enable;
    void (*f)(void *);
    void* params;
} heartbeat;

// Parser related functions
/*
Requires a pointer to a parser state, and the byte to process.
returns NEW_MESSAGE if a message has been successfully parsed.
The result can be found in msg_type and msg_payload.
Parsing another byte will override the contents of those arrays.
*/
int parse_byte(parser_state* ps, char byte);
/*
Takes a string as input, and converts it to an integer. Stops parsing when reaching
the end of string or a ","
the result is undefined if a wrong string is passed
*/
int extract_integer(const char* str);
/*
The function takes a string, and an index within the string, and returns the index where the next data can be found
Example: with the string "10,20,30", and i=0 it will return 3. With the same string and i=3, it will return 6.
*/  
int next_value(const char* msg, int i);

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
void send_uart(char* data);

// PWM related functions
void PWMsetup(int PWM_freq);
void PWMstop();
void PWMstart(int surge, int yaw_rate);

void cb_push(volatile CircularBuffer *cb, char data);

// Scheduler related functions
void scheduler(heartbeat schedInfo[], int nTasks);
void task_control_lights(void* param);
void task_send_distance(void* param);
void task_send_battery(void* param);
void task_send_dutycycle(void* param);

float getDistance();


#endif	