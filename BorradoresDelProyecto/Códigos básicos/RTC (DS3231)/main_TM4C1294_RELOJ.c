#include <stdint.h>
#include <stdbool.h>
#include "utils/uartstdio.h"
#include "driverlib2.h"
#include "ds3231.h"

/************************************************************
 * TM4C1294 + HW-084 (DS3231)  --  reloj
 *
 *   - Reloj del sistema a 120 MHz.
 *   - UART0 a 115200 baudios (UARTStdioConfig).
 *   - Timer0A genera una interrupcion a 50 Hz.
 *   - SLEEP espera a que se produzca la interrupcion.
 *   - I2C2 hardware (PN5 = SCL, PN4 = SDA): lo configura DS3231_Init().
 *   - Imprime fecha y hora por UART una vez por segundo.
 ************************************************************/

#define FREC            50          // Timer0A: 50 Hz (20 ms por interrupcion)
#define PROGRAMAR_RTC   0           // Poner a 1 para ajustar la hora; despues volver a 0

// Fecha y hora iniciales (solo se usan si PROGRAMAR_RTC = 1).
#define INI_HORAS       20
#define INI_MINUTOS     46
#define INI_SEGUNDOS    0
#define INI_DIA_SEMANA  1           // 1..7: correspondencia definida por el usuario
#define INI_DIA         2
#define INI_MES         11
#define INI_ANIO        2026        // 2000..2199

// Variable que guarda la frecuencia del programa.
int RELOJ;

// Flag utilizado por SLEEP para esperar a la siguiente interrupcion.
volatile int Flag_ints = 0;

// Interrupcion falsa: espera activa hasta que el Timer0A avise.
void SysCtlSleepFake(void)
{
    while (!Flag_ints);
    Flag_ints = 0;
}

//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()

void IntTimer(void);

int main(void)
{
    DS3231_DateTime rtc;
    uint8_t cuenta = 0;

    // Configuracion del reloj a 120 MHz.
    RELOJ = SysCtlClockFreqSet(
        (SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
         SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
        120000000U);

    // UART0: se conserva la configuracion original.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, RELOJ);

    // I2C2 hardware y pines PN5/PN4 (100 kHz).
    DS3231_Init((uint32_t)RELOJ);

    UARTprintf("\r\nTM4C1294 + DS3231 (reloj)\r\n");

#if PROGRAMAR_RTC
    rtc.hours     = INI_HORAS;
    rtc.minutes   = INI_MINUTOS;
    rtc.seconds   = INI_SEGUNDOS;
    rtc.dayOfWeek = INI_DIA_SEMANA;
    rtc.day       = INI_DIA;
    rtc.month     = INI_MES;
    rtc.year      = INI_ANIO;

    if (DS3231_WriteDateTime(&rtc) && DS3231_ClearOscillatorStopFlag())
    {
        UARTprintf("RTC programado.\r\n");
    }
    else
    {
        UARTprintf("ERROR: no se pudo programar el DS3231. Codigo=%u\r\n",
                   (unsigned int)DS3231_GetLastError());
    }
#endif

    // Configuracion Timer0A e interrupcion: se conserva la original.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ / FREC) - 1U);
    TimerIntRegister(TIMER0_BASE, TIMER_A, IntTimer);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);

    // Configuracion de modos de bajo consumo: se conserva la original.
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_I2C2);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralClockGating(true);

    while (1)
    {
        SLEEP;

        // Leer el RTC cada 50 interrupciones (aprox. una vez por segundo).
        cuenta++;
        if (cuenta >= FREC)
        {
            cuenta = 0;

            if (DS3231_ReadDateTime(&rtc))
            {
                UARTprintf("%02u/%02u/%04u  %02u:%02u:%02u\r\n",
                           (unsigned int)rtc.day,
                           (unsigned int)rtc.month,
                           (unsigned int)rtc.year,
                           (unsigned int)rtc.hours,
                           (unsigned int)rtc.minutes,
                           (unsigned int)rtc.seconds);
            }
            else
            {
                UARTprintf("ERROR: lectura DS3231. Codigo=%u\r\n",
                           (unsigned int)DS3231_GetLastError());
            }
        }
    }
}

// Interrupcion del Timer0A.
void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20);       // Se mantiene el pequeno retardo del ejemplo original.
    Flag_ints++;
}
