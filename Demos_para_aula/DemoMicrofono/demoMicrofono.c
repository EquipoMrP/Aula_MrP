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


#define MICROFONO_ON GPIOPinRead(GPIO_PORTE_BASE,GPIO_PIN_3)

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
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_3);   //E3: entrada
    GPIOPadConfigSet(GPIO_PORTE_BASE,GPIO_PIN_3,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU); //Pullup en E3

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
    ComColor(200, 200, 200);
    ComRect(12, 12, HSIZE-12, VSIZE-12, true);
    //Texto
    ComColor(0,0,0);
    ComTXT(HSIZE/2,VSIZE/5, 30, OPT_CENTERX,"Demo Microfono");
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

        despiertaMrP();


        ComNum(20,20,31,0,Load);

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


void dibujarMrP(int x, int y, char idEmocion, char gafas, char cuerpo, char idAnimacion){

    //Variables para el control de la posicion y todo lo demas
    static int xC=0;        //Eje de revolucion de la cabeza, x
    static int yC=0;        //Eje de revolucion de la cabeza, y
    //static float ang=0;    //Angulo respecto la vertical

    //Animacion (se hace ahora porque toca el resto de variables)
    switch (idAnimacion){

    case 6: //Dormir - W.I.P.
        xC=x;
        if(yC<y+135){
            yC=y+135;
        } else if(yC<y+155){
            yC++;
        }
        break;

    default: //Default es el case 0, sin Animacion
        xC=x;
        yC=y+135;
        break;
    }

    //Si el cuerpo es visible, lo dibujamos
        if(cuerpo){
            //Dibujamos la "camisa"
            ComColor(150,150,150); //Gris
            int i=0;
            for(i=-2;i<3;i++){
                ComLine(x,y+175,x+(i*15),y+300,30);
            }
        }

    //Dibujamos el pelo pt 1
    ComColor(61, 43, 26); //Color marron
    ComLine(xC-22,yC-105,xC+22,yC-105,30);

    //Dibujamos la piel de la cara
    ComColor(255,181,144);//Color piel
    ComCirculo(xC,yC-75,43);
    ComCirculo(xC,yC-50,43);
    //ComRect(C-38,yC+60,C+38,yC+80,true);
    ComLine(xC-33, yC-75,xC-33, yC-50,10);
    ComLine(xC+33, yC-75,xC+33, yC-50,10);
    ComLine(xC-28, yC-63,xC+28, yC-63,15);

    //Dibujamos el pelo pt 2
    ComColor(61, 43, 26); //Color marron
    ComLine(xC-25,yC-106,xC+25,yC-106,12);
    ComColor(107, 91, 57); //Color marron claro
    ComLine(xC-5,yC-115,xC+5,yC-115,7);

    //Dibujamos la nariz
    ComColor(0,0,0); //Negro
    ComCirculo(xC,yC-38,13);
    ComColor(255,181,144);//Color piel
    ComLine(xC-13,yC-36,xC+13,yC-36,12);

    switch (idEmocion){
    case 0: //Contento
        //Ojos
        if(gafas){
            ComColor(0,0,0); //Negro
            ComLine(xC-27,yC-67,xC-27,yC-62,4);
            ComLine(xC+27,yC-67,xC+27,yC-62,4);
        } else { //Si no tiene gafas, ojos en forma de 3
            ComColor(0,0,0); //Negro
            ComCirculo(xC+22,yC-68,6); // Up R
            ComCirculo(xC+22,yC-61,6); // Down R
            ComCirculo(xC-22,yC-68,6); // Up L
            ComCirculo(xC-22,yC-61,6); // Donw L
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+22,yC-68,3); // Up R
            ComCirculo(xC+22,yC-61,3); // Down R
            ComCirculo(xC-22,yC-68,3); // Up L
            ComCirculo(xC-22,yC-61,3); // Donw L
            ComLine(xC-26,yC-66,xC-26,yC-64,4); //Recorte L
            ComLine(xC+18,yC-66,xC+18,yC-64,4); //Recorte R
        }

        // Boca
        ComColor(0,0,0); //Negro
        ComCirculo(xC,yC-38,13);
        ComColor(255,181,144);//Color piel
        ComLine(xC-13,yC-40,xC+13,yC-40,8);
        // Manos
        break;

    case 1: //Shock
        //Ojos (En shock no dependen de si gafas o no) (Aun asi, si no hay gafas se dibujan mas cerca entre si)
        if(gafas){
            ComColor(0,0,0); //Negro - Borde
            ComCirculo(xC-27,yC-65,13);
            ComCirculo(xC+27,yC-65,13);
            ComColor(255,255,255); //Blanco - Blanco
            ComCirculo(xC+27,yC-65,12);
            ComCirculo(xC-27,yC-65,12);
            ComColor(0,0,0); //Negro - Pupila
            ComCirculo(xC-27,yC-65,5);
            ComCirculo(xC+27,yC-65,5);
        } else {
            ComColor(0,0,0); //Negro - Borde
            ComCirculo(xC-24,yC-65,13);
            ComCirculo(xC+24,yC-65,13);
            ComColor(255,255,255); //Blanco - Blanco
            ComCirculo(xC+24,yC-65,12);
            ComCirculo(xC-24,yC-65,12);
            ComColor(0,0,0); //Negro - Pupila
            ComCirculo(xC-24,yC-65,5);
            ComCirculo(xC+24,yC-65,5);

        }
        //Boca
        ComColor(0,0,0); //Negro
        ComCirculo(xC,yC-32,7);

        // Manos
        break;

    case 2: //Dormido
        //Ojos (En dormido no dependen de si gafas o no) (Aun asi, si sin gafas se dibujan mas cerca entre si
        if(gafas){
        ComColor(0,0,0); //Negro
        ComCirculo(xC+27,yC-74,12);
        ComCirculo(xC-27,yC-74,12);
        ComColor(255,181,144);//Color piel
        ComCirculo(xC+27,yC-74,10);
        ComCirculo(xC-27,yC-74,10);
        }else {
            ComColor(0,0,0); //Negro
            ComCirculo(xC+24,yC-74,12);
            ComCirculo(xC-24,yC-74,12);
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+24,yC-74,10);
            ComCirculo(xC-24,yC-74,10);
        }
        ComLine(xC-32,yC-80,xC+32,yC-80,11);

        //Boca
        ComColor(0,0,0); //Negro
        ComCirculo(xC,yC-38,13);
        ComColor(255,181,144);//Color piel
        ComLine(xC-13,yC-40,xC+13,yC-40,8);

        // Manos
        break;

    case 3: //Triste
        //Ojos
        if (gafas){
        ComColor(0,0,0); //Negro
        ComLine(xC-27,yC-67,xC-27,yC-62,4);
        ComLine(xC+27,yC-67,xC+27,yC-62,4);
        ComColor(255,181,144);//Color piel;
        ComLine(xC-32,yC-70,xC-23,yC-74,4);
        ComLine(xC+32,yC-70,xC+23,yC-74,4);
        } else { //Si no tiene gafas, ojos en forma de 3
            ComColor(0,0,0); //Negro
            ComLine(xC-27,yC-63,xC-18,yC-67,2); //Ceja L
            ComLine(xC+27,yC-63,xC+18,yC-67,2); //Ceja R
        }

        //Boca
        ComColor(0,0,0); //Negro
        ComCirculo(xC,yC-20,12);
        ComColor(255,181,144);//Color piel
        ComLine(xC-7,yC-17,xC+7,yC-17,9);
        ComLine(xC,yC-16,xC,yC-16,9);

        // Manos
        break;

    case 4: //Enfadado
        //Ojos
        if(gafas){
            ComColor(0,0,0); //Negro
            ComLine(xC-27,yC-67,xC-27,yC-62,4);
            ComLine(xC+27,yC-67,xC+27,yC-62,4);
            ComLine(xC-23,yC-70,xC-32,yC-74,2);
            ComLine(xC+23,yC-70,xC+32,yC-74,2);
        } else { //Si no tiene gafas, ojos en forma de 3
            ComColor(0,0,0); //Negro
            ComCirculo(xC+22,yC-68,6); // Up R
            ComCirculo(xC+22,yC-61,6); // Down R
            ComCirculo(xC-22,yC-68,6); // Up L
            ComCirculo(xC-22,yC-61,6); // Donw L
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+22,yC-68,3); // Up R
            ComCirculo(xC+22,yC-61,3); // Down R
            ComCirculo(xC-22,yC-68,3); // Up L
            ComCirculo(xC-22,yC-61,3); // Donw L
            ComLine(xC-26,yC-66,xC-26,yC-64,4); //Recorte L
            ComLine(xC+18,yC-66,xC+18,yC-64,4); //Recorte R
            ComColor(0,0,0);
            ComLine(xC-17,yC-74,xC-28,yC-76,2); //Ceja L
            ComLine(xC+17,yC-74,xC+28,yC-76,2); //Ceja R
        }

        //Boca
        ComColor(0,0,0); //Negro
        ComCirculo(xC,yC-20,12);
        ComColor(255,181,144);//Color piel
        ComLine(xC-7,yC-17,xC+7,yC-17,9);
        ComLine(xC,yC-16,xC,yC-16,9);

        // Manos
        break;


    case 5: //Desafiante
        //Ojos
        if(gafas){
            ComColor(0,0,0); //Negro
            ComLine(xC-27,yC-67,xC-27,yC-62,4);
            ComLine(xC+27,yC-67,xC+27,yC-62,4);
            ComLine(xC-23,yC-70,xC-32,yC-74,2);
            ComLine(xC+23,yC-70,xC+32,yC-74,2);
        } else { //Si no tiene gafas, ojos en forma de 3
            ComColor(0,0,0); //Negro
            ComCirculo(xC+22,yC-68,6); // Up R
            ComCirculo(xC+22,yC-61,6); // Down R
            ComCirculo(xC-22,yC-68,6); // Up L
            ComCirculo(xC-22,yC-61,6); // Donw L
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+22,yC-68,3); // Up R
            ComCirculo(xC+22,yC-61,3); // Down R
            ComCirculo(xC-22,yC-68,3); // Up L
            ComCirculo(xC-22,yC-61,3); // Donw L
            ComLine(xC-26,yC-66,xC-26,yC-64,4); //Recorte L
            ComLine(xC+18,yC-66,xC+18,yC-64,4); //Recorte R
            ComColor(0,0,0);
            ComLine(xC-17,yC-74,xC-28,yC-76,2); //Ceja L
            ComLine(xC+17,yC-74,xC+28,yC-76,2); //Ceja R
        }

        //Boca
        ComColor(0,0,0); //Negro
        ComCirculo(xC,yC-38,13);
        ComColor(255,181,144);//Color piel
        ComLine(xC-13,yC-40,xC+13,yC-40,8);

        // Manos
        break;

    default:
        idEmocion=0;
        break;
    }


    //Gafas
    if(gafas){
        ComColor(0,0,80); //Azul oscuro
            //Izquierda
        ComLine(xC-47,yC-78,xC-8 ,yC-78,2); //Arriba
        ComLine(xC-47,yC-52,xC-8 ,yC-52,2); //Abajo
        ComLine(xC-8 ,yC-78,xC-8 ,yC-52,2); //Derecha
        ComLine(xC-47,yC-78,xC-47,yC-52,2); //Izquierda
            //Derecha
        ComLine(xC+47,yC-78,xC+8 ,yC-78,2); //Arriba
        ComLine(xC+47,yC-52,xC+8 ,yC-52,2); //Abajo
        ComLine(xC+8 ,yC-78,xC+8 ,yC-52,2); //Derecha
        ComLine(xC+47,yC-78,xC+47,yC-52,2); //Izquierda
            //Centro
        ComLine(xC-7 ,yC-65,xC+7 ,yC-65,3);
    }


    // Dibujamos manos
    ComCirculo(x+80, y+225, 20); //Izquierda
    ComCirculo(x-80, y+225, 20); //Derecha

}

void despiertaMrP(){

    //Esta es la funcion de demostracion del microfono
    //Su funcionamiento es simple; llevaremos la cuenta de un temporizador (a traves de una variable estatica), y en base a eso cambiaremos estados
        // -1s a 0s -> Sorprendido
        // 0s a 2s -> Contento
        // 2s a 3s -> Dormido estatico
        // 3s+ -> Dormido con animacion

    //1. Temporizador
    static int temp=0; //Declaramos el temporizador, y lo inicializamos a 0
    if(temp<=200){ //El programa se deberia ejecutar cado 20ms, 3s = 150ms. Contamos otros 50ms por si acaso, pero saturamos para no tener que usar long
        temp++; //Comienza la
    }

    //2. Escuchamos la salida del microfono, si microfono ON, temporizador a shock
    if(MICROFONO_ON){
        temp=-50; // -50*20ms = -1s
    }

    //3. Dibujamos estancia
    ComColor(64, 64, 64);
    ComRect(55,65,HSIZE-55,VSIZE-22,true); //Fondo

    if(temp<100){ //Entre -1s y 2s, habitacion iluminada
        ComColor(247, 228, 146);
        ComRect(60,70,HSIZE-60,VSIZE-27,true);
        //Toda la lampara se tiene que dibujar aqui porque si no se la come el fondo
        ComColor(190, 190, 204);
        ComRect(HSIZE/2+138,VSIZE/2-10,HSIZE/2+158,VSIZE/2+10,true); //Base lampara
        ComColor(153, 153, 173);
        ComLine(HSIZE/2+148,VSIZE/2,HSIZE/2+148,VSIZE/2-20,3); //Palo lampara
        ComColor(90, 224, 171); // Turquesa
        ComRect(HSIZE/2+125,VSIZE/2-50,HSIZE/2+170, VSIZE/2-20,true); //Lampara
    } else { //Si no, habitacion "apagada"
        ComColor(106, 113, 125);
        ComRect(60,70,HSIZE-60,VSIZE-27,true); //Pared
        //Toda la lampara se tiene que dibujar aqui porque si no se la come el fondo
        ComColor(190, 190, 204);
        ComRect(HSIZE/2+138,VSIZE/2-10,HSIZE/2+158,VSIZE/2+10,true); //Base lampara
        ComColor(153, 153, 173);
        ComLine(HSIZE/2+148,VSIZE/2,HSIZE/2+148,VSIZE/2-20,3); //Palo lampara
        ComColor(75, 102, 91); // Turquesa
        ComRect(HSIZE/2+125,VSIZE/2-50,HSIZE/2+170, VSIZE/2-20,true); //Lampara
    }
    //La cama siempre permanece igual
    ComColor(102, 94, 75); //Marron grisaceo
    ComRect(HSIZE/2-100,90,HSIZE/2+100,VSIZE/2+20,true); //Reposacabezas
    ComColor(220,220,210);
    ComRect(HSIZE/2-60,VSIZE/2,HSIZE/2+60,VSIZE/2+30,true); //Almohada

    //4. Dibujamos a MrP
    if(temp<0){ //Shock - -1 a 0
        dibujarMrP(HSIZE/2,40,1,0,0,0); //Mr P en shock, sin gafas, sin cuerpo, sin animacion
    } else if(temp<100) { //Contento - 0 a 2
        dibujarMrP(HSIZE/2,40,0,0,0,0); //Mr P contento, sin gafas, sin cuerpo, sin animacion
    } else if (temp<150) { //Dormido - 2 a 3
        dibujarMrP(HSIZE/2,40,2,0,0,0); //Mr P dormido, sin gafas, sin cuerpo, sin animacion
    } else { //Dormido animado
        dibujarMrP(HSIZE/2,40,2,0,0,6); //Mr P dormido, sin gafas, sin cuerpo, con animacion de dormir
    }

    //Dibujamos las sabanas, para que MrP se pueda meter debajo
    ComColor(200,50,50);
    ComRect(HSIZE/2-100,VSIZE/2+20,HSIZE/2+100,VSIZE-30,true); //Sabanas
    //ComColor(180,30,30);
    //ComTXT(HSIZE/2, VSIZE/2+40, 31, OPT_CENTERX, "Aplaude!");
}

