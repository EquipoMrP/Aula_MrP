#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"

/***********************************
 * Demo con botoncitos y sonidos para la pantalla
 *************************************/

// V.Pantalla
const int32_t REG_CAL[6]= {CAL_DEFAULTS}; // Calibración
#define TAG(x)       ((3UL << 24) | ((x) & 0xFF)) // Detección widget tocado

// Etiqueta del slider
#define TAG_SLIDER 34

// V.Reloj
int RELOJ; // Variable que guarda la frecuencia del programa
#define FREC 50 //Frecuencia en hercios del tren de pulsos: 20ms

// V. Programa
volatile char Cambia=0;
char cadena[10];
volatile int seleccion = 127;
int contador_paso = 0;
int estado_color = 0;


// Interrupción falsa
volatile int Flag_ints=0;
volatile int cont_int=0;
void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()
void IntTimer(void);


int MiniJuego1(void) {
    int estado = 0;
    //Fondo
    ComColor(245, 228, 156); //Caqui
    ComLineWidth(5);
    ComRect(10, 10, HSIZE-10, VSIZE-10, true);

    ComColor(0, 0, 0);
    ComTXT(HSIZE/2,VSIZE/5-10, 29, OPT_CENTERX,"MiniJuego1");

    ComFgcolor(0, 255, 0);
    if(Boton(HSIZE/4-75, VSIZE/2-50, 150, 100, 26, "Acierto")){
        estado = 1;
    }

    ComFgcolor(255, 0, 0);
    if(Boton(3*HSIZE/4-75, VSIZE/2-50, 150, 100, 26, "Fallo")){
        estado = 2;
    }
    return estado;
}

int MiniJuego2(void) {
    int estado = 0;
    //Fondo
    ComColor(110, 110, 156);
    ComLineWidth(5);
    ComRect(10, 10, HSIZE-10, VSIZE-10, true);

    ComColor(0, 0, 0);
    ComTXT(HSIZE/2,VSIZE/5-10, 29, OPT_CENTERX,"MiniJuego2");

    ComFgcolor(0, 255, 0);
    if(Boton(HSIZE/4-75, VSIZE/2-50, 150, 100, 26, "Acierto")){
        estado = 1;
    }

    ComFgcolor(255, 0, 0);
    if(Boton(3*HSIZE/4-75, VSIZE/2-50, 150, 100, 26, "Fallo")){
        estado = 2;
    }
    return estado;
}

int MiniJuego3(void) {
    int estado = 0;
    //Fondo
    ComColor(0, 255, 255);
    ComLineWidth(5);
    ComRect(10, 10, HSIZE-10, VSIZE-10, true);

    ComColor(0, 0, 0);
    ComTXT(HSIZE/2,VSIZE/5-10, 29, OPT_CENTERX,"MiniJuego3");

    ComFgcolor(0, 255, 0);
    if(Boton(HSIZE/4-75, VSIZE/2-50, 150, 100, 26, "Acierto")){
        estado = 1;
    }

    ComFgcolor(255, 0, 0);
    if(Boton(3*HSIZE/4-75, VSIZE/2-50, 150, 100, 26, "Fallo")){
        estado = 2;
    }
    return estado;
}



int main(void){
    // Configuración del reloj
    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    // LEDs integrados
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 |GPIO_PIN_4); //F0 y F4: salidas
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1); //N0 y N1: salidas

    // Configuración Timer e interrupción
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/FREC)-1);
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer);
    IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
    IntMasterEnable();  //Habilitacion global de interrupciones
    TimerEnable(TIMER0_BASE, TIMER_A);  //Habilitar Timer0, 1, 2A y 2B

    // Configuración modos bajo consumo
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralClockGating(true);

    // Pantalla y configuración pantalla inicio
    SysCtlDelay(RELOJ/3);
    HAL_Init_SPI(2, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();       //Arranque de la pantalla
    VolNota(seleccion);

    Nueva_pantalla(16,16,16);
    ComColor(0, 100, 100);
    ComLineWidth(5);
    ComRect(5, 5, HSIZE-5, VSIZE-5, true);
    ComColor(0, 134, 124);
    ComRect(12, 12, HSIZE-12, VSIZE-12, true);

    ComColor(255,255,255);
    ComTXT(HSIZE/2,10+VSIZE/5, 31, OPT_CENTERX,"Minijuegos");
    ComTXT(HSIZE/2,60+VSIZE/5, 31, OPT_CENTERX,"(basicos)");
    ComTXT(HSIZE/2,120+VSIZE/5, 28, OPT_CENTERX,"SEPA GIERM. 2026");

    ComRect(40,40, HSIZE-40, VSIZE-40, false);

    Dibuja();
    Espera_pant();


    // Calibración pantalla táctil
    int i;
    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);

    int juego_actual = 1;
    int estado = 0;
    int estado_prev = 0;

    while(1){
        SLEEP;

        Nueva_pantalla(16, 16, 16);

        switch(juego_actual){
        case 1: estado = MiniJuego1(); break;
        case 2: estado = MiniJuego2(); break;
        case 3: estado = MiniJuego3(); break;
        }
        Dibuja();

        if(estado_prev != 0 && estado == 0){
                juego_actual++;
                if(juego_actual > 3) juego_actual = 1;
            }
            estado_prev = estado;
    }
}

// Interrupción
void IntTimer(void){
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
    cont_int++;
    if(cont_int>=10){
        cont_int = 0;
        Cambia = 1;
    }

}

