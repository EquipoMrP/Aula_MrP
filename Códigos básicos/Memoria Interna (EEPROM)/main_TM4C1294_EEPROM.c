#include <stdint.h>
#include <stdbool.h>
#include "utils/uartstdio.h"
#include "driverlib2.h"
#include "eeprom_cad.h"

/************************************************************
 * TM4C1294 + EEPROM interna  --  ejemplo de uso de la biblioteca
 *
 *   - Reloj del sistema a 120 MHz.
 *   - UART0 a 115200 baudios (UARTStdioConfig).
 *   - Timer0A genera una interrupcion a 50 Hz.
 *   - SLEEP espera a que se produzca la interrupcion.
 *   - Muestra por UART las cadenas guardadas en la EEPROM,
 *     pide una nueva y la guarda. Se conservan las ultimas
 *     EECAD_MAX_CADENAS; la nueva sustituye a la mas antigua.
 *     Enter sin texto = no guardar nada.
 *   - Las cadenas se conservan aunque se apague la placa.
 ************************************************************/

#define FREC            50          // Timer0A: 50 Hz (20 ms por interrupcion)

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

int main(void){
    char cadena[EECAD_MAX_LEN + 1U];
    uint32_t num;
    uint32_t i;
    int32_t n;

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

    UARTprintf("\r\nTM4C1294 + EEPROM (cadenas)\r\n");

    // EEPROM interna.
    if (!EECAD_Init())
    {
        UARTprintf("Error al iniciar la EEPROM\r\n");
        while (1);
    }
    UARTprintf("EEPROM: %u bytes, %u cadenas de hasta %u caracteres\r\n",
               (unsigned int)EECAD_TamBytes(),
               (unsigned int)EECAD_MAX_CADENAS,
               (unsigned int)EECAD_MAX_LEN);

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

        // Se muestran las cadenas guardadas (de la mas antigua a la mas reciente).
        num = EECAD_NumCadenas();
        if (num == 0U)
        {
            UARTprintf("\r\nNo hay ninguna cadena guardada\r\n");
        }
        else
        {
            UARTprintf("\r\nCadenas guardadas (%u de %u, la ultima es la mas reciente):\r\n",
                       (unsigned int)num, (unsigned int)EECAD_MAX_CADENAS);
            for (i = 0U; i < num; i++)
            {
                n = EECAD_Leer(i, cadena, sizeof(cadena));
                if (n >= 0)
                    UARTprintf("  %u: %s\r\n", (unsigned int)(i + 1U), cadena);
                else
                    UARTprintf("  %u: (no valida)\r\n", (unsigned int)(i + 1U));
            }
        }

        // Se pide la nueva cadena (bloquea hasta pulsar Enter).
        UARTprintf("Nueva cadena (Enter sin texto = no guardar): ");
        UARTgets(cadena, sizeof(cadena));

        if (cadena[0] == '\0')
        {
            UARTprintf("\r\nSin cambios\r\n");
            continue;
        }

        // La nueva se anade; si esta lleno, sustituye a la mas antigua.
        if (EECAD_Guardar(cadena))
            UARTprintf("\r\nGuardada\r\n");
        else
            UARTprintf("\r\nError al guardar la cadena\r\n");
    }
}

// Interrupcion del Timer0A.
void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20);       // Se mantiene el pequeno retardo del ejemplo original.
    Flag_ints++;
}
