/*
 * main.c
 */


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>


#include "driverlib2.h"
#include "utils/uartstdio.h"

#include "HAL_I2C.h"
#include "sensorlib2.h"

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


//void Timer0IntHandler(void);

// Variables globales para el codigo
uint32_t outputMicrofono[1];
volatile int lectura=0;

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


    //Configuracion pines del microfono
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOE);

    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3); //Pin E3 ADC

    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_5); //E5: input


    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0); //ADC como secuencia tipo 3 -> Ejecuta 1 orden (leer)
    //
    // Configure step 0 on sequence 3.  Sample channel 0 (ADC_CTL_CH0) in
    // differential mode (ADC_CTL_D) and configure the interrupt flag
    // (ADC_CTL_IE) to be set when the sample is done.  Tell the ADC logic
    // that this is the last conversion on sequence 3 (ADC_CTL_END).  Sequence
    // 3 has only one programmable step.  Sequence 1 and 2 have 4 steps, and
    // sequence 0 has 8 programmable steps.  Since we are only doing a single
    // conversion using sequence 3 we will only configure step 0.   For more
    // information on the ADC sequences and steps, refer to the datasheet.
    //
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 |
                             ADC_CTL_IE | ADC_CTL_END);

    ADCSequenceEnable(ADC0_BASE, 3); //Enable sequence 3
    ADCIntClear(ADC0_BASE, 3);



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
    ComTXT(HSIZE/2,VSIZE/5, 30, OPT_CENTERX,"Sensor de sonido");


    //UARTprintf("\nAntepantalla1\n"); //DEBUG
    Dibuja();
    //UARTprintf("\nAntepantalla\n"); //DEBUG
    Espera_pant();
    //UARTprintf("\nTraspantalla\n"); //DEBUG
    int i;
    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);
    Nueva_pantalla(16,16,16);
    Dibuja();

    while(1){
        //UARTprintf("DEBUG\n");
        SLEEP;

        // LEER MICROFONO
            // 1. Leer el pin ADC
        ADCProcessorTrigger(ADC0_BASE, 3);
        while(!ADCIntStatus(ADC0_BASE, 3, false)){}
        ADCIntClear(ADC0_BASE, 3);
        ADCSequenceDataGet(ADC0_BASE, 3, outputMicrofono);

            // 2. Leer si hay ruido
        lectura = GPIOPinRead(GPIO_PORTE_BASE,GPIO_PIN_5);

            // 3. Sacar resultado
        UARTprintf("Output  = %d  = %d%%| %d\n", outputMicrofono[0],(outputMicrofono[0] * 100) / 4095,lectura);


        //UARTprintf("DEBUG SLEEP\n");
        //Lee_pantalla();

        //Nueva_pantalla(16,16,16);


        //tMedida++;
        //UARTprintf("DEBUG 0.05s\n");
        //if(tMedida>10){ //10*50ms=500ms
        //    //UARTprintf("DEBUG 0.5s-------------------\n");
        //    tMedida=0;

        //}
        //Dibuja();
        //Load=100 - (100*TimerValueGet(TIMER0_BASE,TIMER_A))/((RELOJ/FREC)-1);
    }

}

