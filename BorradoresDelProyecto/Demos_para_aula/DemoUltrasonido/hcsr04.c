#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"
#include "hcsr04.h"

/************************************************************
 * CONFIGURACION (cambiar aqui pines / timer)
 ************************************************************/
#define HCSR04_PERIPH_GPIO   SYSCTL_PERIPH_GPIOM
#define HCSR04_GPIO_BASE     GPIO_PORTM_BASE
#define HCSR04_TRIG_PIN      GPIO_PIN_0         // PM0 -> TRIG
#define HCSR04_ECHO_PIN      GPIO_PIN_1         // PM1 -> ECHO (con divisor de tension)

#define HCSR04_PERIPH_TIMER  SYSCTL_PERIPH_TIMER1
#define HCSR04_TIMER_BASE    TIMER1_BASE        // 32 bits, cuenta atras en marcha libre

#define HCSR04_TRIG_US       12U                // ancho del pulso TRIG (minimo 10 us)
#define HCSR04_MAX_US        23500U             // ~4 m: por encima, fuera de rango
#define HCSR04_TIMEOUT_US    40000U             // espera maxima de HCSR04_Medir

/************************************************************
 * Variables internas
 ************************************************************/
static volatile hcsr04_estado_t g_estado = HCSR04_INACTIVO;
static volatile uint32_t g_t_subida = 0;        // valor del timer en el flanco de subida
static volatile uint32_t g_ticks_eco = 0;       // ancho del eco en ciclos de reloj
static volatile uint32_t g_fallos = 0;

static uint32_t g_ticks_por_us = 1;
static uint32_t g_ticks_max = 0;
static uint32_t g_retardo_trig = 0;             // argumento de SysCtlDelay para TRIG

/************************************************************
 * Interrupcion del puerto del ECHO (ambos flancos)
 ************************************************************/
static void HCSR04_IntGPIO(void)
{
    uint32_t ahora, status, ticks;

    ahora  = TimerValueGet(HCSR04_TIMER_BASE, TIMER_A);   // lo primero: mejor precision
    status = GPIOIntStatus(HCSR04_GPIO_BASE, true);
    GPIOIntClear(HCSR04_GPIO_BASE, status);

    if (!(status & HCSR04_ECHO_PIN))
        return;                                           // otro pin del mismo puerto

    if (GPIOPinRead(HCSR04_GPIO_BASE, HCSR04_ECHO_PIN))
    {
        // Flanco de subida: empieza el eco.
        if (g_estado == HCSR04_ESPERANDO)
        {
            g_t_subida = ahora;
            g_estado = HCSR04_MIDIENDO;
        }
    }
    else
    {
        // Flanco de bajada: termina el eco. El timer cuenta hacia atras.
        if (g_estado == HCSR04_MIDIENDO)
        {
            ticks = g_t_subida - ahora;
            g_ticks_eco = ticks;
            g_estado = (ticks > g_ticks_max) ? HCSR04_FUERA_RANGO : HCSR04_LISTO;
        }
    }
}

/************************************************************
 * Funciones publicas
 ************************************************************/
void HCSR04_Init(uint32_t frecuencia_reloj)
{
    g_ticks_por_us = frecuencia_reloj / 1000000U;
    g_ticks_max    = HCSR04_MAX_US * g_ticks_por_us;
    // SysCtlDelay consume 3 ciclos por iteracion.
    g_retardo_trig = (frecuencia_reloj / 3000000U) * HCSR04_TRIG_US;

    // Timer de 32 bits en marcha libre (cuenta atras desde 0xFFFFFFFF).
    SysCtlPeripheralEnable(HCSR04_PERIPH_TIMER);
    while (!SysCtlPeripheralReady(HCSR04_PERIPH_TIMER));
    TimerClockSourceSet(HCSR04_TIMER_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(HCSR04_TIMER_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(HCSR04_TIMER_BASE, TIMER_A, 0xFFFFFFFFU);
    TimerEnable(HCSR04_TIMER_BASE, TIMER_A);

    // Pines: TRIG salida, ECHO entrada con interrupcion en ambos flancos.
    SysCtlPeripheralEnable(HCSR04_PERIPH_GPIO);
    while (!SysCtlPeripheralReady(HCSR04_PERIPH_GPIO));
    GPIOPinTypeGPIOOutput(HCSR04_GPIO_BASE, HCSR04_TRIG_PIN);
    GPIOPinWrite(HCSR04_GPIO_BASE, HCSR04_TRIG_PIN, 0);
    GPIOPinTypeGPIOInput(HCSR04_GPIO_BASE, HCSR04_ECHO_PIN);
    GPIOIntTypeSet(HCSR04_GPIO_BASE, HCSR04_ECHO_PIN, GPIO_BOTH_EDGES);
    GPIOIntClear(HCSR04_GPIO_BASE, HCSR04_ECHO_PIN);
    GPIOIntRegister(HCSR04_GPIO_BASE, HCSR04_IntGPIO);    // tambien habilita la IRQ en el NVIC
    GPIOIntEnable(HCSR04_GPIO_BASE, HCSR04_ECHO_PIN);

    // Modos de bajo consumo (como en el resto de programas).
    SysCtlPeripheralSleepEnable(HCSR04_PERIPH_GPIO);
    SysCtlPeripheralSleepEnable(HCSR04_PERIPH_TIMER);

    g_estado = HCSR04_INACTIVO;
    g_fallos = 0;
}

void HCSR04_Disparar(void)
{
    // Si la medida anterior no termino, se cuenta como fallo.
    if (g_estado == HCSR04_ESPERANDO || g_estado == HCSR04_MIDIENDO)
        g_fallos++;

    GPIOIntClear(HCSR04_GPIO_BASE, HCSR04_ECHO_PIN);
    g_estado = HCSR04_ESPERANDO;

    // Pulso de disparo.
    GPIOPinWrite(HCSR04_GPIO_BASE, HCSR04_TRIG_PIN, HCSR04_TRIG_PIN);
    SysCtlDelay(g_retardo_trig);
    GPIOPinWrite(HCSR04_GPIO_BASE, HCSR04_TRIG_PIN, 0);
}

hcsr04_estado_t HCSR04_Estado(void)
{
    return g_estado;
}

bool HCSR04_Listo(void)
{
    return (g_estado == HCSR04_LISTO);
}

uint32_t HCSR04_LeerUS(void)
{
    return g_ticks_eco / g_ticks_por_us;
}

// Velocidad del sonido ~343 m/s -> 0,1715 mm por microsegundo de eco (ida y vuelta).
uint32_t HCSR04_LeerMM(void)
{
    return (HCSR04_LeerUS() * 1715U) / 10000U;
}

uint32_t HCSR04_LeerCM(void)
{
    return HCSR04_LeerUS() / 58U;
}

uint32_t HCSR04_Fallos(void)
{
    return g_fallos;
}

bool HCSR04_Medir(uint32_t *distancia_mm)
{
    uint32_t t0 = TimerValueGet(HCSR04_TIMER_BASE, TIMER_A);
    uint32_t limite = HCSR04_TIMEOUT_US * g_ticks_por_us;

    HCSR04_Disparar();

    while (g_estado == HCSR04_ESPERANDO || g_estado == HCSR04_MIDIENDO)
    {
        if ((t0 - TimerValueGet(HCSR04_TIMER_BASE, TIMER_A)) > limite)
        {
            g_fallos++;
            g_estado = HCSR04_TIMEOUT;
            return false;
        }
    }

    if (g_estado != HCSR04_LISTO)
        return false;

    *distancia_mm = HCSR04_LeerMM();
    return true;
}
