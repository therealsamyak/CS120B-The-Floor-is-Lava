#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "spiAVR.h"

#include "serialATmega.h"

#define NUM_TASKS 8 // TODO: Change to the number of tasks being used

unsigned char digits[4];
unsigned int score = 0;

unsigned char matrixState[8] = {0};

#define JOY_LOW 200
#define JOY_HIGH 800
#define JOY_CENTER_LOW 500
#define JOY_CENTER_HIGH 600
#define LONG_PRESS_THRESHOLD 10
#define MAX_MOVES 8

unsigned int startX = 4;
unsigned int startY = 1;

unsigned int userX = startX;
unsigned int userY = startY;

unsigned char game_mode = false;
unsigned char correct_moves[MAX_MOVES];
unsigned char move_count = 0;
unsigned char firstMoveRecorded = false;

int visited[9][9] = {0};
unsigned int prevUserX = 0;
unsigned int prevUserY = 0;

int userBlink = 1;

// rgb led stuff
unsigned int pwm_period = 10;
unsigned int red_duty_cycle = 0;
unsigned int red_counter = 0;
unsigned int green_duty_cycle = 0;
unsigned int green_counter = 0;
unsigned int blue_duty_cycle = 0;
unsigned int blue_counter = 0;

unsigned int color1[3] = {0};
unsigned int color2[3] = {0};

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
const unsigned long JOY_PERIOD = 150;
const unsigned long VISITED_PERIOD = 100;
const unsigned long MATRIX_DISP_PERIOD = 650;
const unsigned long RGB_LED_FLICKER_PERIOD = 650;
const unsigned long RGB_LED_PERIOD = 1;

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
        _delay_us(2);
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

void clearMasterMoves()
{
    for (int i = 0; i < MAX_MOVES; i++)
    {
        correct_moves[i] = 0;
    }
}

void playerReset()
{
    score = 100;
    move_count = 0;
    startY = correct_moves[0] % 10;
    startX = (correct_moves[0] - startY) / 10;
    prevUserX = -1;
    prevUserY = -1;
    userX = startX;
    userY = startY;
    clearVisited();
    clearMatrix();
}

void masterReset()
{
    userX = 4;
    userY = 1;
    prevUserX = -1;
    prevUserY = -1;
    score = 0;
    move_count = 0;
    game_mode = 0;
    firstMoveRecorded = false;
    clearVisited();
    clearMatrix();
    clearMasterMoves();
}
// led helper functions

void setRGBCycles(unsigned int red_percent, unsigned int green_percent, unsigned int blue_percent)
{
    if (red_percent > 100 ||
        green_percent > 100 ||
        blue_percent > 100 ||
        red_percent < 0 ||
        blue_percent < 0 ||
        green_percent < 0)
    {
        return;
    }

    red_duty_cycle = (red_percent * pwm_period) / 100;
    green_duty_cycle = (green_percent * pwm_period) / 100;
    blue_duty_cycle = (blue_percent * pwm_period) / 100;
}

void setColor1(uint8_t red, uint8_t green, uint8_t blue)
{
    color1[0] = red;
    color1[1] = green;
    color1[2] = blue;
}

void setColor2(uint8_t red, uint8_t green, uint8_t blue)
{
    color2[0] = red;
    color2[1] = green;
    color2[2] = blue;
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
    JOY_BTN_RELEASE,
    JOY_BTN_LONG_PRESS
} joyState;

unsigned int longPressTimer = 0;

int JoystickTick(int state)
{
    if (game_mode)
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
            else if (btn_pressed)
            {
                longPressTimer = 0;
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
                if (longPressTimer < LONG_PRESS_THRESHOLD)
                {
                    state = JOY_BTN_RELEASE;
                }
                else if (longPressTimer >= LONG_PRESS_THRESHOLD)
                {
                    state = JOY_BTN_LONG_PRESS;
                }
                longPressTimer = 0;
            }
            break;

        case JOY_BTN_RELEASE:
            state = JOY_WAIT;

            // reset everything
            playerReset();
            break;

        case JOY_BTN_LONG_PRESS:
            state = JOY_WAIT;
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

        case JOY_BTN_PRESS:
            longPressTimer++;
            break;

        case JOY_BTN_LONG_PRESS:
            masterReset();
            break;

        default:
            break;
        }
    }
    return state;
}

int MasterJoystickTick(int state)
{
    if (!game_mode)
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
            else if (btn_pressed)
            {
                longPressTimer = 0;
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
                if (longPressTimer < LONG_PRESS_THRESHOLD)
                {
                    state = JOY_BTN_RELEASE;
                }
                else if (longPressTimer >= LONG_PRESS_THRESHOLD)
                {
                    state = JOY_BTN_LONG_PRESS;
                }
                longPressTimer = 0;
            }
            break;

        case JOY_BTN_RELEASE:
            state = JOY_WAIT;

            if (userY <= 1 && !firstMoveRecorded)
            {
                firstMoveRecorded = true;
                startX = userX;
                startY = userY;
                correct_moves[move_count] = userX * 10 + userY;
                serial_println(correct_moves[move_count]);
                move_count++;
            }

            if (!game_mode && move_count >= MAX_MOVES)
            {
                move_count = 0;
                game_mode = true;
                playerReset();
            }
            break;

        case JOY_BTN_LONG_PRESS:
            state = JOY_WAIT;

            userX = 4;
            userY = 1;
            move_count = 0;
            firstMoveRecorded = false;
            clearMasterMoves();
            break;

        default:
            state = JOY_INIT;
            break;
        }

        // State actions
        switch (state)
        {
        case JOY_XY_MOVE:
            if (move_count < MAX_MOVES)
            {
                if (firstMoveRecorded)
                {
                    if (y_pos < JOY_LOW && userY > 1)
                    {
                        userY--;
                        if (x_pos < JOY_LOW && userX > 1)
                        {
                            userX--;
                        }
                        else if (x_pos > JOY_HIGH && userX < 8)
                        {
                            userX++;
                        }
                        correct_moves[move_count] = userX * 10 + userY;
                        serial_println(correct_moves[move_count]);
                        move_count++;
                    }
                    else if (y_pos > JOY_HIGH && userY < 8)
                    {
                        userY++;
                        if (x_pos < JOY_LOW && userX > 1)
                        {
                            userX--;
                        }
                        else if (x_pos > JOY_HIGH && userX < 8)
                        {
                            userX++;
                        }
                        correct_moves[move_count] = userX * 10 + userY;
                        serial_println(correct_moves[move_count]);
                        move_count++;
                    }
                }

                else
                {
                    if (x_pos < JOY_LOW && userX > 1)
                    {
                        userX--;
                    }
                    else if (x_pos > JOY_HIGH && userX < 8)
                    {
                        userX++;
                    }
                }
            }

            break;

        case JOY_BTN_PRESS:
            longPressTimer++;
            break;

        default:
            break;
        }
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
        }

        prevUserX = userX;
        prevUserY = userY;
    }

    return state;
}

int MatrixDispTick(int state)
{
    if (game_mode)
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
    }
    return state;
}

int MasterMatrixDispTick(int state)
{
    if (!game_mode)
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

                bool is_move = false;
                for (int k = 0; k < move_count; k++)
                {
                    unsigned int move = correct_moves[k];
                    unsigned int moveX = move / 10;
                    unsigned int moveY = move % 10;

                    if (i == moveX && j == moveY)
                    {
                        is_move = true;
                        break;
                    }
                }

                if (is_move)
                {
                    setMatrixLED(i, j, true);
                }
                else
                {
                    setMatrixLED(i, j, false);
                }
            }
        }
    }
    return state;
}

enum RGBStates
{
    RGB_INIT,
    RGB_COLOR_ONE,
    RGB_COLOR_TWO,
} rgbState;

int RgbLedFlicker(int state)
{
    // switch (state)
    // {
    // case RGB_INIT:
    //     state = RGB_COLOR_ONE;
    //     break;
    // case RGB_COLOR_ONE:
    //     state = RGB_COLOR_TWO;
    //     break;
    // case RGB_COLOR_TWO:
    //     state = RGB_COLOR_ONE;
    //     break;
    // default:
    //     state = RGB_INIT;
    //     break;
    // }

    if (game_mode == 0)
    {

        red_duty_cycle = color1[0];
        green_duty_cycle = color1[1];
        blue_duty_cycle = color1[2];
    }
    else if (game_mode == 1)
    {
        red_duty_cycle = color2[0];
        green_duty_cycle = color2[1];
        blue_duty_cycle = color2[2];
    }

    return state;
}

int RgbLedTick(int state)
{

    // red pin
    if (red_counter < red_duty_cycle)
    {
        PORTC = SetBit(PORTC, 3, 1);
    }
    else
    {
        PORTC = SetBit(PORTC, 3, 0);
    }

    // green pin
    if (green_counter < green_duty_cycle)
    {
        PORTC = SetBit(PORTC, 4, 1);
    }
    else
    {
        PORTC = SetBit(PORTC, 4, 0);
    }

    // blue pin
    if (blue_counter < blue_duty_cycle)
    {
        PORTC = SetBit(PORTC, 5, 1);
    }
    else
    {
        PORTC = SetBit(PORTC, 5, 0);
    }

    // update counter
    red_counter = (red_counter + 1) % pwm_period;
    green_counter = (green_counter + 1) % pwm_period;
    blue_counter = (blue_counter + 1) % pwm_period;

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

    serial_init(9600);

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

    tasks[4].state = 0;
    tasks[4].period = RGB_LED_PERIOD;
    tasks[4].elapsedTime = 0;
    tasks[4].TickFct = &RgbLedTick;

    tasks[5].state = RGB_INIT;
    tasks[5].period = RGB_LED_FLICKER_PERIOD;
    tasks[5].elapsedTime = 0;
    tasks[5].TickFct = &RgbLedFlicker;

    tasks[6].state = JOY_INIT;
    tasks[6].period = JOY_PERIOD;
    tasks[6].elapsedTime = 0;
    tasks[6].TickFct = &MasterJoystickTick;

    tasks[7].state = 0;
    tasks[7].period = MATRIX_DISP_PERIOD;
    tasks[7].elapsedTime = 0;
    tasks[7].TickFct = &MasterMatrixDispTick;

    TimerSet(GCD_PERIOD);
    TimerOn();

    setColor1(0, 40, 100);

    setColor2(20, 0, 100);
    // setColor2(0, 40, 100);
    // setColor2(0, 0, 0);

    while (1)
    {
    }

    return 0;
}