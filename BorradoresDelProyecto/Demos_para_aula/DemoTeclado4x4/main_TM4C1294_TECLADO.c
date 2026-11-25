#include <stdint.h>
#include <stdbool.h>
#include "utils/uartstdio.h"
#include "driverlib2.h"
#include "teclado.h"

/************************************************************
 * TM4C1294 + teclado matricial 4x4  --  ejemplo de uso
 *
 *   - Reloj del sistema a 120 MHz.
 *   - UART0 a 115200 baudios (UARTStdioConfig).
 *   - Timer0A genera una interrupcion a 50 Hz.
 *   - SLEEP espera a que se produzca la interrupcion; despues
 *     se llama a TECLADO_Actualizar() (una vez cada 20 ms), que
 *     barre la matriz, filtra rebotes y devuelve los eventos.
 *   - Muestra por UART cada tecla pulsada, la pulsacion larga y
 *     el tiempo que ha estado pulsada al soltar.
 *   - Ademas guarda las teclas numericas en un buffer:
 *       '*' borra el buffer,  '#' muestra el codigo escrito.
 *
 *   Conexion (BoosterPack 2):
 *     Filas    1..4 -> PK0, PK1, PK2, PK3   (B2 pines 5..8)
 *     Columnas 1..4 -> PK4, PK5, PK6, PK7   (C2 pines 2, 3, 9, 10)
 *   (los pines se cambian en teclado.h)
 ************************************************************/

#define FREC            50          // Timer0A: 50 Hz (20 ms por interrupcion)
#define MS_TICK         (1000U / FREC)
#define TAM_CODIGO      16U         // maximo de teclas guardadas

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
    teclado_evento_t ev;
    char tecla;
    char codigo[TAM_CODIGO + 1U];
    uint32_t n = 0U;

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

    UARTprintf("\r\nTM4C1294 + teclado 4x4\r\n");

    // Teclado: se le indica cada cuantos ms se actualiza.
    TECLADO_Init(MS_TICK);
    UARTprintf("Pulsa teclas (* borra, # muestra el codigo, larga: %u ms)\r\n",
               (unsigned int)TECLADO_MS_LARGA);

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

        ev = TECLADO_Actualizar();

        switch (ev)
        {
        case TECLADO_EV_PULSADA:
            tecla = TECLADO_Tecla();
            UARTprintf("Tecla %c\r\n", tecla);

            if (tecla == '*')
            {
                n = 0U;
                UARTprintf("  Codigo borrado\r\n");
            }
            else if (tecla == '#')
            {
                codigo[n] = '\0';
                UARTprintf("  Codigo: %s\r\n", codigo);
                n = 0U;
            }
            else if (n < TAM_CODIGO)
            {
                codigo[n++] = tecla;
            }
            break;

        case TECLADO_EV_LARGA:
            UARTprintf("  Pulsacion larga de %c\r\n", TECLADO_UltimaTecla());
            break;

        case TECLADO_EV_SOLTADA:
            UARTprintf("  Soltada %c: %u ms\r\n", TECLADO_UltimaTecla(),
                       (unsigned int)TECLADO_TiempoPulsada_ms());
            break;

        default:
            break;
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
