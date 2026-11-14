#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"

#include "HAL_I2C.h"
#include "sensorlib2.h"

/***********************************
 * Demo con sensores
 *************************************/

// V.Pantalla
const int32_t REG_CAL[6]= {CAL_DEFAULTS}; // Calibración
#define TAG(x)       ((3UL << 24) | ((x) & 0xFF)) // Detección widget tocado

// V.Reloj
int RELOJ; // Variable que guarda la frecuencia del programa
#define FREC 50 //Frecuencia en hercios del tren de pulsos: 20ms

// V. Programa
volatile char Cambia=0;
char cadena[32];

// V. OPT3001
float lux; // Guarda la lectura del sensor de luz
int lux_i; // Guarda el valor de lux sin decimales

// V. BME280
int returnRslt; // Lectura valores sensor
int g_s32ActualTemp   = 0;  // Lectura temperatura
unsigned int g_u32ActualPress  = 0; // Lectura presión
unsigned int g_u32ActualHumity = 0; // Lectura humedad
float T_act,P_act,H_act; // Transformación de lecturas

// BMI160/BMM150
int8_t returnValue;
uint8_t DevID8=0;
struct bmi160_gyro_t        s_gyroXYZ;
struct bmi160_accel_t       s_accelXYZ;
struct bmi160_mag_xyz_s32_t s_magcompXYZ;
float AccX=0, AccY=0, AccZ=0,ValorG=0;
float GyrX=0, GyrY=0, GyrZ=0;

//Calibration off-sets
int8_t accel_off_x;
int8_t accel_off_y;
int8_t accel_off_z;
int16_t gyro_off_x;
int16_t gyro_off_y;
int16_t gyro_off_z;
float T_act,P_act,H_act;
bool BME_on = true;

// V. Confirmación
uint8_t Sensor_OK=0;
uint8_t Opt_OK, Bme_OK, Bmi_OK;

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

void enciende_leds(int Led3, int Led2, int Led1, int Led0){
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1*Led0);
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0*Led1);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4*Led2);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0*Led3);

    // Representación gráfica en pantalla
    ComColor(0, 0, 0);
    ComCirculo(HSIZE/4-60,220,15);
    ComCirculo(HSIZE/4-20,220,15);
    ComCirculo(HSIZE/4+20,220,15);
    ComCirculo(HSIZE/4+60,220,15);

    if(Led3 == 1){ComColor(0, 255, 0);
    }else{ComColor(25, 25, 25);}
    ComCirculo(HSIZE/4-60,220,13);

    if(Led2 == 1){ComColor(0, 255, 0);
    }else{ComColor(25, 25, 25);}
    ComCirculo(HSIZE/4-20,220,13);

    if(Led1 == 1){ComColor(0, 255, 0);
    }else{ComColor(25, 25, 25);}
    ComCirculo(HSIZE/4+20,220,13);

    if(Led0 == 1){ComColor(0, 255, 0);
    }else{ComColor(25, 25, 25);}
    ComCirculo(HSIZE/4+60,220,13);

}

void Sensores (void){
    // Variables propias
    static float Angulo_X = 0.0f;
    static float Angulo_Y = 0.0f;

    // Posicion de la bola en pantalla
    static int Bola_X;
    static int Bola_Y;

    // Sensibilidad del nivel
#define ANGULO_MAX     15.0f

    // Lectura
    if(Cambia==1){
        Cambia=0;
        if(Opt_OK)  // Lectura luz
        {
            lux=OPT3001_getLux();
            lux_i=(int)round(lux);
        }
        if(Bme_OK) // Lectura temp, presion y humedad
        {
            returnRslt = bme280_read_pressure_temperature_humidity(
                    &g_u32ActualPress, &g_s32ActualTemp, &g_u32ActualHumity);
            T_act=(float)g_s32ActualTemp/100.0;
            P_act=(float)g_u32ActualPress/100.0;
            H_act=(float)g_u32ActualHumity/1000.0;
        }
        if(Bmi_OK){
            bmi160_bmm150_mag_compensate_xyz(&s_magcompXYZ);
            bmi160_read_accel_xyz(&s_accelXYZ);
            bmi160_read_gyro_xyz(&s_gyroXYZ);

            // Conversion aceleracion a m/s2
            // Rango de acelerometro: +/- 2g
            AccX=(9.8f*(float)s_accelXYZ.x)/16384.0f;
            AccY=(9.8f*(float)s_accelXYZ.y)/16384.0f;
            AccZ=(9.8f*(float)s_accelXYZ.z)/16384.0f;

            // Calculo de angulos de inclinacion
            Angulo_X=atan2f(AccX,sqrtf(AccY*AccY+AccZ*AccZ))*180.0f/3.14159265f;
            Angulo_Y=atan2f(AccY,sqrtf(AccX*AccX+AccZ*AccZ))*180.0f/3.14159265f;

            // Conversion giroscopio a grados/s
            GyrX=0.061f*(float)s_gyroXYZ.x;
            GyrY=0.061f*(float)s_gyroXYZ.y;
            GyrZ=0.061f*(float)s_gyroXYZ.z;
        }
    }

    // Representacion de sensores
    // ZONA GAUGE
    ComColor(0, 0, 0);
    ComCirculo(110 ,110 , 74);

    ComBgcolor(70, 70, 70);
    ComColor(255, 255, 255);
    if(P_act>=1030){
        cmd_gauge(110 ,110 , 70, 0, 10, 6, 30, 30);
    }else if(P_act <=1000){
        cmd_gauge(110 ,110 , 70, 0, 10, 6, 0, 30);
    }else{
        cmd_gauge(110 ,110 , 70, 0, 10, 6, P_act-1000, 30);
    }

    ComColor(0, 0, 0);
    ComRect(56, 156, 164, 179, true);

    ComColor(70, 70, 70);
    ComRect(60, 160, 160, 175, true);
    ComColor(255, 255, 255);
    sprintf(cadena,"%.0f",P_act);
    ComTXT(110,127, 26, OPT_CENTERX,cadena);
    ComTXT(85,160, 26, OPT_CENTERX,"1000");
    ComTXT(135,160, 26, OPT_CENTERX,"1030");

    // ZONA TEMPERATURA
    ComBgcolor(0, 255, 0);
    ComColor(0, 0, 0);
    ComCirculo(HSIZE/2,3*VSIZE/5+10,22);
    ComLine(HSIZE/2,3*VSIZE/5+10,HSIZE/2,VSIZE/5-5,15);

    ComColor(255, 255, 255);
    ComCirculo(HSIZE/2,3*VSIZE/5+10,20);
    ComLine(HSIZE/2,3*VSIZE/5+10,HSIZE/2,VSIZE/5-5,12);

    ComColor(255, 0, 0);
    ComCirculo(HSIZE/2,3*VSIZE/5+10,16);
    if(T_act>=40){
        ComRect(HSIZE/2-4, 3*VSIZE/5-1.5f*VSIZE/5+10, HSIZE/2+4, 3*VSIZE/5-5+10, true);
    }else if(T_act <=20){
        ComRect(HSIZE/2-4, 3*VSIZE/5-5, HSIZE/2+4, 3*VSIZE/5+5, true);
    }else{
        ComRect(HSIZE/2-4, 3*VSIZE/5-((T_act-20.0f)/20.0f)*(2.0f*VSIZE/5)-5, HSIZE/2+4, 3*VSIZE/5+5, true);
    }

    ComColor(0, 0, 0);
    sprintf(cadena,"%.0f",T_act);
    ComTXT(HSIZE/2,3*VSIZE/5+2, 26, OPT_CENTERX,cadena);

    // ZONA HUMEDAD
    ComColor(0, 0, 0);
    ComLine(6.5f*HSIZE/10-10, 80, 9*HSIZE/10-10, 80,24);

    ComColor(255, 255, 255);
    ComBgcolor(0, 230, 255);
    if(H_act>=100){
        cmd_progress(6.5f*HSIZE/10-10, 60, 2.5f*HSIZE/10, 40, 0, 60, 60);
    }else if(H_act <=40){
        cmd_progress(6.5f*HSIZE/10-10, 60, 2.5f*HSIZE/10, 40, 0, 0, 60);
    }else{
        cmd_progress(6.5f*HSIZE/10-10, 60, 2.5f*HSIZE/10, 40, 0, H_act-40, 60);
    }

    ComColor(0, 0, 0);
    sprintf(cadena,"HUM: %.0f %%",H_act);
    ComTXT(7.75f*HSIZE/10-10,74, 26, OPT_CENTERX,cadena);

    // ZONA LUMINOSIDAD
    if(lux_i >= 7000){
        enciende_leds(0,0,0,0);
    }else if(lux_i >= 4000){
        enciende_leds(0,0,0,1);
    }else if(lux_i >= 1000){
        enciende_leds(0,0,1,1);
    }else if(lux_i >= 100){
        enciende_leds(0,1,1,1);
    }else{
        enciende_leds(1,1,1,1);
    }

    // ZONA GIROSCOPIO
    // Posicion de la bola segun inclinacion
    Bola_X=(HSIZE-120)+(int)((Angulo_X/ANGULO_MAX)*39);
    Bola_Y=(VSIZE-88)-(int)((Angulo_Y/ANGULO_MAX)*39);

    // Limitar desplazamiento horizontal
    if(Bola_X>HSIZE-81)  Bola_X=HSIZE-81;
    if(Bola_X<HSIZE-159) Bola_X=HSIZE-159;

    // Limitar desplazamiento vertical
    if(Bola_Y>VSIZE-49)  Bola_Y=VSIZE-49;
    if(Bola_Y<VSIZE-127) Bola_Y=VSIZE-127;

    // CUADRADO DEL NIVEL
    ComColor(0,0,0);
    ComRect(HSIZE-175, VSIZE-143, HSIZE-65, VSIZE-33, true);

    // Fondo interior
    ComColor(210,225,235);
    ComRect(HSIZE-171, VSIZE-139, HSIZE-69, VSIZE-37, true);

    // Circulo guia
    ComColor(100,100,100);
    ComCirculo(HSIZE-120, VSIZE-88, 35);

    // DIBUJO DE LA BOLA
    ComColor(0,80,255);
    ComCirculo(Bola_X, Bola_Y, 9);

    // Centro de la bola
    ComColor(150,210,255);
    ComCirculo(Bola_X-3, Bola_Y-3, 3);
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

    // Configuración Boosterpack de los sensores:
    Conf_Boosterpack(1, RELOJ);
    // Inicializar OPT3001
    if(Test_I2C_Dir(OPT3001_SLAVE_ADDRESS)){
        OPT3001_init();
        Opt_OK = 1;
    }else {
        Opt_OK = 0;
    }

    // Inicializar BME280
    if(Test_I2C_Dir(BME280_I2C_ADDRESS2)){
        bme280_data_readout_template();
        bme280_set_power_mode(BME280_NORMAL_MODE);
        Bme_OK = 1;
    }else {
        Bme_OK = 0;
    }
    Sensor_OK=Test_I2C_Dir(BMI160_I2C_ADDR2);
    if(!Sensor_OK){
        Bmi_OK=0;
    }else{
        bmi160_initialize_sensor();
        bmi160_config_running_mode(APPLICATION_NAVIGATION);
        readI2C(BMI160_I2C_ADDR2,BMI160_USER_CHIP_ID_ADDR, &DevID8, 1);
        bmi160_accel_foc_trigger_xyz(3, 3, 1, &accel_off_x, &accel_off_y, &accel_off_z);
        bmi160_set_foc_gyro_enable(1, &gyro_off_x, &gyro_off_y,&gyro_off_z);

        Bmi_OK=1;
    }


    // Pantalla y configuración pantalla inicio
    SysCtlDelay(RELOJ/3);
    HAL_Init_SPI(2, RELOJ);  //Boosterpack a usar, Velocidad del MC
    Inicia_pantalla();       //Arranque de la pantalla

    Nueva_pantalla(16,16,16);
    ComColor(0, 100, 100);
    ComLineWidth(5);
    ComRect(5, 5, HSIZE-5, VSIZE-5, true);
    ComColor(0, 134, 124);
    ComRect(12, 12, HSIZE-12, VSIZE-12, true);

    ComColor(255,255,255);
    ComTXT(HSIZE/2,10+VSIZE/5, 31, OPT_CENTERX,"DEMO");
    ComTXT(HSIZE/2,60+VSIZE/5, 31, OPT_CENTERX,"Sensores boosterpack");
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
        Sensores();
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

