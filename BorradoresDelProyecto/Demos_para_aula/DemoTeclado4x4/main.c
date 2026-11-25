#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "teclado.h"
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"


/***********************************
 * Demo matriz 4x4 - telefonillo
 * Conexion (BoosterPack 2):
 *     Filas    1..4 -> PK0, PK1, PK2, PK3   (B2 pines 5..8)
 *     Columnas 1..4 -> PK4, PK5, PK6, PK7   (C2 pines 2, 3, 9, 10)
 *************************************/

// V.Pantalla
const int32_t REG_CAL[6]= {CAL_DEFAULTS}; // Calibraci�n
#define TAG(x)       ((3UL << 24) | ((x) & 0xFF)) // Detecci�n widget tocado
int seleccion = 127;

// Etiqueta del slider
#define TAG_SLIDER 34

// V.Reloj
int RELOJ; // Variable que guarda la frecuencia del programa
#define FREC 50 //Frecuencia en hercios del tren de pulsos: 20ms

// V.Teclado
#define MS_TICK         (1000U / FREC)
#define TAM_CODIGO      16U         // maximo de teclas guardadas
teclado_evento_t ev;
char tecla;
char codigo[TAM_CODIGO + 1U];
uint32_t n = 0U;

// Interrupci�n falsa
volatile int Flag_ints=0;
void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

//#define SLEEP SysCtlSleep()
#define SLEEP SysCtlSleepFake()
void IntTimer(void);


void DibujaTeclaVisual(int x, int y, char *texto){
    // Fondo de la tecla
    ComColor(55, 75, 90);
    ComRect(x-15, y-9, x+15, y+9, 1);

    // Borde
    ComColor(180, 200, 210);
    ComLineWidth(1);
    ComRect(x-15, y-9, x+15, y+9, 0);

    // Numero
    ComColor(255, 255, 255);
    ComTXT(x, y-8, 20, OPT_CENTERX, texto);
}

void Telefono(void)
{
    // Declaración de variables
    static char numeroMostrado[TAM_CODIGO + 1U] = "";
    static char personaLlamada[32] = "Sin llamada";
    static char mensajePersona[64] = "";

    // Se consulta el estado del teclado
    ev = TECLADO_Actualizar();
    
    // Se registra la tecla o se deja de reproducir el sonido
    switch(ev){
    case TECLADO_EV_PULSADA:
        tecla = TECLADO_Tecla();    // Tecla pulsada

        if(tecla >= '0' && tecla <= '9'){
            TocaNota(S_XILO, (rand() % 11) + 48);
            if(n < TAM_CODIGO){
                codigo[n++] = tecla;
                codigo[n] = '\0';

                strcpy(numeroMostrado, codigo);
                strcpy(personaLlamada, "Marcando...");
                strcpy(mensajePersona, "");
            }
        }

        // Borrar
        else if(tecla == '*'){
            TocaNota(S_XILO, 70);
            n = 0;
            codigo[0] = '\0';
            numeroMostrado[0] = '\0';
            strcpy(personaLlamada, "Sin llamada");
            strcpy(mensajePersona, "");
        }

        // Realizar llamada
        else if(tecla == '#'){
            TocaNota(S_XILO, 60);
            codigo[n] = '\0';

            if(n == 0){
                strcpy(personaLlamada, "Sin numero");
                strcpy(mensajePersona, "Introduce un numero");
            }else if(strcmp(codigo, "0000") == 0){
                strcpy(personaLlamada, "Contacto 0000");
                strcpy(mensajePersona, "Hola! Soy el contacto 0000");
            }else if(strcmp(codigo, "01") == 0){
                strcpy(personaLlamada, "Contacto 01");
                strcpy(mensajePersona, "Buenos dias! Quien eres?");
            }else if(strcmp(codigo, "34231") == 0){
                strcpy(personaLlamada, "MrP");
                strcpy(mensajePersona, "Hola MrP! Que tal?");
            }else{
                strcpy(personaLlamada, "Numero desconocido");
                strcpy(mensajePersona, "No conozco este numero");
            }

            n = 0;
            codigo[0] = '\0';
        }
        break;

    case TECLADO_EV_SOLTADA:
        FinNota();
        break;

    default:
        break;
    }

    // Dibujar pantalla
    ComColor(20, 65, 75);
    ComRect(170, 27, 310, 190, 1);

    ComColor(120, 180, 185);
    ComRect(170, 27, 310, 190, 0);

    // Titulo
    ComColor(255, 255, 255);
    ComTXT(240, 32, 20, OPT_CENTERX, "TELEFONO");

    // Numero marcado
    ComColor(100, 255, 170);

    if(numeroMostrado[0] != '\0'){
        ComTXT(240, 52, 24, OPT_CENTERX, numeroMostrado);
    }else{
        ComTXT(240, 52, 20, OPT_CENTERX, "INTRODUZCA NUMERO");
    }

    // Dibujar teclado
    DibujaTeclaVisual(200, 98, "1");
    DibujaTeclaVisual(240, 98, "2");
    DibujaTeclaVisual(280, 98, "3");

    DibujaTeclaVisual(200, 120, "4");
    DibujaTeclaVisual(240, 120, "5");
    DibujaTeclaVisual(280, 120, "6");

    DibujaTeclaVisual(200, 142, "7");
    DibujaTeclaVisual(240, 142, "8");
    DibujaTeclaVisual(280, 142, "9");

    DibujaTeclaVisual(200, 164, "*");
    DibujaTeclaVisual(240, 164, "0");
    DibujaTeclaVisual(280, 164, "#");


    // Cuadro texto
    // Fondo blanco
    ComColor(255, 255, 255);
    ComRect(20, 200, 460, 268, 1);

    // Borde negro
    ComColor(0, 0, 0);
    ComLineWidth(2);
    ComRect(20, 200, 460, 268, 0);

    // Etiqueta
    ComColor(100, 100, 100);
    ComTXT(35, 211, 16, 0, "PERSONA LLAMADA:");

    // Nombre de la persona
    ComColor(0, 80, 100);
    ComTXT(35, 229, 20, 0, personaLlamada);

    // Mensaje de la persona
    ComColor(0, 0, 0);
    ComTXT(35, 249, 18, 0, mensajePersona);

}

int main(void){
    // Configuraci�n del reloj
    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

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

    // Configuraci�n Timer e interrupci�n
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);       //Habilita T0
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);   //T0 a 120MHz
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);    //T0 periodico y conjunto (32b)
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/FREC)-1);
    TimerIntRegister(TIMER0_BASE,TIMER_A,IntTimer);
    IntEnable(INT_TIMER0A); //Habilitar las interrupciones globales de los timers
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);    // Habilitar las interrupciones de timeout
    IntMasterEnable();  //Habilitacion global de interrupciones
    TimerEnable(TIMER0_BASE, TIMER_A);  //Habilitar Timer0, 1, 2A y 2B

    // Configuraci�n modos bajo consumo
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralClockGating(true);

    // Pantalla y configuraci�n pantalla inicio
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
    ComTXT(HSIZE/2,60+VSIZE/5, 31, OPT_CENTERX,"Matriz 4x4");
    ComTXT(HSIZE/2,120+VSIZE/5, 28, OPT_CENTERX,"SEPA GIERM. 2026");

    ComRect(40,40, HSIZE-40, VSIZE-40, false);

    Dibuja();
    Espera_pant();


    // Calibraci�n pantalla t�ctil
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
        Telefono();
        Dibuja();
    }
}

// Interrupci�n
void IntTimer(void){
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}

