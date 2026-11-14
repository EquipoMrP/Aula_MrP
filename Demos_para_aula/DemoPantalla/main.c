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


void TecladoMusical(void) {
    // Variable estática para recordar qué tecla está sonando actualmente
    static unsigned char tecla_anterior = 0;
    unsigned char tecla_pulsada;

    static int octava_semitonos = 0; // -12, 0 y +12
    static bool gauge_pulsado_anterior = false;

    // --- 1. GESTIÓN DEL GAUGE INTERACTIVO DE OCTAVA (Margen superior >= 50px) ---
#define TAG_GAUGE 60

    // Rastrear el área táctil del Gauge (Centro en X=80, Y=100, Radio=45 -> Top en Y=55)
    cmd_track(35, 55, 90, 90, TAG_GAUGE);

    uint32_t track_val = Lee_Reg(REG_TRACKER);
    uint16_t tag_pulsado = (uint16_t)(track_val & 0xFFFFU);
    uint16_t raw_val = (uint16_t)(track_val >> 16);

    // Si tocamos el gauge, cambiamos de octava de forma cíclica
    if (tag_pulsado == TAG_GAUGE) {
        if (!gauge_pulsado_anterior) {
            gauge_pulsado_anterior = true;

            if (octava_semitonos == -12) {
                octava_semitonos = 0;      // Normal
            } else if (octava_semitonos == 0) {
                octava_semitonos = 12;     // +1 Octava
            } else {
                octava_semitonos = -12;    // -1 Octava
            }
        }
    } else {
        gauge_pulsado_anterior = false;
    }

    // Dibujar el Gauge indicador de octava (Centro en X=80, Y=100, Radio=45)
    int valor_gauge = octava_semitonos + 12;

    ComColor(255, 255, 255);
    EscribeRam32(TAG(TAG_GAUGE));
    cmd_gauge(80, 100, 45, 0, 2, 2, valor_gauge, 24);
    EscribeRam32(TAG(255));

    // Cajón para el texto de la octava (situado debajo del gauge, Y: 148 a 170)
    ComColor(0, 0, 0);
    ComRect(35, 148, 125, 170, true);
    ComColor(255, 255, 255);
    ComRect(37, 150, 123, 168, true);

    ComColor(0, 0, 0);
    sprintf(cadena, "Oct: %+d", octava_semitonos / 12);
    ComTXT(42, 152, 26, 0, cadena);


    // --- 2. AJUSTE DE VOLUMEN Y SU SLIDER (Desplazado a la derecha) ---
    cmd_track(150, 90, 180, 15, TAG_SLIDER);

    if (tag_pulsado == TAG_SLIDER){
        seleccion = ((uint32_t)raw_val * 255U) / 65535U;
    }

    // Dibujar el slider de volumen
    EscribeRam32(TAG(TAG_SLIDER));
    ComColor(100, 100, 100);
    ComFgcolor(200, 200, 200);
    ComBgcolor(30, 30, 30);
    cmd_slider(150, 90, 180, 10, 0, seleccion, 255);
    EscribeRam32(TAG(255));

    // Cajón de volumen al lado del slider
    ComColor(0, 0, 0);
    ComRect(340, 82, 420, 108, true);
    ComColor(255, 255, 255);
    ComRect(342, 84, 418, 106, true);

    ComColor(0, 0, 0);
    sprintf(cadena, "VOL: %.0f%%", (float)seleccion * 100.0f / 255.0f);
    ComTXT(345, 87, 26, 0, cadena);


    // --- 3. DIBUJO DEL TECLADO MUSICAL CON COLOR DINÁMICO ---
    contador_paso++;
    if (contador_paso >= 40) {
        contador_paso = 0;
        estado_color++;
        if (estado_color > 2) {
            estado_color = 0;
        }
    }

    EscribeRam32(CMD_GRADCOLOR);
    switch (estado_color) {
    case 0: // ROJO
        EscribeRam32(0xFF0000);
        break;
    case 1: // VERDE
        EscribeRam32(0x00FF00);
        break;
    case 2: // AZUL
        EscribeRam32(0x0000FF);
        break;
    }

    ComTeclas((HSIZE-HSIZE*3/4)/2, VSIZE-VSIZE/4-15, (HSIZE * 3)/4-10, VSIZE/4-10, 31, 0, "CDEFGAB");
    EscribeRam32(CMD_GRADCOLOR);
    EscribeRam32(0x00555555);


    // --- 4. LECTURA TÁCTIL Y AUDIO (APLICANDO OCTAVA) ---
    tecla_pulsada = Lee_Reg8(REG_TOUCH_TAG);

    if (tag_pulsado != TAG_SLIDER && tag_pulsado != TAG_GAUGE && tecla_pulsada != 0 && tecla_pulsada != 255) {
        if (tecla_pulsada != tecla_anterior) {
            VolNota(seleccion);

            int nota_base = 0;
            switch (tecla_pulsada) {
            case 'C': nota_base = N_DO; break;
            case 'D': nota_base = N_RE; break;
            case 'E': nota_base = N_MI; break;
            case 'F': nota_base = N_FA; break;
            case 'G': nota_base = N_SOL; break;
            case 'A': nota_base = N_LA; break;
            case 'B': nota_base = N_SI; break;
            default: break;
            }

            // Reproducir nota sumando los semitonos de octava (-12, 0 o +12)
            TocaNota(S_PIANO, nota_base + octava_semitonos);
            tecla_anterior = tecla_pulsada;
        }
    } else {
        if (tag_pulsado != TAG_GAUGE && tecla_anterior != 0) {
            FinNota();
            tecla_anterior = 0;
        }
    }
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
    ComTXT(HSIZE/2,10+VSIZE/5, 31, OPT_CENTERX,"DEMO");
    ComTXT(HSIZE/2,60+VSIZE/5, 31, OPT_CENTERX,"Pantalla resistiva");
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
        TecladoMusical();
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

