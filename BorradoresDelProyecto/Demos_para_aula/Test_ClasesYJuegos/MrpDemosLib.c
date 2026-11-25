////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////               /////////                ///////    /////////////   /////////           //////////            //////////////////////////////////////////
/////   //////////    ///////    ///////////////////      /////////     ///////   ////////   ///////   ////////////////////////////////////
/////   ///////////    //////    ///////////////////   //   /////   /   //////   ////////////   //    ///////////////////////////////////
/////   ////////////   //////    ///////////////////   ////   /   ///   /////   /////////////   //    /////////////////////
/////   ////////////   //////                ///////   //////    ////   /////   /////////////   ////      /////////////////////////////////////////////////
/////   ////////////   //////    ///////////////////   //////////////   /////   /////////////   ////////       /////////////////////////
/////   ////////////   //////    ///////////////////   //////////////   /////   /////////////   //////////////     ///////////////////////
/////   ////////////   //////    ///////////////////   //////////////   //////   ///////////   //////////////////   /////////////////////
/////   //////////    ///////    ///////////////////   //////////////   ///////   ////////   /////////////////     ///////////////////////
/////               /////////                ///////   //////////////   ////////           //////////         ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* ---------------------------------------------------------------------------------------------------\\
|| En esta libreria se incluyen todas las funciones necesarias para todos los programas               ||
|| de demostracion de funcionamiento de los sensores utilizados en el aula de Mr. P                   ||
|| 0. Declaracion de las librerias necesarias para cada sensor.                                       ||
|| 1.  Sensor capacitivo                                                                              ||
|| 2.  Sensor IR                                                                                      ||
|| 3.  Microfono                                                                                      ||
|| 4.  Pantalla                                                                                       ||
|| 5.  Sensor-Boosterpack                                                                             ||
|| 6.  Teclado matricial                                                                              ||
|| 7.  Ultrasonido                                                                                    ||
|| 8.  Real Time Clock (RTC)                                                                          ||
|| 9.  Joystick                                                                                       ||
|| 10. Funcion para mostral la imagen del microcontrolador                                            ||
\\----------------------------------------------------------------------------------------------------*/


/// 0. Librerias
#include <MrpDemosLib.h>
// 0.0 Librerias generales
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "MrpEscenariosLib.h"
#include "driverlib2.h"
//Extra: Algunos define respecto el funcionamiento de la placa
#define FREC            50          // Timer0A: 50 Hz (20 ms por interrupcion)

//0.1. Sensor capacitivo
#include "touch.h"
//0.2. Sensor IR
#include "tsop1838.h"
//0.3. Microfono ---- No requiere
//0.4. Pantalla resisitiva
#include "FT800_TIVA.h"
//0.5. Sensor Boosterpack
#include "HAL_I2C.h"
#include "sensorlib2.h"
//0.6. Teclado matricial
#include "teclado.h"
//0.7 Sensor de ultrasonido
#include "hcsr04.h"
//0.8. Real Time Clock
#include "ds3231.h"
//0.9. Joystick ---- No requiere
//0.10 Imagen del micro
#include "imgs.h"

/// 1. SENSOR CAPACITIVO
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

    static int seleccion = 127;   // Volumen (0-255)

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

/// 2. Sensor IR

// Suma PASO_COLOR a un canal de la ropa (desplazamiento: 16 = R, 8 = G, 0 = B).
#define PASO_COLOR 15
// Si se pasa de 255 vuelve a 0 y NO afecta a los otros canales.
static void SumaColor(int desplazamiento)
{
    int c = (colorRopaMrP >> desplazamiento) & 0xFF;

    c += PASO_COLOR;
    if(c > 255) c = 0;

    colorRopaMrP = (colorRopaMrP & ~(0xFFu << desplazamiento)) | ((uint32_t)c << desplazamiento);
}

void sensorIRyMrP(void){

    //Variables estaticas para relacionar a mrP con el sensor de IR
    static char visibleMrP=1;
    static char gafasMrP=1;
    static char emocionMrP=0;
    static char animMrP=0;
    //Dibujamos a MrP (antes para evitar que no se dibuja si no se lee nada)
    dibujarMrP(HSIZE/2,50,emocionMrP,gafasMrP,visibleMrP,animMrP);

    static IR_Trama trama;

    // Solo se actua cuando llega una tecla NUEVA (antes el switch se ejecutaba
    // en cada ciclo de 20 ms con la ultima tecla guardada)
    if(!IR_Lee(&trama)){
        return;
    }

    //UARTprintf("cmd=0x%02X rep=%d\n", trama.comando, trama.repeticion);

    switch(trama.comando)
    {
    //Cambios de animacion
    case 0x45: animMrP=1; break; //Tecla 1
    case 0x46: animMrP=2; break; //Tecla 2
    case 0x47: animMrP=3; break; //Tecla 3
    case 0x44: animMrP=4; break; //Tecla 4
    case 0x40: animMrP=5; break; //Tecla 5
    case 0x43: animMrP=6; break; //Tecla 6
    case 0x07: animMrP=7; break; //Tecla 7
    case 0x15: animMrP=8; break; //Tecla 8
    case 0x09: animMrP=9; break; //Tecla 9
    case 0x19: animMrP=0; break; //Tecla 0

    //Cambios cosmeticos (solo con pulsacion nueva, no con la repeticion al mantener)
    case 0x16: //Tecla *
        if(!trama.repeticion){
            gafasMrP = !gafasMrP;
        }
        break;
    case 0x0D: //Tecla #
        if(!trama.repeticion){
            emocionMrP++; //Ciclamos por sus emociones
            if(emocionMrP>=6){
                emocionMrP=0;
            }
        }
        break;
    case 0x1C: //Tecla OK
        if(!trama.repeticion){
            visibleMrP = !visibleMrP;
        }
        break;

        //Cambios de ropa (con la tecla mantenida sigue cambiando)
    case 0x08: SumaColor(16); break; //Tecla izq   - Cambiamos R
    case 0x18: SumaColor(8);  break; //Tecla up    - Cambiamos G
    case 0x5A: SumaColor(0);  break; //Tecla der   - Cambiamos B
    case 0x52:                       //Tecla down  - Regresamos al color original
        colorRopaMrP=9868950;
        break;
    }

}

//3. Sensor de sonido

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

//4. Pantalla resistiva
void TecladoMusical(void) {
    // V. Programa
    static char cadena[10];
    static int seleccion = 127;
    static int contador_paso = 0;
    static int estado_color = 0;

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

//5. Sensor Boosterpack

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

void SetupSensorBoosterpack(){
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
}

void SensoresBoosterpack (void){
    // Variables propias
    static float Angulo_X = 0.0f;
    static float Angulo_Y = 0.0f;

    // Posicion de la bola en pantalla
    static int Bola_X;
    static int Bola_Y;

    char cadena[32];


    // Sensibilidad del nivel
#define ANGULO_MAX     15.0f

    // Lectura
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

//6. Teclado matricial
void DibujaTeclaVisual(int x, int y, char *texto)
{
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

    // V.Teclado
    #define TAM_CODIGO      16U         // maximo de teclas guardadas
    static teclado_evento_t ev;
    static char tecla;
    static char codigo[TAM_CODIGO + 1U];
    static uint32_t n = 0U;

    //
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


// 7. Sensor de ultrasonidos
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
    VolNota(20);

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

// 8. RTC

// V.RTC: ajuste de la hora (solo se usa si PROGRAMAR_RTC = 1)
#define PROGRAMAR_RTC   0           // Poner a 1 para ajustar la hora; despues volver a 0
#define INI_HORAS       20
#define INI_MINUTOS     46
#define INI_SEGUNDOS    0
#define INI_DIA_SEMANA  1           // 1..7: correspondencia definida por el usuario
#define INI_DIA         2
#define INI_MES         11
#define INI_ANIO        2026        // 2000..2199

// V.Pantalla: posicion y tamano del reloj
#define RELOJ_X         (HSIZE/2)
#define RELOJ_Y         100
#define RELOJ_R         68
#define LECTURA_RTC     5           // Se lee el RTC cada 5 interrupciones (100 ms)
// Programa completo del reloj: lee el RTC y dibuja la pantalla.
// Se llama en cada interrupcion; solo redibuja cuando cambia el segundo.
void ProgramaReloj(void)
{
    static uint8_t cuenta = LECTURA_RTC - 1;  // Fuerza la lectura en la primera llamada
    static int ultima_marca = -1;             // Segundo dibujado (100 = error)
    static DS3231_DateTime rtc;
    static bool rtc_ok=0;
    int marca;
    char texto[24];

    // --- 1. RELOJ DE MANILLAS ---
    ComBgcolor(0, 100, 100);    // Esfera
    ComColor(255, 255, 255);    // Marcas y manillas
    if (rtc_ok) {
        // La manilla de las horas es de 12 h: se pasa la hora modulo 12
        cmd_clock(RELOJ_X, RELOJ_Y, RELOJ_R, 0, rtc.hours % 12, rtc.minutes, rtc.seconds, 0);
    } else {
        cmd_clock(RELOJ_X, RELOJ_Y, RELOJ_R, 0, 0, 0, 0, 0);
    }

    // --- 2. FECHA Y HORA EN TEXTO, DEBAJO DEL RELOJ ---
    ComColor(0, 0, 0);
    if (rtc_ok) {
        sprintf(texto, "%02u/%02u/%04u", (unsigned int)rtc.day, (unsigned int)rtc.month,
                (unsigned int)rtc.year);
        ComTXT(HSIZE/2, 172, 28, OPT_CENTERX, texto);

        sprintf(texto, "%02u:%02u:%02u", (unsigned int)rtc.hours, (unsigned int)rtc.minutes,
                (unsigned int)rtc.seconds);
        ComTXT(HSIZE/2, 200, 30, OPT_CENTERX, texto);
    } else {
        ComTXT(HSIZE/2, 172, 28, OPT_CENTERX, "Error RTC");
        sprintf(texto, "Codigo %u", (unsigned int)DS3231_GetLastError());
        ComTXT(HSIZE/2, 200, 28, OPT_CENTERX, texto);
    }

    // --- 3. LECTURA DEL RTC cada 100 ms ---
    cuenta++;
    if (cuenta < LECTURA_RTC) return;
    cuenta = 0;

    rtc_ok = DS3231_ReadDateTime(&rtc);

    // --- 4. Solo se actualiza si cambia el segundo o hay error
    marca = rtc_ok ? rtc.seconds : 100;
    if (marca == ultima_marca) return;
    ultima_marca = marca;

}


// 9. Joystick, aka, shooter de MrP
// Ejes (potenciometros): PK2 = AIN18 (X), PK3 = AIN19 (Y). Boton: PP4
#define ADC_MAX 4095    // Fondo de escala del ADC (12 bits)
//Funcion de demostracion del joystick
void JoystickMrP(void){
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

// 10. Foto del micro
void Dibuja_TM4C(void)
{
    static bool hasImgLoaded=0;

    if(!hasImgLoaded){
        // 6. Medidas de la imagen y carga en RAM_G (una sola vez)
        if(!JPEG_Medidas(mi_imagen_jpeg, mi_imagen_jpeg_len, &ImgAncho, &ImgAlto)){
            while(1){ // JPEG no valido
                Nueva_pantalla(16,16,16);
                ComColor(255,0,0);
                ComTXT(HSIZE/2,VSIZE/2,31,OPT_CENTERX,"JPEG INVALIDO");
                Dibuja();
            }
        }
        if((unsigned long)ImgAncho * ImgAlto * 2 > 262144UL){  // no cabe en RAM_G
            while(1){
                Nueva_pantalla(16,16,16);
                ComColor(255,0,0);
                ComTXT(HSIZE/2,VSIZE/2,31,OPT_CENTERX,"NO RAM DISPONIBLE");
                Dibuja();

            }
        }
        if(!ComLoadImageJPEG(IMG_DIR, OPT_RGB565 | OPT_NODL, mi_imagen_jpeg, mi_imagen_jpeg_len)){
            while(1){
                Nueva_pantalla(16,16,16);
                ComColor(255,0,0);
                ComTXT(HSIZE/2,VSIZE/2,31,OPT_CENTERX,"ERROR COPORCESADOR");  // fallo del coprocesador
                Dibuja();
            }
        }
        hasImgLoaded=1;

    }

    int x = (HSIZE - (int)ImgAncho) / 2;
    int y = (VSIZE+10 - (int)ImgAlto) / 2;

    // Como se cargo con OPT_NODL hay que decir donde esta y como es el bitmap
    EscribeRam32(0x01000000 | IMG_DIR);                                          // BITMAP_SOURCE
    EscribeRam32(0x07000000 | (7UL << 19) | ((ImgAncho * 2UL) << 9) | ImgAlto);  // BITMAP_LAYOUT (RGB565 = 7)
    EscribeRam32(0x08000000 | ((unsigned long)ImgAncho << 9) | ImgAlto);         // BITMAP_SIZE

    ComColor(255,255,255);
    EscribeRam32(CMD_BEGIN_BMP);
    ComVertex2II(x, y, 0, 0);
    EscribeRam32(CMD_END);

    //Una vez hemos dibujado la imagen, tocan los corazoncitos
    ComColor(255, 173, 238);
    ComCirculo(50,VSIZE-60,20);
    ComCirculo(70,VSIZE-60,20);
    ComLine(60,VSIZE-40,50,VSIZE-60,20);
    ComLine(60,VSIZE-40,70,VSIZE-60,20);


    ComColor(255, 173, 238);
    ComCirculo(HSIZE-60,60,30);
    ComCirculo(HSIZE-90,60,30);
    ComLine(HSIZE-75,100,HSIZE-95,70,20);
    ComLine(HSIZE-75,100,HSIZE-55,70,20);

    static int temp=0;
    static int xStar=0;
    static int yStar=0;

    ComColor(255,255,0);
    ComLine(xStar+5,yStar,xStar+15,yStar,2);
    ComLine(xStar-5,yStar,xStar-15,yStar,2);
    ComLine(xStar,yStar+5,xStar,yStar+15,2);
    ComLine(xStar,yStar-5,xStar,yStar-15,2);

    temp++;
    if(temp>4){
        xStar = rand()%(HSIZE-20)+20;
        yStar = rand()%(VSIZE-20)+20;
        temp=0;
    }

    dibujarMrP(HSIZE-120,VSIZE/2-30,0,1,1,5);
}


