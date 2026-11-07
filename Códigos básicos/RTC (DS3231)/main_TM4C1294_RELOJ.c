#include <stdint.h>
#include <stdbool.h>
#include "utils/uartstdio.h"
#include "driverlib2.h"

/************************************************************
 * TM4C1294 + HW-084 (DS3231)  --  version basica: solo reloj
 *
 *   - Reloj del sistema a 120 MHz.
 *   - UART0 a 115200 baudios (UARTStdioConfig).
 *   - Timer0A genera una interrupcion a 50 Hz.
 *   - SLEEP espera a que se produzca la interrupcion.
 *   - I2C2 hardware: PN5 = SCL, PN4 = SDA (100 kHz).
 *   - Imprime fecha y hora por UART una vez por segundo.
 *
 * Todo el codigo esta en este fichero: no necesita ds3231.h ni
 * ds3231_TM4C1294.c (excluyelos del build si tienes otro main).
 ************************************************************/

#define FREC            50          // Timer0A: 50 Hz (20 ms por interrupcion)
#define DS3231_ADDR     0x68        // direccion I2C de 7 bits
#define PROGRAMAR_RTC   0           // Poner a 0 despues de ajustar la hora una vez

// Fecha y hora iniciales (solo se usan si PROGRAMAR_RTC = 1).
#define INI_HORAS       20
#define INI_MINUTOS     46
#define INI_SEGUNDOS    0
#define INI_DIA_SEMANA  1           // 1..7: correspondencia definida por el usuario
#define INI_DIA         2
#define INI_MES         11
#define INI_ANIO        2026        // 2000..2099

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

/* ------------------------------------------------------------
 * I2C basico
 * ------------------------------------------------------------ */

// Lanza un comando I2C y espera a que termine.
// A 120 MHz el bit BUSY tarda en activarse, por eso se espera ~30 us y el
// flag de "operacion terminada" antes de dar el comando por acabado.
static void I2C_Comando(uint32_t comando)
{
    I2CMasterIntClear(I2C2_BASE);
    I2CMasterControl(I2C2_BASE, comando);
    SysCtlDelay(1200);                              // ~30 us a 120 MHz
    while (!I2CMasterIntStatus(I2C2_BASE, false));  // operacion terminada
    while (I2CMasterBusy(I2C2_BASE));
}

// Escribe 'n' bytes a partir del registro 'reg'.
static void DS3231_Escribir(uint8_t reg, const uint8_t *datos, uint8_t n)
{
    uint8_t i;

    while (I2CMasterBusBusy(I2C2_BASE));
    I2CMasterSlaveAddrSet(I2C2_BASE, DS3231_ADDR, false);
    I2CMasterDataPut(I2C2_BASE, reg);
    I2C_Comando(I2C_MASTER_CMD_BURST_SEND_START);

    for (i = 0; i < n; i++)
    {
        I2CMasterDataPut(I2C2_BASE, datos[i]);
        if (i == (n - 1))
            I2C_Comando(I2C_MASTER_CMD_BURST_SEND_FINISH);
        else
            I2C_Comando(I2C_MASTER_CMD_BURST_SEND_CONT);
    }
}

// Lee 'n' bytes (n >= 2) a partir del registro 'reg'.
static void DS3231_Leer(uint8_t reg, uint8_t *datos, uint8_t n)
{
    uint8_t i;

    while (I2CMasterBusBusy(I2C2_BASE));

    // Seleccionar el registro inicial (sin STOP).
    I2CMasterSlaveAddrSet(I2C2_BASE, DS3231_ADDR, false);
    I2CMasterDataPut(I2C2_BASE, reg);
    I2C_Comando(I2C_MASTER_CMD_BURST_SEND_START);

    // START repetido y lectura de los bytes.
    I2CMasterSlaveAddrSet(I2C2_BASE, DS3231_ADDR, true);
    I2C_Comando(I2C_MASTER_CMD_BURST_RECEIVE_START);
    datos[0] = (uint8_t)I2CMasterDataGet(I2C2_BASE);

    for (i = 1; i < n; i++)
    {
        if (i == (n - 1))
            I2C_Comando(I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
        else
            I2C_Comando(I2C_MASTER_CMD_BURST_RECEIVE_CONT);
        datos[i] = (uint8_t)I2CMasterDataGet(I2C2_BASE);
    }
}

/* ------------------------------------------------------------
 * Reloj DS3231 (formato de 24 horas)
 * ------------------------------------------------------------ */

static uint8_t DecABcd(uint8_t d) { return (uint8_t)(((d / 10) << 4) | (d % 10)); }
static uint8_t BcdADec(uint8_t b) { return (uint8_t)(((b >> 4) * 10) + (b & 0x0F)); }

static void RTC_Programar(uint8_t horas, uint8_t minutos, uint8_t segundos,
                          uint8_t diaSemana, uint8_t dia, uint8_t mes,
                          uint16_t anio)
{
    uint8_t reg[7];

    reg[0] = DecABcd(segundos);
    reg[1] = DecABcd(minutos);
    reg[2] = DecABcd(horas);
    reg[3] = DecABcd(diaSemana);
    reg[4] = DecABcd(dia);
    reg[5] = DecABcd(mes);
    reg[6] = DecABcd((uint8_t)(anio - 2000));

    DS3231_Escribir(0x00, reg, 7);

    // Borrar el flag OSF (registro de estado 0x0F) para indicar hora valida.
    reg[0] = 0x00;
    DS3231_Escribir(0x0F, reg, 1);
}

static void RTC_Leer(uint8_t *horas, uint8_t *minutos, uint8_t *segundos,
                     uint8_t *dia, uint8_t *mes, uint16_t *anio)
{
    uint8_t reg[7];

    DS3231_Leer(0x00, reg, 7);

    *segundos = BcdADec(reg[0] & 0x7F);
    *minutos  = BcdADec(reg[1] & 0x7F);
    *horas    = BcdADec(reg[2] & 0x3F);
    *dia      = BcdADec(reg[4] & 0x3F);
    *mes      = BcdADec(reg[5] & 0x1F);
    *anio     = (uint16_t)(2000 + BcdADec(reg[6]));
}

/* ------------------------------------------------------------
 * main
 * ------------------------------------------------------------ */

int main(void)
{
    uint8_t horas, minutos, segundos, dia, mes;
    uint16_t anio;
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

    // I2C2 hardware: PN5 = I2C2SCL, PN4 = I2C2SDA.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C2));

    GPIOPinConfigure(GPIO_PN5_I2C2SCL);
    GPIOPinConfigure(GPIO_PN4_I2C2SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTN_BASE, GPIO_PIN_5);
    GPIOPinTypeI2C(GPIO_PORTN_BASE, GPIO_PIN_4);
    I2CMasterInitExpClk(I2C2_BASE, (uint32_t)RELOJ, false);   // 100 kHz

    UARTprintf("\r\nTM4C1294 + DS3231 (reloj)\r\n");

#if PROGRAMAR_RTC
    RTC_Programar(INI_HORAS, INI_MINUTOS, INI_SEGUNDOS,
                  INI_DIA_SEMANA, INI_DIA, INI_MES, INI_ANIO);
    UARTprintf("RTC programado.\r\n");
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

            RTC_Leer(&horas, &minutos, &segundos, &dia, &mes, &anio);

            UARTprintf("%02u/%02u/%04u  %02u:%02u:%02u\r\n",
                       (unsigned int)dia, (unsigned int)mes, (unsigned int)anio,
                       (unsigned int)horas, (unsigned int)minutos,
                       (unsigned int)segundos);
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
