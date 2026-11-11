/*
 * tsop1838.c
 *
 * Decodificador NEC por interrupcion para el TSOP1838 en el TM4C1294.
 *
 * Trama NEC (la salida del TSOP es activa a nivel bajo):
 *   - Inicio:  marca de 9 ms + espacio de 4,5 ms
 *   - 32 bits (LSB primero): direccion, ~direccion, comando, ~comando
 *       cada bit = marca de 560 us + espacio de 560 us (0) o de 1690 us (1)
 *   - Marca final de 560 us
 *   - Si se mantiene la tecla, cada 108 ms llega una "repeticion":
 *       marca de 9 ms + espacio de 2,25 ms + marca de 560 us
 *
 * Solo se miden los tiempos entre FLANCOS DE BAJADA consecutivos. Asi se evita
 * la deformacion que el TSOP hace en la anchura de las marcas:
 *   inicio = 13,5 ms | repeticion = 11,25 ms | bit 0 = 1,125 ms | bit 1 = 2,25 ms
 */

#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"
#include "tsop1838.h"

// ---------------- Hardware usado ----------------
#define IR_PERIPH_GPIO   SYSCTL_PERIPH_GPIOC
#define IR_GPIO_BASE     GPIO_PORTC_BASE
#define IR_GPIO_PIN      GPIO_PIN_6
#define IR_PERIPH_TIMER  SYSCTL_PERIPH_TIMER1
#define IR_TIMER_BASE    TIMER1_BASE

// ---------------- Tiempos entre flancos de bajada (us) ----------------
#define T_INICIO_MIN    12500   // Nominal 13500
#define T_INICIO_MAX    14500
#define T_REP_MIN       10000   // Nominal 11250
#define T_REP_MAX       12500
#define T_BIT0_MIN        800   // Nominal 1125 (bit 0)
#define T_BIT_UMBRAL     1700   // Entre el bit 0 y el bit 1
#define T_BIT1_MAX       2800   // Nominal 2250 (bit 1)
#define T_REP_TIMEOUT  150000   // Maximo entre una trama (o repeticion) y la siguiente repeticion

// ---------------- Variables internas (solo las toca la ISR) ----------------
static uint32_t g_ticks_us;     // Ticks del Timer1 por microsegundo
static uint32_t g_ultimo;       // Valor del timer en el ultimo flanco de bajada
static uint32_t g_t_evento;     // Valor del timer en la ultima trama/repeticion valida
static uint32_t g_buf;          // Bits recibidos (el primero en el bit 0)
static uint8_t  g_nbits;        // Numero de bits recibidos
static bool     g_en_datos;     // false: esperando inicio de trama | true: recibiendo los 32 bits
static bool     g_previa;       // Hay una trama valida reciente (permite aceptar repeticiones)
static uint8_t  g_dir, g_cmd;   // Ultima trama valida

// ---------------- Variables compartidas con IR_Lee ----------------
static volatile IR_Trama g_trama;
static volatile bool     g_nueva;

static void Publica(bool repeticion)
{
    g_trama.direccion  = g_dir;
    g_trama.comando    = g_cmd;
    g_trama.repeticion = repeticion;
    g_nueva = true;
}

// Interrupcion de flanco de bajada en el pin del sensor
static void IR_ISR(void)
{
    uint32_t ahora = TimerValueGet(IR_TIMER_BASE, TIMER_A);   // Cuenta hacia abajo
    GPIOIntClear(IR_GPIO_BASE, IR_GPIO_PIN);

    // Microsegundos desde el flanco anterior (la resta sin signo tambien vale si el timer da la vuelta)
    uint32_t dt = (g_ultimo - ahora) / g_ticks_us;
    g_ultimo = ahora;

    if(!g_en_datos)
    {
        if(dt >= T_INICIO_MIN && dt <= T_INICIO_MAX)         // Inicio de trama
        {
            g_en_datos = true;
            g_nbits = 0;
            g_buf = 0;
        }
        else if(dt >= T_REP_MIN && dt < T_REP_MAX)           // Repeticion
        {
            if(g_previa && ((g_t_evento - ahora) / g_ticks_us) <= T_REP_TIMEOUT)
            {
                g_t_evento = ahora;
                Publica(true);
            }
            else
            {
                g_previa = false;
            }
        }
        // En cualquier otro caso este flanco puede ser el comienzo de una trama: se sigue esperando
    }
    else
    {
        uint32_t bit;

        if(dt >= T_BIT0_MIN && dt < T_BIT_UMBRAL)
        {
            bit = 0;
        }
        else if(dt >= T_BIT_UMBRAL && dt <= T_BIT1_MAX)
        {
            bit = 1;
        }
        else    // Tiempo fuera de rango: se descarta la trama
        {
            g_en_datos = false;
            g_previa = false;
            return;
        }

        g_buf |= bit << g_nbits;
        g_nbits++;

        if(g_nbits == 32)   // Trama completa (este flanco es el de la marca final)
        {
            g_en_datos = false;

            // Byte 2 = comando, byte 3 = comando negado
            if((uint8_t)((g_buf >> 16) ^ (g_buf >> 24)) == 0xFF)
            {
                g_dir = (uint8_t)g_buf;
                g_cmd = (uint8_t)(g_buf >> 16);
                g_previa = true;
                g_t_evento = ahora;
                Publica(false);
            }
            else
            {
                g_previa = false;
            }
        }
    }
}

void IR_Init(uint32_t reloj)
{
    g_ticks_us = reloj / 1000000;
    g_en_datos = false;
    g_previa = false;
    g_nueva = false;

    // Timer1: contador libre de 32 bits a la frecuencia del sistema (cronometro)
    SysCtlPeripheralEnable(IR_PERIPH_TIMER);
    while(!SysCtlPeripheralReady(IR_PERIPH_TIMER));
    TimerClockSourceSet(IR_TIMER_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(IR_TIMER_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(IR_TIMER_BASE, TIMER_A, 0xFFFFFFFF);
    TimerEnable(IR_TIMER_BASE, TIMER_A);
    g_ultimo = g_t_evento = TimerValueGet(IR_TIMER_BASE, TIMER_A);

    // Pin del sensor con interrupcion en flanco de bajada
    SysCtlPeripheralEnable(IR_PERIPH_GPIO);
    while(!SysCtlPeripheralReady(IR_PERIPH_GPIO));
    GPIOPinTypeGPIOInput(IR_GPIO_BASE, IR_GPIO_PIN);
    GPIOIntTypeSet(IR_GPIO_BASE, IR_GPIO_PIN, GPIO_FALLING_EDGE);
    GPIOIntRegister(IR_GPIO_BASE, IR_ISR);      // Registra la ISR y habilita la linea en el NVIC
    GPIOIntClear(IR_GPIO_BASE, IR_GPIO_PIN);
    GPIOIntEnable(IR_GPIO_BASE, IR_GPIO_PIN);

    // Para que siga funcionando con SysCtlSleep()
    SysCtlPeripheralSleepEnable(IR_PERIPH_GPIO);
    SysCtlPeripheralSleepEnable(IR_PERIPH_TIMER);
}

bool IR_Lee(IR_Trama *trama)
{
    bool hay;

    IntMasterDisable();     // Seccion critica muy corta para no leer una trama a medias
    hay = g_nueva;
    if(hay)
    {
        trama->direccion  = g_trama.direccion;
        trama->comando    = g_trama.comando;
        trama->repeticion = g_trama.repeticion;
        g_nueva = false;
    }
    IntMasterEnable();

    return hay;
}

const char* IR_NombreTecla(uint8_t comando)
{
    switch(comando)
    {
    case IR_TECLA_1:            return "1";
    case IR_TECLA_2:            return "2";
    case IR_TECLA_3:            return "3";
    case IR_TECLA_4:            return "4";
    case IR_TECLA_5:            return "5";
    case IR_TECLA_6:            return "6";
    case IR_TECLA_7:            return "7";
    case IR_TECLA_8:            return "8";
    case IR_TECLA_9:            return "9";
    case IR_TECLA_ASTERISCO:    return "*";
    case IR_TECLA_0:            return "0";
    case IR_TECLA_ALMOHADILLA:  return "#";
    case IR_TECLA_ARRIBA:       return "ARRIBA";
    case IR_TECLA_IZQUIERDA:    return "IZQUIERDA";
    case IR_TECLA_OK:           return "OK";
    case IR_TECLA_DERECHA:      return "DERECHA";
    case IR_TECLA_ABAJO:        return "ABAJO";
    default:                    return 0;
    }
}
