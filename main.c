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
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include "buttons.h"
#include "sampling.h"

uint32_t gSystemClock; // [Hz] system clock frequency
volatile uint32_t gTime = 8345; // time in hundredths of a second

uint32_t cpu_load_count(void);
uint32_t count_unloaded = 0;
uint32_t count_loaded = 0;
float cpu_load = 0.0;

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

    // initialize timer 3 in one-shot mode for polled timing
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
    TimerDisable(TIMER3_BASE, TIMER_BOTH);
    TimerConfigure(TIMER3_BASE, TIMER_CFG_ONE_SHOT);
    TimerLoadSet(TIMER3_BASE, TIMER_A, 120000); // 1 sec interval

    count_unloaded = cpu_load_count();

    IntMasterEnable();



    // full-screen rectangle
    tRectangle rectFullScreen = {0, 0, GrContextDpyWidthGet(&sContext)-1, GrContextDpyHeightGet(&sContext)-1};

    uint16_t ADC_local[128];

    int i = 0;
    int divNumber = 1;
    uint16_t triggerDir = 1;
    char div_str[10];
    char load_str[30];
    const char* slope[] = {"DOWN","UP"};
    const float div[] = {0.1,0.2, 0.5, 1};
    int oldTrigger;

    while (true) {
        GrContextForegroundSet(&sContext, ClrBlack);
        GrRectFill(&sContext, &rectFullScreen); // fill screen with black


        count_loaded = cpu_load_count();
        cpu_load = 1.0f - (float)count_loaded/count_unloaded; // compute CPU load


        GrContextForegroundSet(&sContext, ClrBlue); // blue lines
        for(i = -3; i < 4; i++){
            GrLineDrawH(&sContext, 0, LCD_HORIZONTAL_MAX-1,LCD_VERTICAL_MAX/2+i*PIXELS_PER_DIV);
            GrLineDrawV(&sContext, LCD_HORIZONTAL_MAX/2+i*PIXELS_PER_DIV, 0,LCD_VERTICAL_MAX-1);
        }

        char data = 'A';
        while(fifo_get(&data) != 0) {
            switch(data) {
            case 'D':
                divNumber = divNumber > 0 ? divNumber-1 : divNumber;
                break;
            case 'U':
                divNumber = divNumber < 3 ? divNumber+1 : divNumber;
                break;
            case 'T':
                triggerDir = triggerDir == 1 ? 0 : 1;
                break;
            }
        }

        if(divNumber == 3) {
            snprintf(div_str, sizeof(div_str), " 1 V");
        }
        else {
            snprintf(div_str, sizeof(div_str), "%03d mV", (int)(div[divNumber]*1000));
        }


        GrContextForegroundSet(&sContext, ClrWhite); // white text
        GrStringDraw(&sContext, "20 us", -1, 4, 0, /*opaque*/ false);
        GrStringDraw(&sContext, div_str, -1, 45, 0, /*opaque*/ false);
        GrStringDraw(&sContext, slope[triggerDir], -1, 100, 0, false);

        int trigger = getTriggerIndex(triggerDir);
        if(trigger == -1) trigger = oldTrigger;
        else oldTrigger = trigger;
        for(i = -64; i < 64; i++) {
            ADC_local[i+64] = gADCBuffer[trigger+i];
        }
        GrContextForegroundSet(&sContext, ClrYellow); // yellow text
        for(i = 0; i < 127; i++) GrLineDraw(&sContext, i, voltageScale(ADC_local[i], div[divNumber]), i+1, voltageScale(ADC_local[i+1], div[divNumber]));

        snprintf(load_str, sizeof(load_str), "CPU load = %.1f%%", cpu_load*100);
        GrContextForegroundSet(&sContext, ClrWhite); // yellow text
        GrStringDraw(&sContext, load_str, -1, 0, 120, false);


        GrFlush(&sContext); // flush the frame buffer to the LCD
    }
}

uint32_t cpu_load_count(void)
{
    uint32_t i = 0;
    TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER3_BASE, TIMER_A); // start one-shot timer
    while (!(TimerIntStatus(TIMER3_BASE, false) & TIMER_TIMA_TIMEOUT))
        i++;
    return i;
}
