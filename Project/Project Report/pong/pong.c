





#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// tiva-c stuff - for uart/ints
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
//#include "driverlib/debug.h"
//#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
//#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

// Constants - for TG12864H3-05A LCD
#include "lcd.h"
#define SCLK GPIO_PIN_0 // Port D
#define CS GPIO_PIN_1 // Port D
#define A0 GPIO_PIN_2 // Port D
#define TX GPIO_PIN_3 // Port D
#define GRN GPIO_PIN_1 // Port E
#define BLU GPIO_PIN_2 // Port E
#define RED GPIO_PIN_3 // Port E
#define RST GPIO_PIN_1 // Port F






/**
 * Pong settings
 */

#define LCD_WIDTH 128
#define LCD_HEIGHT 64

#define BALL_RADIUS 2

#define PADDLE_HEIGHT 20
#define PADDLE_WIDTH 2
#define PADDLE_P1_X_POS LCD_WIDTH / 5
#define PADDLE_P2_X_POS LCD_WIDTH * 4 / 5

#define NUM_ROUNDS 5

/**
 * Pong types
 */

#define PLAYER_UNKNOWN 0
#define PLAYER_1 1
#define PLAYER_2 2

#define DIR_TYPE_STATIONARY 0
#define DIR_TYPE_UP 1
#define DIR_TYPE_DOWN 2
#define DIR_TYPE_LEFT 3
#define DIR_TYPE_RIGHT 4
 
/**
 * Global variables
 */

char g_score_p1 = 0x30; // ASCII '0'
char g_score_p2 = 0x30; // ASCII '0'

uint8_t g_ball_position_x = LCD_WIDTH / 2;
uint8_t g_ball_position_y = LCD_HEIGHT / 2;
uint8_t g_ball_direction_x = DIR_TYPE_STATIONARY;
uint8_t g_ball_direction_y = DIR_TYPE_STATIONARY;

uint8_t g_p1_position = LCD_HEIGHT / 2;
uint8_t g_p2_position = LCD_HEIGHT / 2;

uint8_t g_p1_move_dir = DIR_TYPE_STATIONARY;
uint8_t g_p2_move_dir = DIR_TYPE_STATIONARY;
uint8_t g_last_winner = PLAYER_UNKNOWN;
bool g_is_release = false;
bool g_is_game_over = false;

/* ========================================================================= */

/**
 * Pong drawing tools
 */

void DrawCentreDottedLine()
{
    uint8_t y;

    for (y = 0; y < LCD_HEIGHT; y++)
    {
			if (!(y % 5 == 0) && !(((y + 1) % 5 == 0))) // Ratio ensures dotted lines accross the with of the screen
        {
            DrawPixel(LCD_WIDTH / 2, y);
        }
    }
}

void DrawBall(uint8_t x, uint8_t y, bool clear)
{
    int8_t i, j;
        
    for (j = -BALL_RADIUS; j <= BALL_RADIUS; j++)
        for (i = -BALL_RADIUS; i <= BALL_RADIUS; i++)
            if (i * i + j * j <= BALL_RADIUS * BALL_RADIUS)
                if (clear)
                    ClearPixel(x + i, y + j);
                else
                    DrawPixel(x + i, y + j);
}

void DrawPaddleP1(uint8_t y)
{
    int8_t i, j;

    for (j = -PADDLE_HEIGHT / 2; j <= PADDLE_HEIGHT / 2; j++)
        for (i = 0; i < PADDLE_WIDTH; i++)
            DrawPixel(PADDLE_P1_X_POS + i, y + j);
}

void DrawPaddleP2(uint8_t y)
{
    int8_t i, j;

    for (j = -PADDLE_HEIGHT / 2; j <= PADDLE_HEIGHT / 2; j++)
        for (i = 0; i < PADDLE_WIDTH; i++)
            DrawPixel(PADDLE_P2_X_POS + i, y + j);
}

void DrawScores(uint8_t score_p1, uint8_t score_p2)
{
    CursorPos(5, 0);
    PutcharLCD(score_p1);
    CursorPos(15, 0);
    PutcharLCD(score_p2);
}

void DrawGameOver(uint8_t is_p1_winner)
{
    char text[] = "GAME  OVER";
    int8_t i;

    CursorPos(6, 3);
    for (i = 0; i < strlen(text); i++)
    {
        PutcharLCD(text[i]);
    }
}

void TestDrawing()
{
    DrawPaddleP1(g_p1_position);
    DrawPaddleP2(g_p2_position);
    DrawBall(g_ball_position_x, g_ball_position_y, false);
    DrawGameOver(false);
}

/* ========================================================================= */

/**
 * Gameplay tools
 */

void PongInit()
{
    DrawCentreDottedLine();
    DrawScores(g_score_p1, g_score_p2);
    DrawPaddleP1(g_p1_position);
    DrawPaddleP2(g_p2_position);
    DrawBall(g_ball_position_x, g_ball_position_y, false);
}



void ResetBall()
{
    DrawBall(g_ball_position_x, g_ball_position_y, true);
    g_ball_position_x = LCD_WIDTH / 2;
    g_ball_position_y = LCD_HEIGHT / 2;
    g_ball_direction_x = DIR_TYPE_STATIONARY;
    g_ball_direction_y = DIR_TYPE_STATIONARY;
    g_is_release = false;
}

void HandleScores()
{
    if (g_ball_position_x == 0)
    {
        g_score_p2++;
        ResetBall();
        g_last_winner = PLAYER_2;
    }
    if (g_ball_position_x == LCD_WIDTH)
    {
        g_score_p1++;
        ResetBall();
        g_last_winner = PLAYER_1;
    }

    if (g_score_p1 == 0x30 + NUM_ROUNDS)
    {
        DrawGameOver(true);
        g_is_game_over = true;
    }
    else if (g_score_p2 == 0x30 + NUM_ROUNDS)
    {
        DrawGameOver(true);
        g_is_game_over = true;
    }
}

/* ========================================================================= */

/**
 * Dynamics tools
 */

void MovePaddleP1(uint8_t direction)
{
    uint8_t i;
        
    switch (direction)
    {        
        case DIR_TYPE_UP:
        {
            // clear highest pixel
            for (i = 0; i < PADDLE_WIDTH; i++)
                ClearPixel(PADDLE_P1_X_POS + i, g_p1_position + PADDLE_HEIGHT / 2);
            // move up
            g_p1_position--;
            // stop going off screen
            if (g_p1_position <= PADDLE_HEIGHT / 2)
                g_p1_position = PADDLE_HEIGHT / 2;
            // redraw
            DrawPaddleP1(g_p1_position);
        }
        break;

        case DIR_TYPE_DOWN:
        {
            // clear lowest pixel
            for (i = 0; i < PADDLE_WIDTH; i++)
                ClearPixel(PADDLE_P1_X_POS + i, g_p1_position - PADDLE_HEIGHT / 2);
            // move down
            g_p1_position++;
            // stop going off screen
            if (g_p1_position >= LCD_HEIGHT - PADDLE_HEIGHT / 2)
                g_p1_position = LCD_HEIGHT - PADDLE_HEIGHT / 2;
            // redraw
            DrawPaddleP1(g_p1_position);
        }
        break;   
    }
}

void MovePaddleP2(uint8_t direction)
{
    uint8_t i;
        
    switch (direction)
    {
        case DIR_TYPE_UP:
            // clear highest pixel
            for (i = 0; i < PADDLE_WIDTH; i++)
                ClearPixel(PADDLE_P2_X_POS + i, g_p2_position + PADDLE_HEIGHT / 2);
            // move up
            g_p2_position--;
            // stop going off screen
            if (g_p2_position <= PADDLE_HEIGHT / 2)
                g_p2_position = PADDLE_HEIGHT / 2;
            // redraw
            DrawPaddleP2(g_p2_position);
            break;

        case DIR_TYPE_DOWN:
            // clear lowest pixel
            for (i = 0; i < PADDLE_WIDTH; i++)
                ClearPixel(PADDLE_P2_X_POS + i, g_p2_position - PADDLE_HEIGHT / 2);
            // move down
            g_p2_position++;
            // stop going off screen
            if (g_p2_position >= LCD_HEIGHT - PADDLE_HEIGHT / 2)
                g_p2_position = LCD_HEIGHT - PADDLE_HEIGHT / 2;
            // redraw
            DrawPaddleP2(g_p2_position);
            break;
    }
}

void MoveBall(uint8_t x_direction, uint8_t y_direction)
{


	
    DrawBall(g_ball_position_x, g_ball_position_y, true);

    // clear only necessary - test
    /*
    int8_t i, j;
    switch (x_direction)
    {
        case DIR_TYPE_LEFT:
            for (j = -BALL_RADIUS; j <= BALL_RADIUS; j++)
                for (i = 0; i <= BALL_RADIUS; i++)
                    if (i * i + j * j <= BALL_RADIUS * BALL_RADIUS)
                        ClearPixel(g_ball_position_x + i, g_ball_position_y + j);
            break;
        case DIR_TYPE_RIGHT:
            for (j = -BALL_RADIUS; j <= BALL_RADIUS; j++)
                for (i = -BALL_RADIUS; i <= 0; i++)
                    if (i * i + j * j <= BALL_RADIUS * BALL_RADIUS)
                        ClearPixel(g_ball_position_x + i, g_ball_position_y + j);
            break;
    }
    switch (y_direction)
    {
        case DIR_TYPE_UP:
            for (j = 0; j <= BALL_RADIUS; j++)
                for (i = -BALL_RADIUS; i <= BALL_RADIUS; i++)
                    if (i * i + j * j <= BALL_RADIUS * BALL_RADIUS)
                        ClearPixel(g_ball_position_x + i, g_ball_position_y + j);
            break;
        case DIR_TYPE_DOWN:
            for (j = -BALL_RADIUS; j <= 0; j++)
                for (i = -BALL_RADIUS; i <= BALL_RADIUS; i++)
                    if (i * i + j * j <= BALL_RADIUS * BALL_RADIUS)
                        ClearPixel(g_ball_position_x + i, g_ball_position_y + j);
            break;
    }
    */

    // move
    switch (x_direction)
    {
        case DIR_TYPE_LEFT:
            g_ball_position_x--;
            break;
        case DIR_TYPE_RIGHT:
            g_ball_position_x++;
            break;
    }
    switch (y_direction)
    {
        case DIR_TYPE_UP:
            g_ball_position_y--;
            break;
        case DIR_TYPE_DOWN:
            g_ball_position_y++;
            break;
    }

    // redraw
    PongInit();
}

void DynamicsTick()
{
    // bounce off top/bottom walls
    if (g_ball_position_y == 0)
        g_ball_direction_y = DIR_TYPE_DOWN;
    else if (g_ball_position_y == LCD_HEIGHT)
        g_ball_direction_y = DIR_TYPE_UP;

    uint8_t paddle_third = PADDLE_HEIGHT / 3;
    
    // bounce off p1 paddle
    if ((g_ball_direction_x == DIR_TYPE_LEFT) &&

        (g_ball_position_x == PADDLE_P1_X_POS) &&
        (g_ball_position_y >= g_p1_position - PADDLE_HEIGHT / 2) &&
        (g_ball_position_y <= g_p1_position + PADDLE_HEIGHT / 2))
    {
        g_ball_direction_x = DIR_TYPE_RIGHT;

        // handle up/down direction for p1 paddle position
        if ((g_ball_position_y >= g_p1_position - PADDLE_HEIGHT / 2) &&
            (g_ball_position_y <= g_p1_position - PADDLE_HEIGHT / 2 + paddle_third))
            g_ball_direction_y = DIR_TYPE_UP;
        if ((g_ball_position_y >= g_p1_position - PADDLE_HEIGHT / 2 + paddle_third) &&
            (g_ball_position_y <= g_p1_position - PADDLE_HEIGHT / 2 + 2 * paddle_third))
            g_ball_direction_y = DIR_TYPE_STATIONARY;
        if ((g_ball_position_y >= g_p1_position - PADDLE_HEIGHT / 2 + 2 * paddle_third) &&
            (g_ball_position_y <= g_p1_position - PADDLE_HEIGHT / 2 + 3 * paddle_third))
            g_ball_direction_y = DIR_TYPE_DOWN;
    }
    

    // bounce off p2 paddle
    if ((g_ball_direction_x == DIR_TYPE_RIGHT) &&
        (g_ball_position_x == PADDLE_P2_X_POS) &&
        (g_ball_position_y >= g_p2_position - PADDLE_HEIGHT / 2) &&
        (g_ball_position_y <= g_p2_position + PADDLE_HEIGHT / 2))
    {
        g_ball_direction_x = DIR_TYPE_LEFT;

        // handle up/down direction for p2 paddle position
        if ((g_ball_position_y >= g_p2_position - PADDLE_HEIGHT / 2) &&
            (g_ball_position_y <= g_p2_position - PADDLE_HEIGHT / 2 + paddle_third))
            g_ball_direction_y = DIR_TYPE_UP;
        if ((g_ball_position_y >= g_p2_position - PADDLE_HEIGHT / 2 + paddle_third) &&
            (g_ball_position_y <= g_p2_position - PADDLE_HEIGHT / 2 + 2 * paddle_third))
            g_ball_direction_y = DIR_TYPE_STATIONARY;
        if ((g_ball_position_y >= g_p2_position - PADDLE_HEIGHT / 2 + 2 * paddle_third) &&
            (g_ball_position_y <= g_p2_position - PADDLE_HEIGHT / 2 + 3 * paddle_third))
            g_ball_direction_y = DIR_TYPE_DOWN;    
    }

    // scores
    HandleScores();
        
    MoveBall(g_ball_direction_x, g_ball_direction_y);
}

void TestDynamics()
{
    g_ball_direction_x = DIR_TYPE_RIGHT;
    g_ball_direction_y = DIR_TYPE_UP;
    
    MoveBall(g_ball_direction_x, g_ball_direction_y);
}

/* ========================================================================= */

/**
 * The UART interrupt handler.
 */

void UARTIntHandler(void)
{
    uint32_t ui32Status;
    uint8_t value;

    // Get the interrrupt status.
    ui32Status = ROM_UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts.
    ROM_UARTIntClear(UART0_BASE, ui32Status);

    // Loop while there are characters in the receive FIFO.
    while(ROM_UARTCharsAvail(UART0_BASE))
    {
        // Read the next character from the UART and write it back to the UART and LCD.
        value = ROM_UARTCharGetNonBlocking(UART0_BASE);
        ROM_UARTCharPutNonBlocking(UART0_BASE, value);

        //PutcharLCD(value); // test

        /*
			
        switch (value)
        {
            case 'a':
            case 'A':
                MovePaddleP1(DIR_TYPE_UP);
                break;

            case 'z':
            case 'Z':
                MovePaddleP1(DIR_TYPE_DOWN);
                break;

            case ';':
            case ':':
                MovePaddleP2(DIR_TYPE_UP);
                break;

            case '.':
            case '>':
                MovePaddleP2(DIR_TYPE_DOWN);
                break;

            case 0x20:
            case 'r':
                ReleaseBall();
                break;
                                            
        }
        */

        // Work around solution for the above.
        // This approach is not ideal, as the paddles speed is equal to the ball speed
        switch (value)
        {
            case 'a':
            case 'A':
                g_p1_move_dir = DIR_TYPE_UP;
                break;

            case 'z':
            case 'Z':
                g_p1_move_dir = DIR_TYPE_DOWN;
                break;

            case ';':
            case ':':
                g_p2_move_dir = DIR_TYPE_UP;
                break;

            case '.':
            case '>':
                g_p2_move_dir = DIR_TYPE_DOWN;
                break;

            case 0x20:
            case 'r':
                g_is_release = true;
                break;
                                            
        }
			
        // Blink the LED to show a character transfer is occuring.
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);

        // Delay for 1 millisecond.  Each SysCtlDelay is about 3 clocks.
        SysCtlDelay(SysCtlClockGet() / (1000 * 3));

        // Turn off the LED
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

    }
}

/* ========================================================================= */

/**
 * Send a string to the UART.
 */

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    // Loop while there are more characters to send.
    while(ui32Count--)
    {
        // Write the next character to the UART.
        ROM_UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}

/* ========================================================================= */

/**
 * Core
 */

int main(void)
{
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    ROM_FPUEnable();
    ROM_FPULazyStackingEnable();

    // Set the clocking to run directly from the crystal.
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    // Enable the GPIO port that is used for the on-board LED.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Enable the GPIO pins for the LED (PF2).
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);

    // Enable the peripherals used by this example.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Enable processor interrupts.
    ROM_IntMasterEnable();

    // Set GPIO A0 and A1 as UART pins.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure the UART for 115,200, 8-N-1 operation.
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    // Enable the UART interrupt.
    ROM_IntEnable(INT_UART0);
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    // Prompt welcome message
    UARTSend((uint8_t *)"\033[2JWelecome to Pong! ", 16);
    
    InitLCD();
    ClearLCD();
    
    PongInit();

    //TestDrawing(); 
    // TestDynamics();
   
    // Loop forever processing motion ticks
    while (1)
    {
        
        DynamicsTick();

        // start of work around as discussed in the UART read interrupt above
        
        switch (g_p1_move_dir)
        {

            case DIR_TYPE_UP:
                MovePaddleP1(DIR_TYPE_UP);
                g_p1_move_dir = DIR_TYPE_STATIONARY;
                break;

            case DIR_TYPE_DOWN:
                MovePaddleP1(DIR_TYPE_DOWN);
                g_p1_move_dir = DIR_TYPE_STATIONARY;
                break;
        }

        switch (g_p2_move_dir)
        {
            case DIR_TYPE_UP:
                MovePaddleP2(DIR_TYPE_UP);
                g_p2_move_dir = DIR_TYPE_STATIONARY;
                break;

            case DIR_TYPE_DOWN:
                MovePaddleP2(DIR_TYPE_DOWN);
                g_p2_move_dir = DIR_TYPE_STATIONARY;
                break;
        }

        // handle release
        if (g_is_release)
        {

            if (g_is_game_over)
            {
                g_score_p1 = 0x30;
                g_score_p2 = 0x30;
                DrawScores(g_score_p1, g_score_p2);
                ClearLCD();
                PongInit();
                g_is_game_over = false;
            }
            
            switch (g_last_winner)
            {
                case PLAYER_UNKNOWN:
                case PLAYER_1:
                    g_ball_direction_x = DIR_TYPE_LEFT;
                    break;
                case PLAYER_2:
                    g_ball_direction_x = DIR_TYPE_RIGHT;
                    break;
            }
            g_ball_direction_y = DIR_TYPE_STATIONARY;
            g_is_release = false;
        }

        // end of work around as discussed in the UART read interrupt above
        
        // ball speed - 50ms for now - can get faster with better hack removed from MoveBall()
        SysCtlDelay(SysCtlClockGet() / (1000 * 3) * 50);

    }
        
}
