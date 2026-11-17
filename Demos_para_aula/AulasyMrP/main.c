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
volatile int page_s=0;
volatile int pantalla_s=0;
volatile int subPantalla_s=0;
volatile int botonPresionado=0;

//Dibujar por pantalla;
char buffer[100];
void dibujarFondo(int id);

//Dibujar lecciones
#define tamLineaClase 49
#define numLineaClase 7 //Se escribe una linea mas que la especificada porque se indexa en 0
#define numLineaIntro 2
#define numLineaFin 4
#define txtDesarrollo 2000
#define txtCorto 500
#define maxPag 5
volatile int contLetra=0;
volatile int contLinea=0;
volatile int contString=0;
volatile int contString_page[maxPag]={0,0,0,0,0};
volatile int escribiendo=0; // Monitoriza si estamos escribiendo texto

//volatile char dummy[] = "En un lugar de la mancha de cuyo nombre no quiero acordarme, vivia un hidalgo de aquellos de lanza en astillero, rocin flaco y galgo corredor.\nVa una olla de algo mas vaca que carnero, salpicon las mas noches, duelos y quebrantos los sabado, lantejas viernes, algun palomino de anadidura los domingos, consumian las tres partes de su hacienda. El resto concluian sayo de velarte, calcas de velludo para las fiestas, con sus pantunflos de lo mesmo, y los dias entre semana se honraua con su vellori de lo mas fino.\n\n  Tenia en su casa una ama que pasaba de los cuarenta, y una sobrina que no llegaba a los veinte, y unn mozo de campo y plaza, que asi ensillaua el rozin como tomaua la podadera. Frisaba la edad nuestro hidalgo con los cincuenta anos. Era de complexion rezia, seco de carnes, enjunto de rostro, gran madrugador y amigo de la caza. Quieren decir que tenia el sobrenombre de Quixadam o Quesada, que en esto hay alguna diferencia en los autores que deste caso escriuen, aunque por conjeturas verosimiles se dexa entender que se llamaua Quexada. Pero esto importa poco a nuestro cuento; basta con que en la narracion del no se salga un punto de la verdad";
//volatile char dummy2[] = "Sphinx of black quartz, judge my vow.\n...\nThe five boxing wizards jump quickly.\n...\nPack my box with five dozen liquor jugs.";

///////////// DATOS PARA LAS LECCIONES -- MOVER A LIBRERIA APARTE??? ---
struct DatosLeccion{
    char nombre[20];
    char textoIntroduccion[txtCorto];
    char textoExplicacion[txtDesarrollo];
    char textoCaracteristicas[txtDesarrollo];
    char textoUso[txtCorto];
    void (*demo)(void);
};

struct DatosLeccion Clases[nClases];

//Funciones de lecciones
void dummyDemo(); //Funcion predeterminada para las clases
void TecladoMusical(); //Demo teclado

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
    struct DatosLeccion defaultStruct = {"\0","\0","\0","\0","\0",&dummyDemo};

    for(i=0;i<nClases;i++){
        Clases[i]=defaultStruct;
    }

    strcpy(Clases[0].nombre,"Pantalla Resistiva");
    strcpy(Clases[0].textoIntroduccion,"Las pantallas resistivas ofrecen al usuario una forma comoda y rapida de interaccion con el sistema.");
    strcpy(Clases[0].textoExplicacion,"Para evitar un sobrecargar del microcontrolador\nprincipal, la pantalla tiene un coprocesador\nintegrado, encargado de dibujar la pantalla con\nfunciones propias para la interfaz.\n\nLa resolucion de las pantallas consiste en la\ncantidad de pixeles a lo ancho y alto que\nposeen y no de sus dimensiones fisicas.\nA cada pixel se le debe proporcionar un valor\nde rojo, verde y azul, permitiendo dibujar en una\ngama de 262.144 colores distintos.\n(Este rango de colores depende del rango de\nvalores que acepten el rojo, verde y azul,\nusualmente de 0 a 255)");
    strcpy(Clases[0].textoCaracteristicas,"Por delante de la capa en la que muestra la\nimagen se colocan laminas resistivas para la\ndeteccion de pulsacion.\nCada una de estas capas tienen un terminal\npositivo y otro negativo, por tanto si\nutilizamos dos podemos obtener la posicion en\nel plano (X+,X-,Y+,Y-)\n\nPor ejemplo, para calcular la posicion en el\neje Y se colocan los terminales 'Y+' a 1, 'Y-'\na 0 y 'X-' al aire, y se lee el valor de la\npulsacion por el terminal 'X+'; este valor\nvariara segun la altura a la que pulsemos de\nla pantalla.\n\n\nUsar una pantalla resistiva nos permite\nintegrar comunicacion usuario-maquina de forma\ncomoda y elegante, distinguiendose de otras\npantallas tactiles gracias a que podemos\nutilizarla solo con presionarla y no\nnecesitando unas condiciones de uso que\npermitan que un sensor capacitivo funcione.\n\nEl costo a pagar es que las capas resistivas\nreducen la visibilidad de la imagen, solo son\ncapaces de detectar un punto de presion y es\nnecesario calibrarlas antes de usarlas.");
    strcpy(Clases[0].textoUso,"Estas se usan en muchos entornos distintos;\nen consultas medicas, donde por proteccion y\nlimpieza usan guantes y esterilizan el equipo,\no en pantallas de videoconsolas como la familia\nde las ds.");
    Clases[0].demo = &TecladoMusical;

    //UARTprintf("\nComenzamos pantalla\n");
    // PANTALLA
    HAL_Init_SPI(2, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();       //Arranque de la pantalla


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

            if( (nClases-(page*5)) >=5){
                clasesDisponibles=5;
            } else if ( (nClases-(page*5)) >0){ //Proteccion en caso de error
                clasesDisponibles= (nClases-(page*5)) ;
            }

            ComFgcolor(255,0,0);
            for(c=page*5;c<clasesDisponibles+page*5;c++){
                sprintf(buffer,"Clase %d",c);
                if(Boton(15+70*(c-page*5), VSIZE/2, 50, 50,21, buffer) && botonPresionado==0){
                    pantalla_s=3; //Vamos a clase
                    claseSeleccionada=c;
                    page_s=0;
                    botonPresionado=1;
                }

            }

            //Posible cambio de pagina si se necesita
            //Siguiente pagina si aun quedan juegos
            if( (nClases-(page*5)) >5){
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
            ComTXT(HSIZE/2,VSIZE/2,31,OPT_CENTERX,"JUEGOS");
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
                    if(buffer[contLetra]=='\0' || contString > txtDesarrollo){
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
                    if(buffer[contLetra]=='\0' || contString > txtDesarrollo){
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
                if(Clases[claseSeleccionada].textoExplicacion[contString]!='\0' && contString<txtDesarrollo && page<(maxPag-1) ){
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
                    if(buffer[contLetra]=='\0' || contString > txtDesarrollo){
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
                if(Clases[claseSeleccionada].textoCaracteristicas[contString]!='\0' && contString<txtDesarrollo && page<(maxPag-1) ){
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
                contString=0;
                contLinea=0;
                sprintf(buffer,"\0");
                while (escribiendo){

                    buffer[contLetra] = Clases[claseSeleccionada].textoUso[contString];
                    if(buffer[contLetra]=='\0' || contString > txtDesarrollo){
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
                        if(contLinea>numLineaFin){
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
                ComFgcolor(0,255,0);
                ComColor(0,0,0);
                if(Boton(HSIZE-75, VSIZE-50, 50, 25, 16, "Siguiente") && botonPresionado==0){
                    subPantalla_s=4;
                    botonPresionado=1;
                }

                    //Retroceso
                ComFgcolor(0,255,0);
                ComColor(0,0,0);
                if(Boton(25, VSIZE-50, 50, 25, 16, "Anterior") && botonPresionado==0){
                    subPantalla_s=2;
                    botonPresionado=1;
                }
                break;

            case 4: //Ejemplo de uso ----------------------------------------
                //////// REALMENTE TODO ESTE CASO SE VA A IR AL CARAJO
                dibujarFondo(1);
                ComColor(0,0,0);
                ComTXT(HSIZE/2,30,27,OPT_CENTERX,"EJEMPLO");

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
            //NO presente en la pantalla 0
        if(pantalla>0){
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

//Funcion predeterminada para las clases, para evitar errores.
void dummyDemo(){
    ComColor(100,0,0);
    ComTXT(HSIZE/2,VSIZE/2,31,OPT_CENTERX,"W.I.P.");
}


#define TAG(x)       ((3UL << 24) | ((x) & 0xFF)) // Detección widget tocado
// Etiqueta del slider
#define TAG_SLIDER 34

// V. Programa
volatile char Cambia=0;
char cadena[10];
volatile int seleccion = 127;
int contador_paso = 0;
int estado_color = 0;

//Funcion teclado musical ---- MOVER A LIBRERIA POSTERIORMENTE
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

