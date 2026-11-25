#include <stdint.h>
#include <stdbool.h>
#include "utils/uartstdio.h"
#include "driverlib2.h"
#include "hcsr04.h"

/************************************************************
 * TM4C1294 + HC-SR04  --  ejemplo de uso de la biblioteca
 *
 *   - Reloj del sistema a 120 MHz.
 *   - UART0 a 115200 baudios (UARTStdioConfig).
 *   - Timer0A genera una interrupcion a 50 Hz.
 *   - SLEEP espera a que se produzca la interrupcion.
 *   - HC-SR04: TRIG = PM0, ECHO = PM1 (Timer1 mide el eco).
 *   - Lanza una medida cada 60 ms y imprime cada 0,5 s.
 ************************************************************/

#define FREC            50          // Timer0A: 50 Hz (20 ms por interrupcion)
#define TICKS_MEDIDA    3           // 3 x 20 ms = 60 ms entre medidas
#define TICKS_PRINT     25          // 25 x 20 ms = 0,5 s entre impresiones

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
    uint8_t cuenta_medida = 0;
    uint8_t cuenta_print = 0;
    uint32_t mm = 0;
    bool valida = false;

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

    UARTprintf("\r\nTM4C1294 + HC-SR04 (distancia)\r\n");

    // Sensor de ultrasonidos.
    HCSR04_Init((uint32_t)RELOJ);

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
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralClockGating(true);

    HCSR04_Disparar();      // primera medida

    while (1)
    {
        SLEEP;

        // Cada 60 ms: recoger la medida anterior y lanzar otra.
        cuenta_medida++;
        if (cuenta_medida >= TICKS_MEDIDA)
        {
            cuenta_medida = 0;

            valida = HCSR04_Listo();
            if (valida)
                mm = HCSR04_LeerMM();

            HCSR04_Disparar();
        }

        // Cada 0,5 s: imprimir la ultima medida.
        cuenta_print++;
        if (cuenta_print >= TICKS_PRINT)
        {
            cuenta_print = 0;

            if (valida)
                UARTprintf("Distancia: %u.%u cm\r\n",
                           (unsigned int)(mm / 10), (unsigned int)(mm % 10));
            else
                UARTprintf("Sin eco / fuera de rango (fallos: %u)\r\n",
                           (unsigned int)HCSR04_Fallos());
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
