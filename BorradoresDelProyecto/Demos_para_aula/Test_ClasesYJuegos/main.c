/*
 * main.c
 */


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>


#include "driverlib2.h"
#include "utils/uartstdio.h"

#include "FT800_TIVA.h"
#include "HAL_I2C.h"
#include "sensorlib2.h"
#include "touch.h"
#include "tsop1838.h"
#include "teclado.h"
#include "hcsr04.h"
#include "ds3231.h"

#include <MrpDemosLib.h>
#include <MrpEscenariosLib.h>

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

/////////////// VARIABLES FUNCIONAMIENTO ////////////////////
//Gestion de la maquina de estados global
volatile int pantalla=0;
#define nClases 10
volatile int claseSeleccionada;
volatile int clasesDisponibles;
volatile int page=0;

//Gestion sub maquinas de estado
volatile int subPantalla=0;

//Control de los botones
#define clasesPorPagina 2
volatile int page_s=0;
volatile int pantalla_s=0;
volatile int subPantalla_s=0;
volatile int botonPresionado=0;

//Dibujar por pantalla;
char buffer[100];

//Dibujar lecciones
#define tamLineaClase 49
#define numLineaClase 7 //Se escribe una linea mas que la especificada porque se indexa en 0
#define numLineaIntro 2
#define numLineaFin 4
#define txtSizeIntro 200
#define txtSizeExpl 3700
#define txtSizeCaract 1700
#define txtSizeUso 600
#define maxPag 15
volatile int contLetra=0;
volatile int contLinea=0;
volatile int contString=0;
volatile int contString_page[maxPag]={0,0,0,0,0};
volatile int escribiendo=0; // Monitoriza si estamos escribiendo texto

//volatile char dummy[] = "En un lugar de la mancha de cuyo nombre no quiero acordarme, vivia un hidalgo de aquellos de lanza en astillero, rocin flaco y galgo corredor.\nVa una olla de algo mas vaca que carnero, salpicon las mas noches, duelos y quebrantos los sabado, lantejas viernes, algun palomino de anadidura los domingos, consumian las tres partes de su hacienda. El resto concluian sayo de velarte, calcas de velludo para las fiestas, con sus pantunflos de lo mesmo, y los dias entre semana se honraua con su vellori de lo mas fino.\n\n  Tenia en su casa una ama que pasaba de los cuarenta, y una sobrina que no llegaba a los veinte, y unn mozo de campo y plaza, que asi ensillaua el rozin como tomaua la podadera. Frisaba la edad nuestro hidalgo con los cincuenta anos. Era de complexion rezia, seco de carnes, enjunto de rostro, gran madrugador y amigo de la caza. Quieren decir que tenia el sobrenombre de Quixadam o Quesada, que en esto hay alguna diferencia en los autores que deste caso escriuen, aunque por conjeturas verosimiles se dexa entender que se llamaua Quexada. Pero esto importa poco a nuestro cuento; basta con que en la narracion del no se salga un punto de la verdad";
//volatile char dummy2[] = "Sphinx of black quartz, judge my vow.\n...\nThe five boxing wizards jump quickly.\n...\nPack my box with five dozen liquor jugs.";

// JUEGO
#define nJuegos 3
#define maxVidas 3
volatile char juego=0; //Juego en ejecuccion
volatile char juego_p=0; //Juego anterior, para el generador de numeros aleatorios.
volatile char vidas=maxVidas;       //Contar las vidas
volatile int  pts=0;                //Contar los puntos acumulados
volatile int tempJuego=0;           //Temporizador para los minjuegos, sirve tambien para el generador de numero aleatorio
volatile int resultJuego=0;        //Mientras siga a 0, el juego esta en ejecucion

///////////// DATOS PARA LAS LECCIONES -- MOVER A LIBRERIA APARTE??? ---
struct DatosLeccion{
    char nombre[25];
    char textoIntroduccion[txtSizeIntro];
    char textoExplicacion[txtSizeExpl];
    char textoCaracteristicas[txtSizeCaract];
    char textoUso[txtSizeUso];
    void (*demo)(void);
};

struct DatosMinijuego{
    char nombre[25];
    int (*minijuego)(void);
};

struct DatosLeccion Clases[nClases];
struct DatosMinijuego Minijuegos[nJuegos];

//Funciones de lecciones
void dummyDemo(); //Funcion predeterminada para las clases
void TecladoMusical(); //Demo teclado

int dummyJuego(void);
int demoJuego1(void);
int demoJuego2(void);
int demoJuego3(void);

int main(void) {


    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    // LEDs integrados
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 |GPIO_PIN_4); //F0 y F4: salidas
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1); //N0 y N1: salidas

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

    //Perifericos UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, RELOJ);
    //Configuracion de los sensores
    Conf_Boosterpack(1, RELOJ);

    TOUCH_Init(MS_TICK);
    IR_Init(RELOJ);
    SetupSensorBoosterpack();
    HCSR04_Init((uint32_t)RELOJ);
    HCSR04_Disparar();
    TECLADO_Init(MS_TICK);
    SysCtlPeripheralEnable(SOUND_PERIPH);
    GPIOPinTypeGPIOInput(SOUND_BASE, SOUND_PIN);   //E3: entrada
    GPIOPadConfigSet(SOUND_BASE,SOUND_PIN,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU); //Pullup en E3
    // RTC: I2C2 hardware y pines PN5/PN4 (100 kHz)
    DS3231_Init((uint32_t)RELOJ);
    //////// Joystick ---------------------------------
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
    //////////// -----------------------------------------------------------------------------------------

    //Asignar valores a los struct (no se puede hacer fuera)
        //Limpiamos todos los struct por si acaso
    int i;
    struct DatosLeccion defaultStruct = {"\0","\0","\0","\0","\0",&dummyDemo};
    //struct DatosMinijuego defaultJuego = {"\0",&dummyJuego};

    for(i=0;i<nClases;i++){
        Clases[i]=defaultStruct;
    }

        //TM4C 1294
    strcpy(Clases[0].nombre,nombreMC);
    strcpy(Clases[0].textoIntroduccion,introduccionMC);
    strcpy(Clases[0].textoExplicacion,caracteristicasMC);
    strcpy(Clases[0].textoCaracteristicas,ventajasMC);
    strcpy(Clases[0].textoUso,usosScreen);
    Clases[0].demo = &Dibuja_TM4C;

        //Pantalla resistiva
    strcpy(Clases[1].nombre,nombreScreen);
    strcpy(Clases[1].textoIntroduccion,introduccionScreen);
    strcpy(Clases[1].textoExplicacion,caracteristicasScreen);
    strcpy(Clases[1].textoCaracteristicas,ventajasScreen);
    strcpy(Clases[1].textoUso,usosScreen);
    Clases[1].demo = &TecladoMusical;

        //Boosterpack de sensores
    strcpy(Clases[2].nombre,nombreSensors);
    strcpy(Clases[2].textoIntroduccion,introduccionSensors);
    strcpy(Clases[2].textoExplicacion,caracteristicasSensors);
    strcpy(Clases[2].textoCaracteristicas,ventajasSensors);
    strcpy(Clases[2].textoUso,usosSensors);
    Clases[2].demo = &SensoresBoosterpack;

        //Joystick
    strcpy(Clases[3].nombre,nombreJoystick);
    strcpy(Clases[3].textoIntroduccion,introduccionJoystick);
    strcpy(Clases[3].textoExplicacion,caracteristicasJoystick);
    strcpy(Clases[3].textoCaracteristicas,ventajasJoystick);
    strcpy(Clases[3].textoUso,usosTeclado);
    Clases[3].demo = &JoystickMrP;

        //Teclado matricial
    strcpy(Clases[4].nombre,nombreTeclado);
    strcpy(Clases[4].textoIntroduccion,introduccionTeclado);
    strcpy(Clases[4].textoExplicacion,caracteristicasTeclado);
    strcpy(Clases[4].textoCaracteristicas,ventajasTeclado);
    strcpy(Clases[4].textoUso,usosTeclado);
    Clases[4].demo = &Telefono;

        //Sensor capacitivo
    strcpy(Clases[5].nombre,nombreTouch);
    strcpy(Clases[5].textoIntroduccion,introduccionTouch);
    strcpy(Clases[5].textoExplicacion,caracteristicasTouch);
    strcpy(Clases[5].textoCaracteristicas,ventajasTouch);
    strcpy(Clases[5].textoUso,usosTouch);
    Clases[5].demo = &NotasAleatorias;

        //Sensor de sonido
    strcpy(Clases[6].nombre,nombreSound);
    strcpy(Clases[6].textoIntroduccion,introduccionSound);
    strcpy(Clases[6].textoExplicacion,caracteristicasSound);
    strcpy(Clases[6].textoCaracteristicas,ventajasSound);
    strcpy(Clases[6].textoUso,usosSound);
    Clases[6].demo = &despiertaMrP;

        //RTC
    strcpy(Clases[7].nombre,nombreRTC);
    strcpy(Clases[7].textoIntroduccion,introduccionRTC);
    strcpy(Clases[7].textoExplicacion,caracteristicasRTC);
    strcpy(Clases[7].textoCaracteristicas,ventajasRTC);
    strcpy(Clases[7].textoUso,usosRTC);
    Clases[7].demo = &ProgramaReloj;

        //Sensor de infrarrojos
    strcpy(Clases[8].nombre,nombreIR);
    strcpy(Clases[8].textoIntroduccion,introduccionIR);
    strcpy(Clases[8].textoExplicacion,caracteristicasIR);
    strcpy(Clases[8].textoCaracteristicas,ventajasIR);
    strcpy(Clases[8].textoUso,usosIR);
    Clases[8].demo = &sensorIRyMrP;

        //Sensor de ultrasonido
    strcpy(Clases[9].nombre,nombreUltrasonido);
    strcpy(Clases[9].textoIntroduccion,introduccionUltrasonido);
    strcpy(Clases[9].textoExplicacion,caracteristicasUltrasonido);
    strcpy(Clases[9].textoCaracteristicas,ventajasUltrasonido);
    strcpy(Clases[9].textoUso,usosUltrasonido);
    Clases[9].demo = &Theremin;



    //Asignar funciones a "minijuegos"
    /*
    for(i=0;i<nClases;i++){
        Minijuegos[i]=defaultJuego;
    }*/
    Minijuegos[0].minijuego = &demoJuego1;
    Minijuegos[1].minijuego = &demoJuego2;
    Minijuegos[2].minijuego = &demoJuego3;

    //UARTprintf("\nComenzamos pantalla\n");
    // PANTALLA
    HAL_Init_SPI(2, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();       //Arranque de la pantalla
    VolNota(127);


    //UARTprintf("\nComenzamos pantalla2\n");
    Nueva_pantalla(16,16,16);
    //Nueva pantalla, borde
    ComColor(255, 255, 0);
    ComLineWidth(5);
    ComRect(10, 10, HSIZE-10, VSIZE-10, true);
    //Nueva pantalla, relleno
    ComColor(0, 0, 0);
    ComRect(12, 12, HSIZE-12, VSIZE-12, true);

    //Texto
    ComColor(0,255,255);
    ComTXT(HSIZE/2,VSIZE/5, 30, OPT_CENTERX,"Aula Mr. P");


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
    int c=0;

    while(1){
        SLEEP;
        Nueva_pantalla(16,16,16);
        ComColor(255,255,255);
        switch(pantalla){
        case 0: //Pantalla de inicio
            //Clases vs pantallas
            ComColor(0,0,0);

            ComFgcolor(0,0,255);
            if(Boton(50,VSIZE/2-50, 100, 100,31, "Clases") && botonPresionado==0){
                pantalla_s=1; //Pantalla de clases
                botonPresionado=1;
            }

            ComFgcolor(255,0,0);
            if(Boton(HSIZE-50-100,VSIZE/2-50, 100, 100, 31, "Juegos") && botonPresionado==0){
                pantalla_s=2; //Pantalla de clases
                botonPresionado=1;
            }
            break;

        case 1:
            //Seleccion de clases

            ComTXT(HSIZE/2,20,31,OPT_CENTERX,"CLASES");

            if( (nClases-(page*clasesPorPagina)) >=clasesPorPagina){
                clasesDisponibles=clasesPorPagina;
            } else if ( (nClases-(page*clasesPorPagina)) >0){ //Proteccion en caso de error
                clasesDisponibles= (nClases-(page*clasesPorPagina)) ;
            }

            ComFgcolor(255,0,0);
            for(c=page*clasesPorPagina;c<clasesDisponibles+page*clasesPorPagina;c++){
                sprintf(buffer,Clases[c].nombre,c);
                if(Boton(15+140*(c-page*clasesPorPagina), VSIZE/2, 125, 50,21, buffer) && botonPresionado==0){
                    pantalla_s=3; //Vamos a clase
                    claseSeleccionada=c;
                    page_s=0;
                    botonPresionado=1;
                }

            }

            //Posible cambio de pagina si se necesita
            //Siguiente pagina si aun quedan juegos
            if( (nClases-(page*clasesPorPagina)) >clasesPorPagina){
                //Si hay mas clases que las que se pueden mostrar, boton de otra pestana
                ComFgcolor(0,255,0);
                if(Boton(HSIZE-75, VSIZE-50, 50, 25, 16, "Siguiente") && botonPresionado==0){
                    page_s=page+1;
                    botonPresionado=1;
                }
            }
            //Pagina anterior si se ha cambiado de pagina
            if (page>0){
                ComFgcolor(0,255,0);
                if(Boton(25, VSIZE-50, 50, 25, 16, "Anterior") && botonPresionado==0){
                    page_s=page-1;
                    if(page_s<0){ //Saturacion pa porsi
                        page_s=0;
                    }
                    botonPresionado=1;
                }
            }

            break;

        case 2:
            //Seleccion de minijuegos
            switch(subPantalla){
            case 0: //LISTOOOOOOS?
                ComTXT(HSIZE/2,20,31,OPT_CENTERX,"JUEGOS");

                tempJuego++; //Contamos el tiempo para poder aleatorizar el juego

                //Boton de atras
                ComColor(0,0,0);
                ComFgcolor(200,0,200);
                if(Boton(HSIZE/2-150, VSIZE/2-50, 100, 100, 21, "Atras") && botonPresionado==0){
                    botonPresionado=1;
                    subPantalla_s=0;
                    pantalla_s=0;
                }

                //Boton de seguir
                ComColor(0,0,0);
                ComFgcolor(260,250,90);
                if(Boton(HSIZE/2+50, VSIZE/2-50, 100, 100, 21, "A jugar!") && botonPresionado==0){
                    botonPresionado=1;
                    if(tempJuego>2){
                        juego=(rand() % nJuegos); //Generamos juego aleatorio
                    } else {
                        juego=juego_p; //Por si tenemos a flash jugando, que no se estropee el sistema
                    }

                    subPantalla_s=1;
                    tempJuego=0;
                }

                //Si vidas!=maxVidas, set vidas to maxVidas
                if(vidas!=maxVidas){ vidas=maxVidas; }
                //Reseteamos puntuacion
                if(pts!=0){ pts=0; }

                break;
            case 1: //Mostramos nombre de juego a jugar
                //Dibujamos vida
                for(c=1;c<=vidas;c++){
                    ComColor(200,100,100);
                    ComCirculo(50,20+30*c,12);
                }

                //Dibujamos puntuacion
                ComNum(HSIZE-50, 20, 16, OPT_RIGHTX, pts);

                //Dibujamos nombre de juego
                sprintf(buffer,"JUEGO %d",juego+1);
                ComTXT(HSIZE/2, 75, 31, OPT_CENTERX, buffer);

                //Temporizacion
                tempJuego++;
                if(tempJuego>=100){ //Segundo y medio de preparacion
                    tempJuego=0;
                    subPantalla_s=2;
                    subPantalla=2;
                    juego_p=juego; //Actualizamos juego anterior
                }
                break;

            case 2: //Juego
                resultJuego=Minijuegos[juego].minijuego();
                if(resultJuego>=1){ //Yay
                    pts++;
                    subPantalla=3;
                    subPantalla_s=3;
                } else if (resultJuego<=-1){ //Nay
                    vidas--;
                    subPantalla=3;
                    subPantalla_s=3;
                }
                break;

            case 3: //Resultado del juego, seguir jugando?

                //Dibujamos fondo de borde rojo si mal, verde si guay
                if(resultJuego>0){
                    ComColor(0,255,0);
                } else {
                    ComColor(255,0,0);
                }
                ComRect(5,5,HSIZE-5,VSIZE-5,true);
                ComColor(0,0,0);
                ComRect(12,12,HSIZE-12,VSIZE-12,true);

                tempJuego++;

                //Dibujamos vida
                for(c=1;c<=vidas;c++){
                    ComColor(200,100,100);
                    ComCirculo(50,20+30*c,12);
                }

                //Dibujamos puntuacion
                ComNum(HSIZE-50, 20, 16, OPT_RIGHTX, pts);tempJuego++; //Contamos el tiempo para poder aleatorizar el juego

                //Boton de atras
                ComColor(0,0,0);
                ComFgcolor(200,0,200);
                if(Boton(HSIZE/2-100, VSIZE/2-50, 100, 100, 21, "Salir") && botonPresionado==0){
                    botonPresionado=1;
                    subPantalla_s=0;
                    pantalla_s=0;
                }

                //Boton de seguir, solo disponible si se tienen vidas
                if(vidas>0){
                    ComColor(0,0,0);
                    ComFgcolor(160,150,90);
                    if(Boton(HSIZE/2+100, VSIZE/2-50, 100, 100, 21, "Una mas!") && botonPresionado==0){
                        botonPresionado=1;
                        while(juego==juego_p){
                            juego=(rand() % nJuegos); //Generamos juego aleatorio
                        }
                        subPantalla_s=1;
                        tempJuego=0;
                    }
                }
            }
            break;

        case 3:
            //Lección
                //Maquina de estados de la explicacion
            switch(subPantalla){
            case 0: //Introduccion ----------------------------------------------
                dibujarFondo(0);
                sprintf(buffer,Clases[claseSeleccionada].nombre);
                ComColor(255,255,255);
                ComTXT(HSIZE/2,30,29,OPT_CENTERX,buffer);

                //Texto
                escribiendo=1;
                contLetra=0;
                contString=0;
                contLinea=0;
                sprintf(buffer,"\0");
                while (escribiendo){

                    buffer[contLetra] = Clases[claseSeleccionada].textoIntroduccion[contString];
                    //UARTprintf("contLetra=%d - ",contLetra);
                    if(buffer[contLetra]=='\0' || contString > txtSizeIntro){
                        //Si caracter terminador, dejar de escribir
                        ComTXT(28,60+contLinea*20,27,0,buffer);
                        escribiendo=0;

                    } else if (buffer[contLetra]=='\n' || contLetra>tamLineaClase){
                        //Si /n o se acaba la linea, salto de linea
                        if (buffer[contLetra]=='\n' ){
                            buffer[contLetra]='\0'; //Por si acaso limpiamos los casos de \n

                        }else if(buffer[contLetra]!=' ' && Clases[claseSeleccionada].textoIntroduccion[contString+1]!=' '){ //En los saltos de linea provocados de forma "natural", ponemos guion
                            buffer[contLetra+1]='-';
                            //UARTprintf("Cambio de linea, -1=%c, 0=%d, 1=%c\n",dummy[contString-1],dummy[contString],dummy[contString+1]);
                        } else {
                            buffer[contLetra+1]='\0'; //Si hemos llegado hasta aqui, es por cambio de linea "natrual", por tanto se debe terminar este string por si acaso
                        }


                        ComTXT(28,60+contLinea*20,27,0,buffer);
                        //UARTprintf("Cambio de linea, contLetra=%d\n",contLetra);
                        contLetra=0;
                        contLinea++;
                        contString++;
                        //UARTprintf("\nSalto de linea ---------------\n\n");
                        sprintf(buffer,"\0"); //Limpiamos el buffer por si acaso
                        if(contLinea>numLineaIntro){
                            //Si nos salimos de la pantalla, cortar
                            escribiendo=0;
                        }

                    } else {
                        //Si no sucede nada, proseguir
                        contLetra++;
                        contString++;
                    }

                }


                // Imagen

                // Extra

                // Boton de cambio de pantalla
                ComFgcolor(0,255,0);
                ComColor(0,0,0);
                if(Boton(HSIZE-75, VSIZE-50, 50, 25, 16, "Siguiente") && botonPresionado==0){
                    subPantalla_s=1;
                    botonPresionado=1;
                }
                break;

            case 1: //Detalles ------------------------------------
                dibujarFondo(0);
                ComColor(255,255,255);
                ComTXT(HSIZE/2,30,29,OPT_CENTERX,"Detalles");

                // Texto
                escribiendo=1;
                contLetra=0;
                contString=contString_page[page];
                contLinea=0;
                sprintf(buffer,"\0");
                while (escribiendo){

                    buffer[contLetra] = Clases[claseSeleccionada].textoExplicacion[contString];
                    //UARTprintf("contLetra=%d - ",contLetra);
                    if(buffer[contLetra]=='\0' || contString > txtSizeExpl){
                        //Si caracter terminador, dejar de escribir
                        ComTXT(28,60+contLinea*20,27,0,buffer);
                        escribiendo=0;

                    } else if (buffer[contLetra]=='\n' || contLetra>tamLineaClase){
                        //Si /n o se acaba la linea, salto de linea
                        if (buffer[contLetra]=='\n' ){
                            buffer[contLetra]='\0'; //Por si acaso limpiamos los casos de \n

                        }else if(buffer[contLetra]!=' ' && Clases[claseSeleccionada].textoExplicacion[contString+1]!=' '){ //En los saltos de linea provocados de forma "natural", ponemos guion
                            buffer[contLetra+1]='-';
                            //UARTprintf("Cambio de linea, -1=%c, 0=%d, 1=%c\n",dummy[contString-1],dummy[contString],dummy[contString+1]);
                        } else {
                            buffer[contLetra+1]='\0'; //Si hemos llegado hasta aqui, es por cambio de linea "natrual", por tanto se debe terminar este string por si acaso
                        }


                        ComTXT(28,60+contLinea*20,27,0,buffer);
                        //UARTprintf("Cambio de linea, contLetra=%d\n",contLetra);
                        contLetra=0;
                        contLinea++;
                        contString++;
                        //UARTprintf("\nSalto de linea ---------------\n\n");
                        sprintf(buffer,"\0"); //Limpiamos el buffer por si acaso
                        if(contLinea>numLineaClase){
                            //Si nos salimos de la pantalla, cortar
                            escribiendo=0;
                        }

                    } else {
                        //Si no sucede nada, proseguir
                        contLetra++;
                        contString++;
                    }

                }

                // Botones de cambio de pantalla
                    //Continuar
                if(Clases[claseSeleccionada].textoExplicacion[contString]!='\0' && contString<txtSizeExpl && page<(maxPag-1) ){
                        //En caso de haber mas texto disponible, continuar a mas texto disponible
                    ComFgcolor(100,255,255);
                    ComColor(0,0,0);
                    if(Boton(HSIZE-75, VSIZE-50, 50, 25, 16, "->") && botonPresionado==0){
                        page_s=page+1;
                        if(page_s>=maxPag){ //Saturacion
                            page_s=maxPag-1;
                        }
                        contString_page[page_s]=contString;
                        botonPresionado=1;
                    }

                } else {
                        //En caso contrario, continuar a la siguiente diapositiva
                    ComFgcolor(0,255,0);
                    ComColor(0,0,0);
                    if(Boton(HSIZE-75, VSIZE-50, 50, 25, 16, "Siguiente") && botonPresionado==0){
                        subPantalla_s=2;
                        page_s=0;
                        botonPresionado=1;
                    }
                }

                    //Retroceso
                if(page>0){
                    //En caso de no estar en la primera pagina, dar la opcion a retroceder al texto anterior
                    ComFgcolor(100,255,255);
                    ComColor(0,0,0);
                    if(Boton(25, VSIZE-50, 50, 25, 16, "<-") && botonPresionado==0){
                        page_s=page-1;
                        if(page_s<0){ //Saturacion
                            page_s=0;
                        }
                        botonPresionado=1;
                    }

                } else {
                    //Si estamos en la primera pagina, retroceder de diapositiva
                    ComFgcolor(0,255,0);
                    ComColor(0,0,0);
                    if(Boton(25, VSIZE-50, 50, 25, 16, "Anterior") && botonPresionado==0){
                        subPantalla_s=0;
                        page_s=0;
                        botonPresionado=1;
                    }
                }
                break;

            case 2: //Caracteristicas (Ventajas/Desventajas) ----------------------------------------
                dibujarFondo(0);
                ComColor(255,255,255);
                ComTXT(HSIZE/2,30,29,OPT_CENTERX,"Caracteristicas");

                // Texto
                escribiendo=1;
                contLetra=0;
                contString=contString_page[page];
                contLinea=0;
                sprintf(buffer,"\0");
                while (escribiendo){

                    buffer[contLetra] = Clases[claseSeleccionada].textoCaracteristicas[contString];
                    //UARTprintf("contLetra=%d - ",contLetra);
                    if(buffer[contLetra]=='\0' || contString > txtSizeCaract){
                        //Si caracter terminador, dejar de escribir
                        ComTXT(28,60+contLinea*20,27,0,buffer);
                        escribiendo=0;

                    } else if (buffer[contLetra]=='\n' || contLetra>tamLineaClase){
                        //Si /n o se acaba la linea, salto de linea
                        if (buffer[contLetra]=='\n' ){
                            buffer[contLetra]='\0'; //Por si acaso limpiamos los casos de \n

                        }else if(buffer[contLetra]!=' ' && Clases[claseSeleccionada].textoCaracteristicas[contString+1]!=' '){ //En los saltos de linea provocados de forma "natural", ponemos guion
                            buffer[contLetra+1]='-';
                            //UARTprintf("Cambio de linea, -1=%c, 0=%d, 1=%c\n",dummy[contString-1],dummy[contString],dummy[contString+1]);
                        } else {
                            buffer[contLetra+1]='\0'; //Si hemos llegado hasta aqui, es por cambio de linea "natrual", por tanto se debe terminar este string por si acaso
                        }


                        ComTXT(28,60+contLinea*20,27,0,buffer);
                        //UARTprintf("Cambio de linea, contLetra=%d\n",contLetra);
                        contLetra=0;
                        contLinea++;
                        contString++;
                        //UARTprintf("\nSalto de linea ---------------\n\n");
                        sprintf(buffer,"\0"); //Limpiamos el buffer por si acaso
                        if(contLinea>numLineaClase){
                            //Si nos salimos de la pantalla, cortar
                            escribiendo=0;
                        }

                    } else {
                        //Si no sucede nada, proseguir
                        contLetra++;
                        contString++;
                    }

                }

                // Botones de cambio de pantalla
                    //Continuar
                if(Clases[claseSeleccionada].textoCaracteristicas[contString]!='\0' && contString<txtSizeCaract && page<(maxPag-1) ){
                        //En caso de haber mas texto disponible, continuar a mas texto disponible
                    ComFgcolor(100,255,255);
                    ComColor(0,0,0);
                    if(Boton(HSIZE-75, VSIZE-50, 50, 25, 16, "->") && botonPresionado==0){
                        page_s=page+1;
                        if(page_s>=maxPag){ //Saturacion
                            page_s=maxPag-1;
                        }
                        contString_page[page_s]=contString;
                        botonPresionado=1;
                    }

                } else {
                        //En caso contrario, continuar a la siguiente diapositiva
                    ComFgcolor(0,255,0);
                    ComColor(0,0,0);
                    if(Boton(HSIZE-75, VSIZE-50, 50, 25, 16, "Siguiente") && botonPresionado==0){
                        subPantalla_s=3;
                        page_s=0;
                        botonPresionado=1;
                    }
                }

                    //Retroceso
                if(page>0){
                    //En caso de no estar en la primera pagina, dar la opcion a retroceder al texto anterior
                    ComFgcolor(100,255,255);
                    ComColor(0,0,0);
                    if(Boton(25, VSIZE-50, 50, 25, 16, "<-") && botonPresionado==0){
                        page_s=page-1;
                        if(page_s<0){ //Saturacion
                            page_s=0;
                        }
                        botonPresionado=1;
                    }

                } else {
                    //Si estamos en la primera pagina, retroceder de diapositiva
                    ComFgcolor(0,255,0);
                    ComColor(0,0,0);
                    if(Boton(25, VSIZE-50, 50, 25, 16, "Anterior") && botonPresionado==0){
                        subPantalla_s=1;
                        page_s=0;
                        botonPresionado=1;
                    }
                }
                break;

            case 3: //Ejemplos de uso real ----------------------------------------
                dibujarFondo(0);
                ComColor(255,255,255);
                ComTXT(HSIZE/2,30,29,OPT_CENTERX,"Uso en la realidad");

                //Texto
                escribiendo=1;
                contLetra=0;
                contString=contString_page[page];
                contLinea=0;
                sprintf(buffer,"\0");
                while (escribiendo){

                    buffer[contLetra] = Clases[claseSeleccionada].textoUso[contString];
                    if(buffer[contLetra]=='\0' || contString > txtSizeUso){
                        ComTXT(28,60+contLinea*20,27,0,buffer);
                        escribiendo=0;

                    } else if (buffer[contLetra]=='\n' || contLetra>tamLineaClase){
                        if (buffer[contLetra]=='\n' ){
                            buffer[contLetra]='\0';

                        }else if(buffer[contLetra]!=' ' && Clases[claseSeleccionada].textoUso[contString+1]!=' '){
                            buffer[contLetra+1]='-';
                        } else {
                            buffer[contLetra+1]='\0';
                        }


                        ComTXT(28,60+contLinea*20,27,0,buffer);
                        contLetra=0;
                        contLinea++;
                        contString++;
                        sprintf(buffer,"\0");
                        if(contLinea>numLineaClase){
                            escribiendo=0;
                        }

                    } else {
                        contLetra++;
                        contString++;
                    }

                }

                // Imagen

                // Extra

                // Botones de cambio de pantalla
                //Continuar
                if(Clases[claseSeleccionada].textoUso[contString]!='\0' && contString<txtSizeUso && page<(maxPag-1) ){
                    //En caso de haber mas texto disponible, continuar a mas texto disponible
                    ComFgcolor(100,255,255);
                    ComColor(0,0,0);
                    if(Boton(HSIZE-75, VSIZE-50, 50, 25, 16, "->") && botonPresionado==0){
                        page_s=page+1;
                        if(page_s>=maxPag){ //Saturacion
                            page_s=maxPag-1;
                        }
                        contString_page[page_s]=contString;
                        botonPresionado=1;
                    }

                } else {
                    //En caso contrario, continuar a la siguiente diapositiva
                    ComFgcolor(0,255,0);
                    ComColor(0,0,0);
                    if(Boton(HSIZE-75, VSIZE-50, 50, 25, 16, "Siguiente") && botonPresionado==0){
                        subPantalla_s=4;
                        page_s=0;
                        botonPresionado=1;
                    }
                }

                    //Retroceso
                if(page>0){
                    //En caso de no estar en la primera pagina, dar la opcion a retroceder al texto anterior
                    ComFgcolor(100,255,255);
                    ComColor(0,0,0);
                    if(Boton(25, VSIZE-50, 50, 25, 16, "<-") && botonPresionado==0){
                        page_s=page-1;
                        if(page_s<0){ //Saturacion
                            page_s=0;
                        }
                        botonPresionado=1;
                    }

                } else {
                    //Si estamos en la primera pagina, retroceder de diapositiva
                    ComFgcolor(0,255,0);
                    ComColor(0,0,0);
                    if(Boton(25, VSIZE-50, 50, 25, 16, "Anterior") && botonPresionado==0){
                        subPantalla_s=2;
                        page_s=0;
                        botonPresionado=1;
                    }
                }

                break;

            case 4: //Ejemplo de uso ----------------------------------------
                //////// REALMENTE TODO ESTE CASO SE VA A IR AL CARAJO
                dibujarFondo(1);
                ComColor(0,0,0);
                //ComTXT(HSIZE/2,30,27,OPT_CENTERX,"EJEMPLO");

                // Funcion
                Clases[claseSeleccionada].demo();

                // Botones de cambio de pantalla
                    //Retroceso
                ComFgcolor(0,255,0);
                ComColor(0,0,0);
                if(Boton(25, VSIZE-50, 50, 25, 16, "Anterior") && botonPresionado==0){
                    subPantalla_s=3;
                    botonPresionado=1;
                }
                break;

            default:
                subPantalla=0;
                break;
            }

            break;

        case 4:
            //Minijuego
            ComTXT(HSIZE/2,VSIZE/2,31,OPT_CENTERX,"MINIJUEGO");
            break;


        default:
            ComTXT(HSIZE/2,VSIZE/2,31,OPT_CENTERX,"ERROR");
            break;
        }

        // Boton de retroceso
            //NO presente en la pantalla 0 ni en la 2
        if(pantalla>0 && pantalla!=2){
            ComColor(255,255,255);
            ComFgcolor(175,0,0);
            if(Boton(25, 25, 50, 25, 16,"<-")  && botonPresionado==0){

                if (pantalla==3) {
                    pantalla_s=1; //Volvemos a la pantalla de escoger leccion
                } else if(pantalla==4){
                    pantalla_s=2 ; //Volvemos a la pantalla de escoger juego
                } else {
                    pantalla_s=0; //Volvemos a la pantalla de inicio en el resto de casos
                }

                page_s=0;
                subPantalla_s=0;
                botonPresionado=1;
            }
        }

        // Actualizar los valores, esto permite evitar que se lien los botones por los cambios de pantalla
        if(botonPresionado!=0 && !(POSX>0 && POSX<HSIZE) && !(POSY>0 && POSY<VSIZE)){
            page=page_s;
            pantalla=pantalla_s;
            botonPresionado=0;
            subPantalla=subPantalla_s;
        }

        Dibuja();
        Load=100 - (100*TimerValueGet(TIMER0_BASE,TIMER_A))/((RELOJ/FREC)-1);
    }

}

//Funcion predeterminada para las clases, para evitar errores.
void dummyDemo(){
    ComColor(100,0,0);
    ComTXT(HSIZE/2,VSIZE/2,31,OPT_CENTERX,"W.I.P.");
}
//Funcion predeterminada para los juegos, para evitar errores.
int dummyJuego(){
    ComColor(100,0,0);
    if(Boton(HSIZE/2-100,VSIZE/2-100,200,200,31,"WIP")){
        return 1;
    } else {
        return 0;
    }
}


int demoJuego1(void) {
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
        estado = -1;
    }
    return estado;
}


int demoJuego2(void) {
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
        estado = -1;
    }
    return estado;
}

int demoJuego3(void) {
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
        estado = -1;
    }
    return estado;
}


