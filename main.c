#include <stdlib.h>
#include <stdio.h>
#include "System/system.h"
#include "System/delay.h"
#include "oledDriver/oledC.h"
#include "oledDriver/oledC_colors.h"
#include "oledDriver/oledC_shapes.h"

typedef unsigned char DISPLAY_MODE;
#define MODE_NORMAL 0
#define MODE_INVERSE !MODE_NORMAL

static uint16_t current_background_color;

// Forward Declarations
void UpdateDisplayCounter(int *new_counter);
void UpdatePotentiometerValue(int *potentiometer_value, int *previous_potentiometer_value);
void ToggleDisplayMode(int *counter, int *potentiometer_value, int *previous_potentiometer_value, DISPLAY_MODE *current_mode);

void BlueLed()
{
    RPOR11bits.RP23R = 15;
    OC3RS = 1023;
    OC3CON2bits.SYNCSEL = 0x1F; // self-sync
    OC3CON2bits.OCTRIG = 0;     // sync mode
    OC3CON1bits.OCTSEL = 0b111; // FOSC/2
    OC3CON1bits.OCM = 0b110;    // edge aligned
    OC3CON2bits.TRIGSTAT = 1;   // Fixed stray '\240'
}

void RedLed()
{
    RPOR13 = 13;
    OC1RS = 1023;
    OC1CON2bits.SYNCSEL = 0x1F; // self-sync
    OC1CON2bits.OCTRIG = 0;     // sync mode
    OC1CON1bits.OCTSEL = 0b111; // FOSC/2
    OC1CON1bits.OCM = 0b110;    // edge aligned
    OC1CON2bits.TRIGSTAT = 1;
}

void GreenLed()
{
    RPOR13bits.RP27R = 14;
    OC2RS = 1023;
    OC2CON2bits.SYNCSEL = 0x1F; // self-sync
    OC2CON2bits.OCTRIG = 0;     // sync mode
    OC2CON1bits.OCTSEL = 0b111; // FOSC/2
    OC2CON1bits.OCM = 0b110;    // edge aligned
    OC2CON2bits.TRIGSTAT = 1;   // Fixed stray '\240'
}

void InitializeUserHardware(void)
{
    TRISBbits.TRISB12 = 1; // Potentiometer input
    ANSBbits.ANSB12 = 1;   // Analog input enabled

    // Configure buttons
    TRISAbits.TRISA11 = 1; // S1 button
    TRISAbits.TRISA12 = 1; // S2 button

    // Configure LEDs
    TRISAbits.TRISA8 = 0; // LED1 output
    TRISAbits.TRISA9 = 0; // LED2 output

    // ADC setup for potentiometer
    AD1CON1bits.SSRC = 0;
    AD1CON1bits.FORM = 0;
    AD1CON1bits.ASAM = 0;
    AD1CON1bits.MODE12 = 0;
    AD1CON2 = 0x00;
    AD1CON3bits.ADCS = 0xFF;
    AD1CON3bits.SAMC = 0x10;
    AD1CHSbits.CH0SA = 8;
    AD1CON1bits.ADON = 1;
}

static void oledC_clearScreen(void)
{
    uint8_t column, row;

    oledC_setColumnAddressBounds(0, 96);
    oledC_setRowAddressBounds(0, 96);

    for (column = 0; column < 96; column++)
    {
        for (row = 0; row < 96; row++)
        {
            oledC_sendColorInt(current_background_color);
        }
    }
}

static void SetOLEDBackground(uint16_t color)
{
    if (current_background_color != color)
    {
        current_background_color = color;
        oledC_clearScreen();
    }
}

void ToggleDisplayMode(int *counter, int *potentiometer_value, int *previous_potentiometer_value, DISPLAY_MODE *current_mode)
{
    if (*current_mode == MODE_NORMAL)
    {
        oledC_sendCommand(OLEDC_CMD_SET_DISPLAY_MODE_ON, NULL, 0);
    }
    else
    {
        oledC_sendCommand(OLEDC_CMD_SET_DISPLAY_MODE_INVERSE, NULL, 0);
    }

    *current_mode = !(*current_mode);

    UpdateDisplayCounter(counter);
    UpdatePotentiometerValue(potentiometer_value, previous_potentiometer_value);
}

void UpdateDisplayCounter(int *new_counter)
{
    static int last_counter = -1;
    char text_buffer[10];

    if (last_counter != *new_counter)
    {
        sprintf(text_buffer, "S:%3d", last_counter);
        oledC_DrawString(10, 60, 2, 2, (uint8_t *)text_buffer, current_background_color);

        sprintf(text_buffer, "S:%3d", *new_counter);
        oledC_DrawString(10, 60, 2, 2, (uint8_t *)text_buffer, OLEDC_COLOR_ROYALBLUE);

        last_counter = *new_counter;
    }
}

void UpdatePotentiometerValue(int *potentiometer_value, int *previous_potentiometer_value)
{
    static int last_potentiometer = -1;
    char text_buffer[10];

    if (last_potentiometer != *potentiometer_value)
    {
        sprintf(text_buffer, "P:%3d", last_potentiometer);
        oledC_DrawString(10, 10, 2, 2, (uint8_t *)text_buffer, current_background_color);

        sprintf(text_buffer, "P:%3d", *potentiometer_value);
        oledC_DrawString(10, 10, 2, 2, (uint8_t *)text_buffer, OLEDC_COLOR_TURQUOISE);

        last_potentiometer = *potentiometer_value;
    }
}

int main(void)
{
    int display_counter = 0, potentiometer = 0;
    int previous_potentiometer = -1;
    DISPLAY_MODE current_mode = MODE_INVERSE;

    SYSTEM_Initialize();
    InitializeUserHardware();

    SetOLEDBackground(OLEDC_COLOR_AZURE);

    UpdateDisplayCounter(&display_counter);
    UpdatePotentiometerValue(&potentiometer, &previous_potentiometer);

    while (1)
    {
        if (PORTAbits.RA11 == 0)
        {
            LATAbits.LATA8 = 1;
        }
        else if (LATAbits.LATA8 == 1)
        {
            LATAbits.LATA8 = 0;
            display_counter++;
            UpdateDisplayCounter(&display_counter);
        }

        if (PORTAbits.RA12 == 0)
        {
            LATAbits.LATA9 = 1;
        }
        else if (LATAbits.LATA9 == 1)
        {
            LATAbits.LATA9 = 0;
            ToggleDisplayMode(&display_counter, &potentiometer, &previous_potentiometer, &current_mode);
        }

        AD1CON1bits.SAMP = 1;
        for (int i = 0; i < 1000; i++)
            ;
        AD1CON1bits.SAMP = 0;

        while (!AD1CON1bits.DONE)
            ;
        potentiometer = ADC1BUF0;

        if (abs(potentiometer - previous_potentiometer) > 8)
        {
            UpdatePotentiometerValue(&potentiometer, &previous_potentiometer);
            previous_potentiometer = potentiometer;
        }
    }

    return 1;
}
