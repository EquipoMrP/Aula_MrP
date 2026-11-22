#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include <math.h>

// V.Pantalla
const int32_t REG_CAL[6]= {CAL_DEFAULTS}; // Calibración
#define TAG(x)       ((3UL << 24) | ((x) & 0xFF)) // Detección widget tocado
int Load;   // Comprobar el % de carga del ciclo de trabajo

// V. Reloj
int RELOJ;      // Variable que guarda la frecuencia del programa
#define FREC 50 // Frecuencia en hercios del Timer0 (refresco de pantalla): 20 ms

// Interrupcion falsa
volatile int Flag_ints=0;
void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints=0;
}

//#define SLEEP SleepReal()
#define SLEEP SysCtlSleepFake()
void IntTimer(void);


//Variables especificas de MrP
uint32_t colorRopaMrP=9868950;  // 1001 0110, 1001 0110, 1001 0110 = 150 150 150 -> Gris default
#define mascaraRojo 16711680    // 1111 1111, 0000 0000, 0000 0000 = 255   0   0
#define mascaraVerde 65280      // 0000 0000, 1111 1111, 0000 0000 =   0 255   0
#define mascaraAzul 255         // 0000 0000, 0000 0000, 1111 1111 =   0   0 255


void demoJoystick(void);
void dibujarFondo(int id);
void dibujarMrP(int x, int y, char idEmocion, char gafas, char cuerpo, char idAnimacion);
static int Oscurece(int c);

//// VAIRABLES ESPECIFICAS DEL JOYSTICK
// Ejes (potenciometros): PK2 = AIN18 (X), PK3 = AIN19 (Y). Boton: PP4
#ifndef BASE_BUTJOYSTICK
#define BASE_BUTJOYSTICK GPIO_PORTP_BASE
#endif
#ifndef PIN_BUTJOYSTICK
#define PIN_BUTJOYSTICK  GPIO_PIN_4
#endif
// El boton es activo a nivel bajo (pull-up interno): pulsado -> el pin lee 0
#define BUTJOYSTICK_ON (GPIOPinRead(BASE_BUTJOYSTICK, PIN_BUTJOYSTICK)==0)
#define ADC_MAX 4095    // Fondo de escala del ADC (12 bits)

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

    ///////////////////////
    //Apartado para inicializar el joystick
    //////////////////////
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);    // PK2 (X) y PK3 (Y)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);    // PP4 (boton)
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0) ||
          !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK) ||
          !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOP));

    // Pines: PK2/PK3 analogicos, PP4 entrada digital con pull-up
    GPIOPinTypeADC(GPIO_PORTK_BASE, GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinTypeGPIOInput(BASE_BUTJOYSTICK, PIN_BUTJOYSTICK);
    GPIOPadConfigSet(BASE_BUTJOYSTICK, PIN_BUTJOYSTICK, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // ADC0: reloj de 32 MHz (VCO 480 MHz / 15). Secuenciador 2, disparo por software, 2 pasos
    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL, 15);
    ADCHardwareOversampleConfigure(ADC0_BASE, 8);   // Promedio x8 por hardware (filtra ruido)
    ADCSequenceConfigure(ADC0_BASE, 2, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 2, 0, ADC_CTL_CH18);                            // PK2 -> eje X
    ADCSequenceStepConfigure(ADC0_BASE, 2, 1, ADC_CTL_CH19 | ADC_CTL_IE | ADC_CTL_END); // PK3 -> eje Y
    ADCSequenceEnable(ADC0_BASE, 2);
    ADCIntClear(ADC0_BASE, 2);

    // Configuracion modos bajo consumo
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralClockGating(true);

    // Pantalla y configuración pantalla inicio
        SysCtlDelay(RELOJ/3);
        HAL_Init_SPI(2, RELOJ);  //Boosterpack a usar, Velocidad del MC
        Inicia_pantalla();       //Arranque de la pantalla
        VolNota(127);

        Nueva_pantalla(16,16,16);
        ComColor(100, 100, 0);
        ComLineWidth(5);
        ComRect(5, 5, HSIZE-5, VSIZE-5, true);
        ComColor(0, 134, 124);
        ComRect(12, 12, HSIZE-12, VSIZE-12, true);

        ComColor(255,255,255);
        ComTXT(HSIZE/2,10+VSIZE/5, 31, OPT_CENTERX,"DEMO");
        ComTXT(HSIZE/2,60+VSIZE/5, 31, OPT_CENTERX,"Joystick");
        ComTXT(HSIZE/2,120+VSIZE/5, 28, OPT_CENTERX,"SEPA GIERM. 2026");


        Dibuja();
        Espera_pant();


        // Calibración pantalla táctil
        int i;
        for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);

    Load = 0;

    while(1){
        SLEEP;

        Nueva_pantalla(16, 16, 16);
        dibujarFondo(1);
        demoJoystick();

        Dibuja();

    }
}




//Funcion de demostracion del joystick
void demoJoystick(void){
    //Para este minijuego tendremos a Mr P corriendo de un lado a otro como lo del DVD
        //El usuario manejara una cruceta que debera apuntar encima de MrP
        //Al pulsarle al boton del joystick:
            //Si acierta a Mr P, sonidito alegre y Mr P sorprendido
            //Si falla, Mr P desafiante y sonidito no alegre
            //Breve cooldown para que se pueda ver
            //Cruzeta de gris durante un poco mas para que MrP pueda huir
            //MrP cambia de velocidad de manera aleatoria
    
    //Variables
    static int maqEstado=0;
    static int tempJoystick=0;

    static int MrPx=HSIZE/2;
    static int MrPy=VSIZE/2;
    static int MrPvx=5; //La velocidad horizontal de MrP (pixel/tick)
    static int MrPvy=1; //Velocidad vertical de MrP (pixel/tick)
    
    static int MiraX=HSIZE/2;
    static int MiraY=VSIZE/2;

    switch(maqEstado){
    case 0: //1. Mr P correteando

        //1.1 MrP
        dibujarMrP(MrPx,MrPy,0,1,0,0);
        //Cambio de posicion en X
        if( (MrPx<(HSIZE-60) && MrPvx>0) || (MrPx>60 && MrPvx<0) ){
            MrPx += MrPvx;
        } else {
            MrPvx = -MrPvx;
        }
        //Cambio de posicion en Y
        if( (MrPy<=VSIZE-120 && MrPvy>0) || (MrPy>5 && MrPvy<=0) ){
            MrPy += MrPvy;
        } else {
            MrPvy = -MrPvy;
        }

        //1.2 Cambio de estado segun la posicion
        if(BUTJOYSTICK_ON){
            tempJoystick++; //Debouncing
            if(tempJoystick>3){
                if(abs(MiraX-MrPx)<70 && abs(MiraY-MrPy)<120){ //Acierto
                    //Cambio a estado acierto
                    maqEstado=1;
                } else{ //Fallo
                    //Cambio a estado fallo
                    maqEstado=2;
                }
                //Sea como sea, nueva velocidad aleatoria
                MrPvx = rand()%10+1;
                MrPvy = rand()%6+1;
                tempJoystick=0;
            }
        }
        break;

    //2. Resultado
    case 1: //2.1 Acierto
        //2.1.1 MrP
        dibujarMrP(MrPx,MrPy,1,1,0,1); //En shock

        //2.1.2 Cambio de estado segun el tiempo
        tempJoystick++;
        if(tempJoystick>20){ //Solo 1s aqui
            maqEstado=3;
            tempJoystick=0;
        }
        break;

    case 2:  //2.2 Fallo
        //2.2.1 MrP
        dibujarMrP(MrPx,MrPy,5,1,0,1); //Desafiante

        //2.2.2 Cambio de estado segun el tiempo
        tempJoystick++;
        if(tempJoystick>20){ //Solo 1s aqui
            maqEstado=3;
            tempJoystick=0;
        }
        break;

    case 3: //3. Cooldown
        //3.1 MrP
        dibujarMrP(MrPx,MrPy,0,1,0,0);
        //Cambio de posicion en X
        if( (MrPx<(HSIZE-60) && MrPvx>0) || (MrPx>60 && MrPvx<0) ){
            MrPx += MrPvx;
        } else {
            MrPvx = -MrPvx;
        }
        //Cambio de posicion en Y
        if( (MrPy<=VSIZE-120 && MrPvy>0) || (MrPy>5 && MrPvy<=0) ){
            MrPy += MrPvy;
        } else {
            MrPvy = -MrPvy;
        }

        //3.2 Cambio de estado segun el tiempo
        tempJoystick++;
        if(tempJoystick>20){ //Solo 1s de cooldown
            maqEstado=0;
        }
        break;
    }

    // 4. Dibujamos la mira
    if(maqEstado==0){ ComColor(200,0,0);  /* Activa */} else { ComColor(80,80,80); /* En cooldown */}
    ComLine(MiraX,MiraY+10,MiraX,MiraY+20,2);
    ComLine(MiraX,MiraY-10,MiraX,MiraY-20,2);
    ComLine(MiraX+10,MiraY,MiraX+20,MiraY,2);
    ComLine(MiraX-10,MiraY,MiraX-20,MiraY,2);
    ComCirculo(MiraX,MiraY,5);

    // 5. Lectura del joystick para reposicionar la mira
    uint32_t joy[4];                            // El FIFO del SS2 tiene 4 huecos
    ADCProcessorTrigger(ADC0_BASE, 2);          // Lanza la conversion de PK2 y PK3
    while(!ADCIntStatus(ADC0_BASE, 2, false));  // Espera a que termine (unos microsegundos)
    ADCIntClear(ADC0_BASE, 2);
    ADCSequenceDataGet(ADC0_BASE, 2, joy);      // joy[0] = PK2 (X), joy[1] = PK3 (Y)

    // Mapeo directo: 0..4095 -> pantalla. Centro del joystick (~2048) = centro de pantalla
        //Nota: Esto da mucha velocidad, asi que la partimos entre 3
    MiraX = (int)((joy[0] * HSIZE) / ADC_MAX)/3;
    MiraY = VSIZE - (int)((joy[1] * VSIZE) / ADC_MAX)/3;  // Y invertido (en pantalla Y crece hacia abajo)
}

// Oscurece un canal sin que dé la vuelta por debajo de 0
static int Oscurece(int c)
{
    return (c > 10) ? (c - 10) : 0;
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

    static int temp=0;    //Temporizado

    static int xML=0;       //Posicion mano izquierda
    static int yML=0;
    static int xMR=0;       // ''       ''  derecha
    static int yMR=0;
    static float ang=0;     //Control del angulo de las manos

    static char estadoAnim=0;    //Para gestionar las animaciones
    static char idAnim_p=0;      //Para poder transicionar correctamente entre animaciones

    if(idAnim_p!=idAnimacion){
        estadoAnim=0;
        idAnim_p=idAnimacion;
    }

    temp++;
    if(temp>200){ //Saturacion del temporizador, de forma que no fastidie los movimientos circulares
        temp=0;
    }

    //Animacion (se hace ahora porque toca el resto de variables)
    switch (idAnimacion){

    default: //Default es el case 0, sin Animacion
        xC=x;
        yC=y+135;
        xML=x-80; yML=y+225;
        xMR=x+80; yMR=y+225;

        break;

    case 1: //Idle normal, cabeza sube y baja, pero poco
        switch(estadoAnim){
        case 0:
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;

        case 1: // CORREGIDO: el cambio de estado solo ocurre al llegar al limite
            if (yC>y+132){
                if(temp%5==0) yC--;
            } else {  estadoAnim=2;   }
            break;

        case 2:
            if (yC<y+139){
                if(temp%5==0) yC++;
            } else { estadoAnim=1; }
            break;

        }
        break;

    case 2: //Emocionado

        switch (estadoAnim){
        case 0: //Inicio, llegar al "reposo"
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;
        case 1:
            if(yML>y+100){
                yML-=5;
                yMR-=5;
            } else {
                estadoAnim=2;
            }
            break;

        case 2:
            if(yML>y+100){
                yML--;
                yMR--;
            } else {
                estadoAnim=3;
            }
            break;

        case 3:
            if(yML<y+120){
                yML++;
                yMR++;
            } else {
                estadoAnim=2;
            }
            break;
        }
        break;

    case 3: //Hablando

        switch (estadoAnim){
        case 0: //Inicio, llegar al "reposo"
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;
        case 1:
            if(yML>y+150){
                yML-=5;
                yMR-=5;
            } else {
                estadoAnim=2;
            }
            break;

        case 2:
            if(yML>y+150){
                yML--;
                yMR--;
            } else {
                estadoAnim=3;
            }
            break;

        case 3:
            if(yML<y+170){
                yML++;
                yMR++;
            } else {
                estadoAnim=2;
            }
            break;
        }
        break;

    case 4: //Risa
        switch(estadoAnim){
        case 0:
            xC=x;
            yC=y+135;
            xML=x-15; yML=yC-15;
            xMR=x+15; yMR=yC-15;
            estadoAnim=1;
            break;

        case 1: // CORREGIDO: igual que en la animacion 1
            if (yC>y+132){
                if(temp%3==0){
                    yC--;
                    yML=yC-15;
                    yMR=yML;
                }
            } else {  estadoAnim=2;   }
            break;

        case 2:
            if (yC<y+139){
                if(temp%3==0){
                    yC++;
                    yML=yC-15;
                    yMR=yML;
                }
            } else { estadoAnim=1; }
            break;

        }
        break;

    case 5: //Palmada

        switch (estadoAnim){
        case 0: //Inicio, llegar al "reposo"
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;
        case 1:
            if(yML>190){
                yML-=5;
                yMR-=5;
            } else {
                estadoAnim=2;
            }
            break;
        case 2:
            if(xML<x-10){
                xML+=3;
            }

            if(xMR>x+10){
                xMR-=3;
            } else {
                estadoAnim=3;
            }
            break;

        case 3:
            if(xMR<x+50){
                xMR+=3;
            }

            if(xML>x-50){
                xML-=3;
            } else {
                estadoAnim=2;
            }
            break;

        }
        break;

    case 6: //Dormir

        switch (estadoAnim){
        case 0: //Inicio, llegar al "reposo"
            xC=x;
            yC=y+135;
            xML=x-60; yML=y+205;
            xMR=x+60; yMR=y+205;
            estadoAnim=1;
            break;

        case 1: //Moverse hacia abajo y las manos a la barbilla
            if(yC<y+155){    yC++;  }
            if(xML<x-12){ xML+=4;  }
            if(xMR>x+12){  xMR-=4;  }
            if(yML>y+160){  yML-=4;  }
            yMR=yML;

            break;
        }

        break;

    case 7: //Pensando
        switch(estadoAnim){
        case 0:
            xC=x;
            yC=y+135;
            xML=x-30; yML=yC-20;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;

        case 1: // CORREGIDO: igual que en la animacion 1
            if (xC>x-5){
                if(temp%3==0){
                    xC--;
                    xML=xC-30;
                }
            } else {  estadoAnim=2;   }
            break;

        case 2:
            if (xC<x+5){
                if(temp%3==0){
                    xC++;
                    xML=xC-30;
                }
            } else { estadoAnim=1; }
            break;

        }
        break;

    case 8: //Saludando
        switch(estadoAnim){
        case 0:
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;

        case 1:
            if (ang>0){
                ang-=0.04;
                xML=xC-100.*cos(ang);
                yML=yC-100.*sin(ang);
            } else {  estadoAnim=2;   }
            break;

        case 2:
            if (ang<M_PI/2){
                ang+=0.04;
                xML=xC-100.*cos(ang);
                yML=yC-100.*sin(ang);
            } else { estadoAnim=1; }
            break;

        }
        break;

    case 9: //Movimiento circulero
        switch(estadoAnim){
        case 0:
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;
        case 1:
            xC=x+10.*cos((float)temp/10./M_PI);
            yC=y+135+10.*sin((float)temp/10./M_PI);
            break;
        }
        break;
    }

    //Si el cuerpo es visible, lo dibujamos
    if(cuerpo){
        //Dibujamos la "camisa"
        ComColor((colorRopaMrP & mascaraRojo)>>16,(colorRopaMrP & mascaraVerde)>>8,(colorRopaMrP & mascaraAzul)); //Color cami
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
            ComCirculo(xC-22,yC-61,6); // Down L
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+22,yC-68,3); // Up R
            ComCirculo(xC+22,yC-61,3); // Down R
            ComCirculo(xC-22,yC-68,3); // Up L
            ComCirculo(xC-22,yC-61,3); // Down L
            ComLine(xC-26,yC-66,xC-26,yC-64,4); //Recorte L
            ComLine(xC+18,yC-66,xC+18,yC-64,4); //Recorte R
        }

        // Boca
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-38,13);
            ComColor(255,181,144);//Color piel
            ComLine(xC-13,yC-40,xC+13,yC-40,8);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
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
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,7);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
        break;

    case 2: //Dormido
        //Ojos (En dormido no dependen de si gafas o no) (Aun asi, sin gafas se dibujan mas cerca entre si)
        if(gafas){
            ComColor(0,0,0); //Negro
            ComCirculo(xC+27,yC-74,12);
            ComCirculo(xC-27,yC-74,12);
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+27,yC-74,10);
            ComCirculo(xC-27,yC-74,10);
        } else {
            ComColor(0,0,0); //Negro
            ComCirculo(xC+24,yC-74,12);
            ComCirculo(xC-24,yC-74,12);
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+24,yC-74,10);
            ComCirculo(xC-24,yC-74,10);
        }
        ComLine(xC-32,yC-80,xC+32,yC-80,11);

        //Boca
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-38,13);
            ComColor(255,181,144);//Color piel
            ComLine(xC-13,yC-40,xC+13,yC-40,8);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
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
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-20,12);
            ComColor(255,181,144);//Color piel
            ComLine(xC-7,yC-17,xC+7,yC-17,9);
            ComLine(xC,yC-16,xC,yC-16,9);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
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
            ComCirculo(xC-22,yC-61,6); // Down L
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+22,yC-68,3); // Up R
            ComCirculo(xC+22,yC-61,3); // Down R
            ComCirculo(xC-22,yC-68,3); // Up L
            ComCirculo(xC-22,yC-61,3); // Down L
            ComLine(xC-26,yC-66,xC-26,yC-64,4); //Recorte L
            ComLine(xC+18,yC-66,xC+18,yC-64,4); //Recorte R
            ComColor(0,0,0);
            ComLine(xC-17,yC-74,xC-28,yC-76,2); //Ceja L
            ComLine(xC+17,yC-74,xC+28,yC-76,2); //Ceja R
        }

        //Boca
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-20,12);
            ComColor(255,181,144);//Color piel
            ComLine(xC-7,yC-17,xC+7,yC-17,9);
            ComLine(xC,yC-16,xC,yC-16,9);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
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
            ComCirculo(xC-22,yC-61,6); // Down L
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+22,yC-68,3); // Up R
            ComCirculo(xC+22,yC-61,3); // Down R
            ComCirculo(xC-22,yC-68,3); // Up L
            ComCirculo(xC-22,yC-61,3); // Down L
            ComLine(xC-26,yC-66,xC-26,yC-64,4); //Recorte L
            ComLine(xC+18,yC-66,xC+18,yC-64,4); //Recorte R
            ComColor(0,0,0);
            ComLine(xC-17,yC-74,xC-28,yC-76,2); //Ceja L
            ComLine(xC+17,yC-74,xC+28,yC-76,2); //Ceja R
        }

        //Boca
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-38,13);
            ComColor(255,181,144);//Color piel
            ComLine(xC-13,yC-40,xC+13,yC-40,8);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
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

    if(cuerpo){
        // Dibujamos manos (color de la camisa mas oscuro, sin pasar por debajo de 0)
        ComColor(Oscurece((colorRopaMrP & mascaraRojo)>>16),
                 Oscurece((colorRopaMrP & mascaraVerde)>>8),
                 Oscurece(colorRopaMrP & mascaraAzul));
        ComCirculo(xML, yML, 20); //Izquierda
        ComCirculo(xMR, yMR, 20); //Derecha
    }
}



// Interrupcion
void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20); //Retraso necesario. Mirar Driverlib p.550
    Flag_ints++;
}
