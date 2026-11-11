#include <stdint.h>
#include <stdbool.h>
#include "utils/uartstdio.h"
#include "driverlib2.h"
#include "touch.h"

/************************************************************
 * TM4C1294 + sensor capacitivo touch  --  ejemplo de uso
 *
 *   - Reloj del sistema a 120 MHz.
 *   - UART0 a 115200 baudios (UARTStdioConfig).
 *   - Timer0A genera una interrupcion a 50 Hz.
 *   - SLEEP espera a que se produzca la interrupcion; despues
 *     se llama a TOUCH_Actualizar() (una vez cada 20 ms), que
 *     filtra rebotes y devuelve los eventos del sensor.
 *   - Muestra por UART cada toque (con su numero), la pulsacion
 *     larga y el tiempo que ha estado tocado al soltar.
 *
 *   Conexion: VCC -> 3V3, GND -> GND, I/O -> PP5 (BoosterPack 2)
 *   (el pin se cambia en touch.h)
 ************************************************************/

#define FREC            50          // Timer0A: 50 Hz (20 ms por interrupcion)
#define MS_TICK         (1000U / FREC)

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
    touch_evento_t ev;
    uint32_t t_ms = 0U;

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

    UARTprintf("\r\nTM4C1294 + sensor touch\r\n");

    // Sensor touch: se le indica cada cuantos ms se actualiza.
    TOUCH_Init(MS_TICK);
    UARTprintf("Toca el sensor (pulsacion larga: %u ms)\r\n",
               (unsigned int)TOUCH_MS_LARGA);

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

    while (1)
    {
        SLEEP;

        ev = TOUCH_Actualizar();

        switch (ev)
        {
        case TOUCH_EV_TOQUE:
            UARTprintf("Toque %u\r\n", (unsigned int)TOUCH_Contador());
            break;

        case TOUCH_EV_LARGO:
            UARTprintf("  Pulsacion larga\r\n");
            break;

        case TOUCH_EV_SOLTADO:
            UARTprintf("  Soltado\r\n");
            break;

        default:
            break;
        }

        // Ejemplo de consulta del tiempo tocado: se guarda el ultimo valor
        // mientras esta tocado y se muestra al soltar.
        if (TOUCH_Tocado())
            t_ms = TOUCH_TiempoTocado_ms();
        if (ev == TOUCH_EV_SOLTADO)
            UARTprintf("  Duracion: %u ms\r\n", (unsigned int)t_ms);
    }
}

// Interrupcion del Timer0A.
void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20);       // Se mantiene el pequeno retardo del ejemplo original.
    Flag_ints++;
}
