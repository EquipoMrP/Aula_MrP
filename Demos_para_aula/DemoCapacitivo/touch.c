#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"
#include "touch.h"

// Ticks (llamadas a TOUCH_Actualizar) que corresponden a cada tiempo.
static uint32_t ms_tick = 20U;
static uint32_t ticks_antirebote = 2U;
static uint32_t ticks_larga = 50U;

// Estado interno.
static bool     estado = false;         // Estado filtrado
static uint32_t cnt_filtro = 0U;        // Ticks seguidos con nivel distinto al filtrado
static uint32_t ticks_tocado = 0U;      // Ticks que lleva tocado
static bool     largo_enviado = false;  // Ya se aviso de la pulsacion larga
static uint32_t toques = 0U;

void TOUCH_Init(uint32_t ms_por_tick)
{
    if (ms_por_tick == 0U)
        ms_por_tick = 1U;
    ms_tick = ms_por_tick;

    // Se redondea hacia arriba y como minimo 1 tick.
    ticks_antirebote = (TOUCH_MS_ANTIREBOTE + ms_tick - 1U) / ms_tick;
    if (ticks_antirebote == 0U)
        ticks_antirebote = 1U;
    ticks_larga = (TOUCH_MS_LARGA + ms_tick - 1U) / ms_tick;

    SysCtlPeripheralEnable(TOUCH_PERIPH);
    while (!SysCtlPeripheralReady(TOUCH_PERIPH));
    GPIOPinTypeGPIOInput(TOUCH_BASE, TOUCH_PIN);

    estado = false;
    cnt_filtro = 0U;
    ticks_tocado = 0U;
    largo_enviado = false;
    toques = 0U;
}

bool TOUCH_LeerCrudo(void)
{
    bool nivel = (GPIOPinRead(TOUCH_BASE, TOUCH_PIN) != 0);
#if TOUCH_ACTIVO_ALTO
    return nivel;
#else
    return !nivel;
#endif
}

touch_evento_t TOUCH_Actualizar(void)
{
    touch_evento_t ev = TOUCH_EV_NADA;
    bool crudo = TOUCH_LeerCrudo();

    // Antirebote: el nivel nuevo debe mantenerse varios ticks seguidos.
    if (crudo != estado)
    {
        cnt_filtro++;
        if (cnt_filtro >= ticks_antirebote)
        {
            cnt_filtro = 0U;
            estado = crudo;
            if (estado)
            {
                ticks_tocado = 0U;
                largo_enviado = false;
                toques++;
                ev = TOUCH_EV_TOQUE;
            }
            else
            {
                ev = TOUCH_EV_SOLTADO;
            }
        }
    }
    else
    {
        cnt_filtro = 0U;
    }

    // Pulsacion larga: se avisa una sola vez por toque.
    if (estado)
    {
        if (ticks_tocado < 0xFFFFFFFFU)
            ticks_tocado++;
        if (!largo_enviado && (ticks_tocado >= ticks_larga))
        {
            largo_enviado = true;
            ev = TOUCH_EV_LARGO;
        }
    }

    return ev;
}

bool TOUCH_Tocado(void)
{
    return estado;
}

uint32_t TOUCH_TiempoTocado_ms(void)
{
    return estado ? (ticks_tocado * ms_tick) : 0U;
}

uint32_t TOUCH_Contador(void)
{
    return toques;
}

void TOUCH_ResetContador(void)
{
    toques = 0U;
}
