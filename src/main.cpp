#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "spiAVR.h"

#define NUM_TASKS 4 // TODO: Change to the number of tasks being used

unsigned char digits[4];
unsigned int score = 0;

unsigned char matrixState[8] = {0};

#define JOY_LOW 200
#define JOY_HIGH 800
#define JOY_CENTER_LOW 500
#define JOY_CENTER_HIGH 600

unsigned int startX = 8;
unsigned int startY = 1;

unsigned int userX = startX;
unsigned int userY = startY;

int visited[9][9] = {0};
unsigned int prevUserX = 0;
unsigned int prevUserY = 0;

int userBlink = 1;

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
const unsigned long JOY_PERIOD = 200;
const unsigned long VISITED_PERIOD = 100;
const unsigned long MATRIX_DISP_PERIOD = 503;

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

    // send new data
    unsigned char data = seg_data[digit];

    for (int i = 0; i < 8; i++)
    {

        PORTD = SetBit(PORTD, 5, (data & (1 << (7 - i))) ? 1 : 0);

        // Clock (shift)
        PORTD = SetBit(PORTD, 6, 1);

        PORTD = SetBit(PORTD, 6, 0);
    }

    // Latch (output)
    PORTD = SetBit(PORTD, 7, 1);
    PORTD = SetBit(PORTD, 7, 0);
}

void setMatrixLED(unsigned char row, unsigned char col, bool state)
{
    if (row < 1 || row > 8 || col < 1 || col > 8)
        return;

    unsigned char x = row;
    unsigned char y = 8 - col;

    if (state)
        matrixState[x] |= (1 << y);
    else
        matrixState[x] &= ~(1 << y);

    PORTB = SetBit(PORTB, PIN_SS, 0);
    SPI_SEND(x);
    _delay_us(4);
    SPI_SEND(matrixState[x]);
    PORTB = SetBit(PORTB, PIN_SS, 1);
}

void clearMatrix()
{
    for (unsigned char i = 1; i <= 8; i++)
    {
        PORTB = SetBit(PORTB, PIN_SS, 0);
        SPI_SEND(i);
        SPI_SEND(0x00);
        PORTB = SetBit(PORTB, PIN_SS, 1);
    }
}

void clearVisited()
{
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            visited[i][j] = 0;
        }
    }
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

int DisplayTick(int state)
{
    int currNum = score;

    for (int i = 0; i < 4; i++)
    {
        digits[i] = currNum % 10;
        currNum /= 10;
    }

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

    PORTB = SetBit(PORTB, 0, 1);
    PORTD = SetBit(PORTD, 2, 1);
    PORTD = SetBit(PORTD, 3, 1);
    PORTD = SetBit(PORTD, 4, 1);

    switch (state)
    {
    case D1:
        outNum(digits[0]);
        PORTB = SetBit(PORTB, 0, 0);
        break;

    case D2:
        if (score >= 10)
        {
            outNum(digits[1]);
            PORTD = SetBit(PORTD, 2, 0);
        }
        else
        {
            outNum(10);
        }
        break;

    case D3:
        if (score >= 100)
        {
            outNum(digits[2]);
            PORTD = SetBit(PORTD, 3, 0);
        }
        else
        {
            outNum(10);
        }
        break;

    case D4:
        if (score >= 1000)
        {
            outNum(digits[3]);
            PORTD = SetBit(PORTD, 4, 0);
        }
        else
        {
            outNum(10);
        }
        break;

    default:
        break;
    }

    return state;
}

enum JoyStates
{
    JOY_INIT,
    JOY_WAIT,
    JOY_HOLD,
    JOY_XY_MOVE,
    JOY_BTN_PRESS,
    JOY_BTN_RELEASE
} joyState;

int JoystickTick(int state)
{
    unsigned int x_pos = ADC_read(1);
    unsigned int y_pos = ADC_read(0);
    unsigned int btn_pressed = ADC_read(2) < 200;

    // State transitions
    switch (state)
    {
    case JOY_INIT:
        state = JOY_WAIT;
        break;

    case JOY_WAIT:

        if (x_pos < JOY_LOW || x_pos > JOY_HIGH || y_pos < JOY_LOW || y_pos > JOY_HIGH)
        {
            state = JOY_XY_MOVE;
        }
        if (btn_pressed)
        {
            state = JOY_BTN_PRESS;
        }
        break;

    case JOY_XY_MOVE:
        state = JOY_HOLD;
        break;

    case JOY_HOLD:
        if (x_pos > JOY_CENTER_LOW && x_pos < JOY_CENTER_HIGH && y_pos > JOY_CENTER_LOW && y_pos < JOY_CENTER_HIGH)
        {
            state = JOY_WAIT;
        }
        break;

    case JOY_BTN_PRESS:
        if (!btn_pressed)
        {
            state = JOY_BTN_RELEASE;
        }
        break;

    case JOY_BTN_RELEASE:
        state = JOY_WAIT;

        // reset everything
        userX = startX;
        userY = startY;
        clearVisited();
        clearMatrix();
        score = 0;
        break;

    default:
        state = JOY_INIT;
        break;
    }

    // State actions
    switch (state)
    {
    case JOY_XY_MOVE:
        if (y_pos < JOY_LOW && userY > 1)
        {
            userY--;
        }
        else if (y_pos > JOY_HIGH && userY < 8)
        {
            userY++;
        }

        if (x_pos < JOY_LOW && userX > 1)
        {
            userX--;
        }
        else if (x_pos > JOY_HIGH && userX < 8)
        {
            userX++;
        }
        break;

    default:
        break;
    }

    return state;
}

int VisitedTick(int state)
{

    if (userX != prevUserX || userY != prevUserY)
    {

        if (visited[userX][userY] == 0)
        {

            visited[userX][userY] = 1;

            if (score < 9999)
            {
                score += 1;
            }
        }
        else
        {
            if (score > 0)
            {
                score -= 1;
            }
        }

        prevUserX = userX;
        prevUserY = userY;
    }

    return state;
}

int MatrixDispTick(int state)
{
    for (unsigned int i = 1; i < 9; i++)
    {
        for (unsigned int j = 1; j < 9; j++)
        {
            if (i == userX && j == userY)
            {
                setMatrixLED(i, j, userBlink);
                userBlink = !userBlink;
                continue;
            }

            if (visited[i][j])
            {
                setMatrixLED(i, j, true);
            }

            else
            {
                setMatrixLED(i, j, false);
            }
        }
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

    // initialize 8x8 led matrix
    SPI_INIT();
    MAX7219_INIT();

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

    tasks[2].state = 0;
    tasks[2].period = VISITED_PERIOD;
    tasks[2].elapsedTime = 0;
    tasks[2].TickFct = &VisitedTick;

    tasks[3].state = 0;
    tasks[3].period = MATRIX_DISP_PERIOD;
    tasks[3].elapsedTime = 0;
    tasks[3].TickFct = &MatrixDispTick;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1)
    {
    }

    return 0;
}