#include "timerISR.h"
#include "helper.h"
#include "periph.h"

#include "serialATmega.h"

#define NUM_TASKS 2 // TODO: Change to the number of tasks being used

// Task struct for concurrent synchSMs implmentations
typedef struct _task
{
    signed char state;         // Task's current state
    unsigned long period;      // Task period
    unsigned long elapsedTime; // Time elapsed since last task tick
    int (*TickFct)(int);       // Task tick function
} task;

// TODO: Define Periods for each task
//  e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long GCD_PERIOD = 1; // TODO:Set the GCD Period

const unsigned long DISP_PERIOD = 5;
const unsigned long JOY_PERIOD = 1000;

task tasks[NUM_TASKS]; // declared task array with 5 tasks

// TODO: Declare your tasks' function and their states here

void TimerISR()
{
    for (unsigned int i = 0; i < NUM_TASKS; i++)
    { // Iterate through each task in the task array
        if (tasks[i].elapsedTime == tasks[i].period)
        {                                                      // Check if the task is ready to tick
            tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
            tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
        }
        tasks[i].elapsedTime += GCD_PERIOD; // Increment the elapsed time by GCD_PERIOD
    }
}

void outNum(unsigned char digit)
{
    static const unsigned char seg_data[] = {
        0b00111111, // 0
        0b00000110, // 1
        0b01011011, // 2
        0b01001111, // 3
        0b01100110, // 4
        0b01101101, // 5
        0b01111101, // 6
        0b00000111, // 7
        0b01111111, // 8
        0b01101111, // 9
        0b00000000  // blank
    };

    // Get the segment pattern for the digit
    unsigned char data = seg_data[digit];

    // Shift out the data
    for (int i = 0; i < 8; i++)
    {
        // Set data bit
        PORTD = (PORTD & ~(1 << 5)) | ((data & (1 << (7 - i))) ? (1 << 5) : 0);

        // Clock pulse
        PORTD |= (1 << 6); // SRCLK high
        _delay_us(1);
        PORTD &= ~(1 << 6); // SRCLK low
        _delay_us(1);
    }

    // Latch pulse
    PORTD |= (1 << 7); // RCLK high
    _delay_us(1);
    PORTD &= ~(1 << 7); // RCLK low
    _delay_us(1);
}

// TODO: Create your tick functions for each task

enum DispStates
{
    D_INIT,
    D1,
    D2,
    D3,
    D4
} dispState;

unsigned char digits[4];
unsigned int number = 0;

int DisplayTick(int state)
{
    int currNum = number;

    for (int i = 0; i < 4; i++)
    {
        digits[i] = currNum % 10;
        currNum /= 10;
    }

    // State transitions
    switch (state)
    {
    case D_INIT:
        state = D1;
        break;
    case D1:
        state = D2;
        break;
    case D2:
        state = D3;
        break;
    case D3:
        state = D4;
        break;
    case D4:
        state = D1;
        break;
    default:
        state = D_INIT;
        break;
    }

    // State actions
    switch (state)
    {
    case D1:
        PORTB &= ~(1 << 0);
        PORTD |= (1 << 2) | (1 << 3) | (1 << 4);
        outNum(digits[0]);
        break;

    case D2:
        PORTB |= (1 << 0);
        PORTD &= ~(1 << 2);
        PORTD |= (1 << 3) | (1 << 4);

        if (number < 10)
        {
            outNum(10);
        }
        else
        {
            outNum(digits[1]);
        }
        break;

    case D3:
        PORTB |= (1 << 0);
        PORTD &= ~(1 << 3);
        PORTD |= (1 << 2) | (1 << 4);
        if (number < 100)
        {
            outNum(10);
        }
        else
        {
            outNum(digits[2]);
        }
        break;

    case D4:
        PORTB |= (1 << 0);
        PORTD &= ~(1 << 4);
        PORTD |= (1 << 2) | (1 << 3);
        if (number < 1000)
        {
            outNum(10);
        }
        else
        {
            outNum(digits[3]);
        }
        break;

    default:
        break;
    }

    return state;
}

#define JOY_LOW 200
#define JOY_HIGH 800

enum JoyStates
{
    JOY_INIT,
    JOY_WAIT,
    JOY_X_MOVE,
    JOY_Y_MOVE
} joyState;

unsigned char joystickReleased = 1;

int JoystickTick(int state)
{

    unsigned int xPos = ADC_read(1);
    unsigned int yPos = ADC_read(0);
    serial_println(xPos);

    // State transitions
    switch (state)
    {
    case JOY_INIT:
        state = JOY_WAIT;
        break;

    case JOY_WAIT:
        if (joystickReleased)
        {
            if (xPos < JOY_LOW || xPos > JOY_HIGH)
            {
                state = JOY_X_MOVE;
                joystickReleased = 0;
            }
            else if (yPos < JOY_LOW || yPos > JOY_HIGH)
            {
                state = JOY_Y_MOVE;
                joystickReleased = 0;
            }
        }
        break;

    case JOY_X_MOVE:
        if (xPos >= JOY_LOW && xPos <= JOY_HIGH)
        {
            joystickReleased = 1;
            state = JOY_WAIT;
        }
        break;

    case JOY_Y_MOVE:
        if (yPos >= JOY_LOW && yPos <= JOY_HIGH)
        {
            joystickReleased = 1;
            state = JOY_WAIT;
        }
        break;

    default:
        state = JOY_INIT;
        break;
    }

    // State actions
    switch (state)
    {
    case JOY_X_MOVE:
        if (xPos < JOY_LOW)
            number++;
        else if (xPos > JOY_HIGH)
            number++;
        break;

    case JOY_Y_MOVE:
        if (yPos < JOY_LOW)
            number++;
        else if (yPos > JOY_HIGH)
            number++;
        break;

    default:
        break;
    }

    return state;
}

int main(void)
{
    // TODO: initialize all your inputs and ouputs
    DDRB = 0xFF;
    DDRD = 0xFF;
    DDRC = 0xF8;

    PORTC = 0x07;
    PORTB = 0x00;
    PORTD = 0x00;

    ADC_init(); // initializes ADC
    serial_init(9600);

    // TODO: Initialize tasks here
    //  e.g.
    //  tasks[0].period = ;
    //  tasks[0].state = ;
    //  tasks[0].elapsedTime = ;
    //  tasks[0].TickFct = ;

    tasks[0].state = 0;
    tasks[0].period = DISP_PERIOD;
    tasks[0].elapsedTime = 0;
    tasks[0].TickFct = &DisplayTick;

    tasks[1].state = JOY_INIT;
    tasks[1].period = JOY_PERIOD;
    tasks[1].elapsedTime = 0;
    tasks[1].TickFct = &JoystickTick;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1)
    {
    }

    return 0;
}