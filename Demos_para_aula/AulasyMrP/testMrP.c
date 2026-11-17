/*
 * main.c
 */


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>


#include "driverlib2.h"
#include "utils/uartstdio.h"

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


void dibujarFondo(int id);
void dibujarMrP(int x, int y);

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
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, RELOJ);

    //Asignar valores a los struct (no se puede hacer fuera)
        //Limpiamos todos los struct por si acaso
    int i;

    //UARTprintf("\nComenzamos pantalla\n");
    // PANTALLA
    HAL_Init_SPI(2, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();       //Arranque de la pantalla


    //UARTprintf("\nComenzamos pantalla2\n");
    Nueva_pantalla(16,16,16);
    //Nueva pantalla, borde
    ComColor(20, 20, 0);
    ComLineWidth(5);
    ComRect(10, 10, HSIZE-10, VSIZE-10, true);
    //Nueva pantalla, relleno
    ComColor(200, 200, 200);
    ComRect(12, 12, HSIZE-12, VSIZE-12, true);

    //Texto
    ComColor(0,0,0);
    ComTXT(HSIZE/2,VSIZE/5, 30, OPT_CENTERX,"Mr. P");


    //UARTprintf("\nAntepantalla1\n"); //DEBUG
    Dibuja();
    //UARTprintf("\nAntepantalla\n"); //DEBUG
    Espera_pant();
    //UARTprintf("\nTraspantalla\n"); //DEBUG
    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);
    Nueva_pantalla(16,16,16);
    Dibuja();

    //Declariacion de variables locales
    // - BUCLES
    float c=0;

    while(1){
        SLEEP;
        Nueva_pantalla(16,16,16);
        ComColor(255,255,255);

        c++;
        if(c>40*M_PI){
            c=0;
        }

        dibujarFondo(0);
        dibujarMrP(HSIZE/2+10*(cos(c/20.)),50+10*(sin(c/20.)));

        Dibuja();
        Load=100 - (100*TimerValueGet(TIMER0_BASE,TIMER_A))/((RELOJ/FREC)-1);
    }

}

void dibujarFondo(int id){

    switch(id){
    case 0: //LECCION TEORIA
        //Fondo
        ComColor(245, 228, 156); //Caqui
        ComLineWidth(5);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);

        //Borde pizarra
        ComColor(156, 90, 60); //Marron
        ComRect(17, 17, HSIZE-17, VSIZE-17, true);

        //Relleno pizarra
        ComColor(13, 77, 53); //Verde oscuro
        ComRect(23, 23, HSIZE-23, VSIZE-23, true);

        ComLineWidth(1);
        //Borrador
        ComColor(70, 70, 70); //Gris oscuro
        ComRect(HSIZE-43, VSIZE-23, HSIZE-63, VSIZE-26, true);
        ComColor(79, 41, 25); //Marron oscuro
        ComRect(HSIZE-43, VSIZE-28, HSIZE-63, VSIZE-33, true);

        //Tizas
        ComColor(255,255,255);
        ComRect(HSIZE-79, VSIZE-24, HSIZE-89, VSIZE-25, true);
        ComRect(HSIZE-104, VSIZE-24, HSIZE-108, VSIZE-25, true);

        //Retoques
        ComColor(13, 77, 53); //Verde oscuro
        ComRect(HSIZE-76, VSIZE-28, HSIZE-111, VSIZE-30, true);

        break;

    case 1://LECCION EJEMPLO
        //Fondo
        ComColor(245, 228, 156); //Caqui
        ComLineWidth(5);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);

        //Borde pizarra
        ComColor(0, 0, 0); //Negro
        ComRect(17, 17, HSIZE-17, 33, true);
        ComRect(23, 23, HSIZE-23, VSIZE-23, true);


        //Borde abajo pizarra
        ComRect(20, VSIZE-26, HSIZE-20, VSIZE-18, true);

        //Relleno pizarra
        ComColor(255, 255, 255); //Blanco
        ComRect(25, 23, HSIZE-25, VSIZE-23, true);



        break;

    default:
        ComColor(245, 228, 156); //Caqui
        ComLineWidth(5);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);
        break;


    }
}


void dibujarMrP(int x, int y){
    int i=0;

    //Dibujamos el pelo pt 1
    ComColor(61, 43, 26); //Color marron
    ComLine(x-25,y+30,x+25,y+30,30);

    //Dibujamos la piel de la cara
    ComColor(255,181,144);//Color piel
    ComCirculo(x,y+60,43);
    ComCirculo(x,y+85,43);
    ComRect(x-43,y+60,x+43,y+80,true);

    //Dibujamos el pelo pt 2
    ComColor(61, 43, 26); //Color marron
    ComLine(x-25,y+30,x+25,y+30,15);
    ComColor(107, 91, 57); //Color marron claro
    ComLine(x-5,y+20,x+5,y+20,7);


    /// Dibujamos expresion. Funcion?
        //1. Dibjamos
            //1.1 Gafas
    ComColor(0,0,80); //Negro
    ComRect(x-45, y+60, x-10, y+80, true);
    ComRect(x+45, y+60, x+10, y+80, true);
    ComLine(x-10,y+70,x+10,y+70,3);
            //1.2 Boca
    ComColor(0,0,0); //Negro
    ComCirculo(x,y+97,13);
        //2. Borramos
    ComColor(255,181,144);//Color piel
            //2.1 Gafas
    ComRect(x-42, y+63, x-13, y+77, true);
    ComRect(x+42, y+63, x+13, y+77, true);
            //2.2 Boca
    ComRect(x-15,y+92,x+15,y+97,true);
        //3. Volvemos a dibujar
            //3.1 Ojos
        ComColor(0,0,0); //Negro
        ComLine(x-27,y+68,x-27,y+73,4);
        ComLine(x+27,y+68,x+27,y+73,4);

    //Dibujamos la "camisa"
    ComColor(150,150,150); //Gris
    for(i=-2;i<3;i++){
        ComLine(x,y+175,x+(i*15),y+300,30);
    }

    // Dibujamos manos Como pos?

    ComCirculo(x+80, y+225, 20); //Izquierda
    ComCirculo(x-80, y+225, 20); //Derecha

}

