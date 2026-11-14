#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include "touch.h"

/***********************************
 * Demo ruiditos sensor capacitivo
 * Conexion: VCC -> 3V3, GND -> GND, I/O -> PP5 (BoosterPack 2)
 *************************************/

// V.Pantalla
const int32_t REG_CAL[6]= {CAL_DEFAULTS}; // Calibración
#define TAG(x)       ((3UL << 24) | ((x) & 0xFF)) // Detección widget tocado

// V.Reloj
int RELOJ; // Variable que guarda la frecuencia del programa
#define FREC 50 //Frecuencia en hercios del tren de pulsos: 20ms
#define MS_TICK (1000U / FREC)  // Milisegundos por interrupcion

// V. Programa
volatile char Cambia=0;
volatile int seleccion = 127;   // Volumen (0-255)

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

void NotasAleatorias(void)
{
    static char * const nombres_nota[7] = {"Do", "Re", "Mi", "Fa", "Sol", "La", "Si"};
    static char * const nombres_instr[4] = {"Piano", "Xilo", "Triang", "Warble"};

    // Estado de la nota que esta sonando
    static bool     sonando = false;
    static uint32_t ticks_restantes = 0;
    static int      nota_idx = 0;
    static int      instr_idx = 0;
    static bool     semilla = false;

    int notas[7] = {N_DO, N_RE, N_MI, N_FA, N_SOL, N_LA, N_SI };
    int instrumentos[4] = {S_PIANO, S_XILO, S_TRIANG, S_WARBLE };

    char texto[32];
    int i, f;

    // Silenciar si lleva un tiempo sonando
    if (sonando){
        if (ticks_restantes > 0)
            ticks_restantes--;
        if (ticks_restantes == 0){
            FinNota();
            sonando = false;
        }
    }

    // Se toca el sensor
    if (TOUCH_Actualizar() == TOUCH_EV_TOQUE){
        // Genera una semilla aleatoria
        if (!semilla){
            srand(TimerValueGet(TIMER0_BASE, TIMER_A));
            semilla = true;
        }

        nota_idx  = rand() % 7;
        instr_idx = rand() % 4;

        VolNota(seleccion);
        TocaNota(instrumentos[instr_idx], notas[nota_idx]);

        sonando = true;
        ticks_restantes = 1000U / MS_TICK;
    }

    // Dibuja la tabla
    int x0 = (HSIZE - 384) / 2;
    int y0 = 54;
    int x1 = x0 + 384;
    int y1 = y0 + 150;

    // Titulo ejemplo
    ComColor(0, 0, 0);
    ComTXT(HSIZE / 2, 27, 26, OPT_CENTERX, "Notas aleatorias");

    // Color para columnas de la tabla
    ComColor(0, 134, 124);
    ComRect(x0, y0, x1, y0 + 30, true);

    // Señalar tecla sonando
    if (sonando){
        int cx = x0 + 90 + nota_idx * 42;
        int cy = y0 + (instr_idx + 1) * 30;
        ComColor(255, 235, 120);
        ComRect(cx, cy, cx + 42, cy + 30, true);
    }

    // Lineas de la tabla
    ComColor(0, 0, 0);
    for (f = 0; f <= 5; f++){
        ComRect(x0, y0 + f * 30 - 1, x1, y0 + f * 30 + 1, true);
    }

    ComRect(x0 - 1, y0, x0 + 1, y1, true);
    for (i = 0; i <= 7; i++){
        ComRect(x0 + 90 + i * 42 - 1, y0, x0 + 90 + i * 42 + 1, y1, true);
    }

    // Titulos columnas (notas)
    ComColor(255, 255, 255);
    for (i = 0; i < 7; i++){
        ComTXT(x0 + 90 + i * 42 + 21, y0 + 5, 26, OPT_CENTERX, nombres_nota[i]);
    }

    // Titulos filas (instrumentos)
    ComTXT(x0 + 8, y0 + 5, 26, 0, "Instr.");
    ComColor(0, 0, 0);
    for (f = 0; f < 4; f++){
        ComTXT(x0 + 8, y0 + (f + 1) * 30 + 5, 26, 0, nombres_instr[f]);
    }

    // Circulo en la casilla que suena
    if (sonando){
        int cxc = x0 + 90 + nota_idx * 42 + 21;
        int cyc = y0 + (instr_idx + 1) * 30 + 15;

        ComColor(0, 0, 0);
        ComCirculo(cxc, cyc, 12);       // Borde
        ComColor(220, 0, 0);
        ComCirculo(cxc, cyc, 9);        // Relleno
    }

    // Texto debajo tabla
    ComColor(0, 0, 0);
    if (sonando){
        sprintf(texto, "Sonando: %s - %s", nombres_nota[nota_idx], nombres_instr[instr_idx]);
    }else{
        sprintf(texto, "Toca el sensor");
    }
    ComTXT(HSIZE / 2, y1 + 10, 26, OPT_CENTERX, texto);
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
    SysCtlPeripheralSleepEnable(TOUCH_PERIPH);
    SysCtlPeripheralClockGating(true);

    TOUCH_Init(MS_TICK);

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
    ComTXT(HSIZE/2,10+VSIZE/5, 31, OPT_CENTERX,"DEMO");
    ComTXT(HSIZE/2,60+VSIZE/5, 31, OPT_CENTERX,"Capacitivo");
    ComTXT(HSIZE/2,120+VSIZE/5, 28, OPT_CENTERX,"SEPA GIERM. 2026");

    ComRect(40,40, HSIZE-40, VSIZE-40, false);

    Dibuja();
    Espera_pant();


    // Calibración pantalla táctil
    int i;
    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);

    while(1){
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

        // Teclado inferior
        NotasAleatorias();
        Dibuja();
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

