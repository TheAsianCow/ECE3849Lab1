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

    uint16_t trigger_index = 63;
    uint16_t frames = 0;
    uint16_t i;

    // full-screen rectangle
    tRectangle rectFullScreen = {0, 0, GrContextDpyWidthGet(&sContext)-1, GrContextDpyHeightGet(&sContext)-1};

    uint16_t ADC_local[128];

    while (true) {
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &rectFullScreen); // fill screen with black

        if(frames == 3){
            frames = 0;
            trigger_index=63;
        }

        int16_t diff1 = gADCBuffer[trigger_index+1]-2048;
        int16_t diff2 = gADCBuffer[trigger_index-1]-2048;
        while(!((diff1&0x8000)^(diff2&0x8000))) trigger_index++;

        for(i = trigger_index-63; i<trigger_index+64; i++) {
            ADC_local[i] = gADCBuffer[trigger_index];
        }

        frames++;
        GrFlush(&sContext); // flush the frame buffer to the LCD
    }
}
