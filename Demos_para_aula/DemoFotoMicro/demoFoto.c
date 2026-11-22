/*
 * main.c
 */


#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>


#include "driverlib2.h"
#include "utils/uartstdio.h"

#include "imgs.h"
#include "MrpEscenariosLib.h"


#include "FT800_TIVA.h"

// =======================================================================
// Function Declarations
// =======================================================================


int RELOJ;
volatile int Flag_ints=0;

void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()

#define NUM_SSI_DATA            3


//Cosas pantalla
#define dword long
#define byte char

unsigned long REG_TT[6];
const int32_t REG_CAL[6]= {CAL_DEFAULTS};

int Load;   // Variable para comprobar el % de carga del ciclo de trabajo

//OTROS
#define FREC 50 //Frecuencia en hercios del tren de pulsos: 20ms

char Fin=0;

void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}


#define MICROFONO_ON !GPIOPinRead(GPIO_PORTP_BASE,GPIO_PIN_1)

void dibujarFondo(int id);
void dibujarMrP(int x, int y, char idEmocion, char gafas, char cuerpo, char idAnimacion);
void despiertaMrP();

int main(void) {


    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    //Habilitar perifericos no boosterpack y configurar temporizador
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, RELOJ/FREC -1);
    TimerIntRegister(TIMER0_BASE, TIMER_A,IntTimer);

    //Interrupciones de Timer
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);

    //Perifericos sensores digitales
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    GPIOPinTypeGPIOInput(GPIO_PORTP_BASE, GPIO_PIN_1);   //P1: entrada
    GPIOPadConfigSet(GPIO_PORTP_BASE,GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU); //Pullup en P1

    //UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, RELOJ);


    // PANTALLA
    HAL_Init_SPI(2, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();       //Arranque de la pantalla
    Nueva_pantalla(16,16,16);
    //Nueva pantalla, borde
    ComColor(60, 20, 0);
    ComLineWidth(5);
    ComRect(10, 10, HSIZE-10, VSIZE-10, true);
    //Nueva pantalla, relleno
    ComColor(20, 200, 100);
    ComRect(12, 12, HSIZE-12, VSIZE-12, true);
    //Texto
    ComColor(0,0,0);
    ComTXT(HSIZE/2,VSIZE/5, 30, OPT_CENTERX,"Demo Foto");
    //
    Dibuja();
    Espera_pant();
    int i;
    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);
    Nueva_pantalla(16,16,16);
    Dibuja();


    while(1){
        SLEEP;
        Nueva_pantalla(16,16,16);

        dibujarFondo(1);

        Dibuja_imagen();


        ComNum(20,20,31,0,Load);

        Dibuja();
        Load=100 - (100*TimerValueGet(TIMER0_BASE,TIMER_A))/((RELOJ/FREC)-1);
    }

}



