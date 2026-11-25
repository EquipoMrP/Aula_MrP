#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include "hcsr04.h"

#define FREC             50U
#define TICKS_MEDIDA     3U       // 60 ms
#define TICKS_PRINT      25U      // 500 ms

#define DIST_MIN_MM     50U    // 5 cm
#define DIST_MAX_MM     400U   // 40 cm

#define NUM_SEGMENTOS   10
#define SEG_ANCHO       22
#define SEG_ALTO        28
#define SEG_ESPACIO     7

//#define NOTA_MIN          48U  // DO3
//#define NOTA_MAX          84U  // DO6

#define NOTA_MIN          68U  // DO3
#define NOTA_MAX          104U  // DO6

const int32_t REG_CAL[6] = {CAL_DEFAULTS};

#define TAG(x) ((3UL << 24) | ((x) & 0xFF))

int RELOJ;

volatile int Flag_ints = 0;

void IntTimer(void);
void SysCtlSleepFake(void);

void SysCtlSleepFake(void)
{
    while (!Flag_ints);

    Flag_ints = 0;
}

#define SLEEP SysCtlSleepFake()


void Theremin(void)
{
    // Variables persistentes entre llamadas.
    static uint8_t cuenta_medida = 0;
    //    static uint8_t cuenta_print = 0;

    static uint32_t mm = 0;
    static bool valida = false;

    static uint8_t nota_anterior = 255;
    static bool sonido_activo = false;
    uint8_t nota_midi;

    uint32_t segmentos_activos = 0;

    int i;
    int x;
    int y;
    int ancho_total;
    int x_inicio;

    char texto[40];
    cuenta_medida++;

    if (cuenta_medida >= TICKS_MEDIDA){
        cuenta_medida = 0;
        valida = HCSR04_Listo();
        if (valida){
            mm = HCSR04_LeerMM();
            if (mm < DIST_MIN_MM){mm = DIST_MIN_MM;}
            if (mm > DIST_MAX_MM){mm = DIST_MAX_MM;}
            nota_midi = NOTA_MIN+((DIST_MAX_MM-mm)*(NOTA_MAX-NOTA_MIN))/(DIST_MAX_MM-DIST_MIN_MM);

            if (!sonido_activo || nota_midi != nota_anterior){
                TocaNota(S_PIPS, nota_midi);
                nota_anterior = nota_midi;
                sonido_activo = true;
            }
        }else{
            if (sonido_activo){
                FinNota();

                sonido_activo = false;
                nota_anterior = 255;
            }
        }
        HCSR04_Disparar();
    }



    if (valida){
        segmentos_activos =((DIST_MAX_MM - mm) * NUM_SEGMENTOS) /(DIST_MAX_MM - DIST_MIN_MM);
    }

    //    cuenta_print++;
    //    if (cuenta_print >= TICKS_PRINT){
    //        cuenta_print = 0;
    //        if (valida){UARTprintf("Distancia: %u.%u cm\r\n",(unsigned int)(mm / 10),(unsigned int)(mm % 10));}
    //        else{UARTprintf("Sin eco / fuera de rango. Fallos: %u\r\n",(unsigned int)HCSR04_Fallos());}
    //    }

    ancho_total = NUM_SEGMENTOS * SEG_ANCHO +(NUM_SEGMENTOS - 1) * SEG_ESPACIO;
    x_inicio = (HSIZE - ancho_total) / 2;
    y = VSIZE / 2;

    // Titulo.
    ComColor(30, 45, 55);
    ComTXT(HSIZE / 2, VSIZE / 4, 31,OPT_CENTERX, "THEREMIN");


    // Dibujar los diez segmentos.
    for (i = 0; i < NUM_SEGMENTOS; i++){
        x = x_inicio + i * (SEG_ANCHO + SEG_ESPACIO);

        if (valida && (uint32_t)i < segmentos_activos){
            if (i < 4){ComColor(35, 175, 95);} // Verde
            else if (i < 7){ComColor(240, 185, 45);} // Amarillo
            else{ComColor(225, 65, 65);} // Rojo
        }
        else{
            ComColor(195, 195, 190);
        }
        ComRect(x, y,x + SEG_ANCHO,y + SEG_ALTO,true);
    }

    ComColor(30, 45, 55);
    ComTXT(x_inicio + 30, y + 42, 20,OPT_CENTERX, "LEJOS");
    ComTXT(x_inicio + ancho_total - 30, y + 42, 20,OPT_CENTERX, "CERCA");


    if (valida){
        sprintf(texto, "Distancia: %u.%u cm",(unsigned int)(mm / 10),(unsigned int)(mm % 10));
        ComTXT(HSIZE / 2, y + 85, 27,OPT_CENTERX, texto);
    }else{
        ComTXT(HSIZE / 2, y + 85, 27,OPT_CENTERX, "ESPERANDO SENSOR");
    }
}


int main(void){
    RELOJ = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |SYSCTL_OSC_MAIN |SYSCTL_USE_PLL |SYSCTL_CFG_VCO_480),120000000U);

    // UART0: se conserva la configuracion original.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, RELOJ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE,GPIO_PIN_0 | GPIO_PIN_1);

    HCSR04_Init((uint32_t)RELOJ);
    HCSR04_Disparar();

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


    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralClockGating(true);

    // Declaración pantalla
    SysCtlDelay(RELOJ / 3U);
    HAL_Init_SPI(2, RELOJ);
    Inicia_pantalla();
    VolNota(10);
    FinNota();

    // Pantalla de inicio
    Nueva_pantalla(16, 16, 16);
    ComColor(0, 100, 100);
    ComLineWidth(5);

    ComRect(5, 5, HSIZE-5, VSIZE-5, true);
    ComColor(0, 134, 124);
    ComRect(12, 12, HSIZE-12, VSIZE-12, true);

    ComColor(255, 255, 255);

    ComTXT(HSIZE/2, 10+VSIZE/5, 31, OPT_CENTERX, "DEMO");
    ComTXT(HSIZE/2, 60+VSIZE/5, 31, OPT_CENTERX, "Ultrasonidos");
    ComTXT(HSIZE/2, 120+VSIZE/5, 28, OPT_CENTERX, "SEPA GIERM. 2026");
    ComRect( 40, 40, HSIZE - 40, VSIZE - 40, false);

    Dibuja();

    Espera_pant();


    // Calibración pantalla táctil
    int i;
    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);

    while (1){
        SLEEP;

        Nueva_pantalla(16, 16, 16);

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

        // Actualizar theremin.
        Theremin();

        // Dibujar pantalla.
        Dibuja();
    }
}


void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE,TIMER_TIMA_TIMEOUT);
    Flag_ints++;
}
