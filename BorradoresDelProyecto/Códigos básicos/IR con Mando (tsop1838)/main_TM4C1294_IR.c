/*
 * main_TM4C1294_IR.c
 *
 * Prueba del sensor infrarrojos TSOP1838 con mando (protocolo NEC) en el TM4C1294.
 *
 *  - Una barra se mueve de derecha a izquierda en la pantalla FT800 (BoosterPack 2).
 *    Si el programa se ralentizara, la barra iria a tirones o mas despacio.
 *  - La tecla pulsada del mando se envia por UART0 y se muestra por pantalla.
 *  - El bucle principal se sincroniza con el Timer0 (50 Hz) mediante SLEEP,
 *    igual que en Parte2.c. El sensor se atiende por interrupcion (tsop1838.c).
 *
 * Conexion del sensor: VCC -> 3.3V, GND -> GND, OUT -> PC6
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include "tsop1838.h"

// V. Reloj
int RELOJ;      // Variable que guarda la frecuencia del programa
#define FREC 50 // Frecuencia en hercios del Timer0 (refresco de pantalla): 20 ms

// V. Barra
#define PISTA_X0    20
#define PISTA_X1    (HSIZE-20)
#define PISTA_Y0    (VSIZE-60)
#define PISTA_Y1    (VSIZE-30)
#define MARGEN      3
#define BARRA_W     40      // Ancho de la barra (pixeles)
#define BARRA_PASO  4       // Pixeles que avanza en cada refresco

int Load;   // Comprobar el % de carga del ciclo de trabajo

// Interrupcion falsa
volatile int Flag_ints=0;
void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

// Con SysCtlSleep() real, cualquier interrupcion despierta al micro (tambien los flancos
// del sensor IR), asi que se vuelve a dormir hasta que sea el Timer0 quien despierte.
void SleepReal(void)
{
    while(!Flag_ints) SysCtlSleep();
    Flag_ints=0;
}

//#define SLEEP SleepReal()
#define SLEEP SysCtlSleepFake()
void IntTimer(void);

int main(void)
{
    // Configuracion del reloj
    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    // UART (primero se habilitan UART0 y GPIOA, despues UARTStdioConfig)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, RELOJ);

    // Configuracion Timer e interrupcion
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/FREC)-1);
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer);
    IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
    IntMasterEnable();  //Habilitacion global de interrupciones
    TimerEnable(TIMER0_BASE, TIMER_A);

    // Sensor IR (PC6 + Timer1). Deja GPIOC y Timer1 habilitados en SLEEP
    IR_Init(RELOJ);

    // Configuracion modos bajo consumo
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralClockGating(true);

    // Pantalla
    SysCtlDelay(RELOJ/3);
    HAL_Init_SPI(2, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();       //Arranque de la pantalla

    UARTprintf("Test mando IR. Pulsa una tecla del mando\n");

    // Variables del programa
    IR_Trama trama;
    bool hay_tecla = false;     // Ya se ha recibido alguna tecla
    uint8_t cmd = 0, dir = 0;   // Ultima tecla recibida
    int barra_x = PISTA_X1 - MARGEN - BARRA_W;
    char cadena[32];

    Load = 0;

    while(1){
        SLEEP;

        // Sensor IR: tecla nueva?
        if(IR_Lee(&trama)){
            cmd = trama.comando;
            dir = trama.direccion;
            hay_tecla = true;

            // Si se mantiene pulsada solo se envia por UART la primera vez
            if(!trama.repeticion){
                UARTprintf(">> Tecla: cmd=0x%02x (%d), dir=0x%02x", cmd, cmd, dir);
                if(IR_NombreTecla(cmd)) UARTprintf(" -> %s", IR_NombreTecla(cmd));
                UARTprintf("\n");
            }
        }

        // Movimiento de la barra: derecha -> izquierda
        barra_x -= BARRA_PASO;
        if(barra_x <= PISTA_X0 + MARGEN){
            barra_x = PISTA_X1 - MARGEN - BARRA_W;
        }

        Nueva_pantalla(16,16,16);

        // Fondo
        ComColor(254, 246, 230);
        ComRect(5, 5, HSIZE-5, VSIZE-5, true);

        // Titulo
        ComColor(0, 0, 0);
        ComTXT(HSIZE/2, 12, 30, OPT_CENTERX, "TEST MANDO IR");
        ComTXT(HSIZE/2, 62, 27, OPT_CENTERX, "Tecla pulsada:");

        // Tecla: el nombre si se conoce; si no, el codigo
        if(!hay_tecla){
            sprintf(cadena, "---");
        }else if(IR_NombreTecla(cmd)){
            sprintf(cadena, "%s", IR_NombreTecla(cmd));
        }else{
            sprintf(cadena, "0x%02x", cmd);
        }
        ComColor(30, 103, 250);
        ComTXT(HSIZE/2, 88, 31, OPT_CENTERX, cadena);

        if(hay_tecla){
            ComColor(0, 0, 0);
            sprintf(cadena, "cmd %d (0x%02x)  dir 0x%02x", cmd, cmd, dir);
            ComTXT(HSIZE/2, 145, 27, OPT_CENTERX, cadena);
        }

        // Carga del programa (del refresco anterior)
        ComColor(0, 0, 0);
        sprintf(cadena, "Carga: %d%%", Load);
        ComTXT(HSIZE/2, 175, 26, OPT_CENTERX, cadena);

        // Pista y barra
        ComColor(0, 0, 0);
        ComRect(PISTA_X0, PISTA_Y0, PISTA_X1, PISTA_Y1, true);
        ComColor(255, 170, 0);
        ComRect(barra_x, PISTA_Y0 + MARGEN, barra_x + BARRA_W, PISTA_Y1 - MARGEN, true);

        Dibuja();

        // Prueba saturacion programa
        Load=100 - (100*TimerValueGet(TIMER0_BASE,TIMER_A))/((RELOJ/FREC)-1);
    }
}

// Interrupcion
void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}
