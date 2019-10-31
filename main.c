/**
 * main.c
 *
 * ECE 3849 Lab 1
 * Jeffrey Huang
 * Ravi Kirschner 10/29/2019
 *
 * This version is using the new hardware for B2017: the EK-TM4C1294XL LaunchPad with BOOSTXL-EDUMKII BoosterPack.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include "buttons.h"
#include "sampling.h"

uint32_t gSystemClock; // [Hz] system clock frequency
volatile uint32_t gTime = 8345; // time in hundredths of a second

int main(void)
{
    IntMasterDisable();

    // Enable the Floating Point Unit, and permit ISRs to use it
    FPUEnable();
    FPULazyStackingEnable();

    // Initialize the system clock to 120 MHz
    gSystemClock = SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 120000000);

    Crystalfontz128x128_Init(); // Initialize the LCD display driver
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP); // set screen orientation

    tContext sContext;
    GrContextInit(&sContext, &g_sCrystalfontz128x128); // Initialize the grlib graphics context
    GrContextFontSet(&sContext, &g_sFontFixed6x8); // select font

    ADC1_Init();
    ButtonInit();
    IntMasterEnable();



    // full-screen rectangle
    tRectangle rectFullScreen = {0, 0, GrContextDpyWidthGet(&sContext)-1, GrContextDpyHeightGet(&sContext)-1};

    uint16_t ADC_local[128];

    int i = 0;
    uint16_t triggerDir = 0;
    float div = 0.2;
    char div_str[10];
    char* slope[] = {"DOWN","UP"};

    while (true) {
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &rectFullScreen); // fill screen with black

        GrContextForegroundSet(&sContext, ClrBlue); // blue lines
        for(i = -3; i < 4; i++){
            GrLineDrawH(&sContext, 0, LCD_HORIZONTAL_MAX-1,LCD_VERTICAL_MAX/2+i*PIXELS_PER_DIV);
            GrLineDrawV(&sContext, LCD_HORIZONTAL_MAX/2+i*PIXELS_PER_DIV, 0,LCD_VERTICAL_MAX-1);
        }
        snprintf(div_str, sizeof(div_str), "%03d mV", (int)(div*1000));
        GrContextForegroundSet(&sContext, ClrWhite); // white text
        GrStringDraw(&sContext, "20 us", -1, 4, 0, /*opaque*/ false);
        GrStringDraw(&sContext, div_str, -1, 45, 0, /*opaque*/ false);
        GrStringDraw(&sContext, slope[triggerDir], -1, 100, 0, false);

        uint16_t trigger = getTriggerIndex(triggerDir);
        for(i = -63; i < 64; i++) {
            ADC_local[i+63] = gADCBuffer[trigger+i];
        }

        GrContextForegroundSet(&sContext, ClrYellow); // yellow text
        for(i = 0; i < 128; i++) GrCircleFill(&sContext, i, voltageScale(ADC_local[i], div),1);
        GrFlush(&sContext); // flush the frame buffer to the LCD
    }
}
