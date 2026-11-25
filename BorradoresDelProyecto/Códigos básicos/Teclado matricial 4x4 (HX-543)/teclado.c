#include "teclado.h"

#define TECLADO_FIL_PINES   (TECLADO_FIL1_PIN | TECLADO_FIL2_PIN | \
                             TECLADO_FIL3_PIN | TECLADO_FIL4_PIN)
#define TECLADO_COL_PINES   (TECLADO_COL1_PIN | TECLADO_COL2_PIN | \
                             TECLADO_COL3_PIN | TECLADO_COL4_PIN)

static const uint8_t fil_pin[TECLADO_NFIL] =
{
    TECLADO_FIL1_PIN, TECLADO_FIL2_PIN, TECLADO_FIL3_PIN, TECLADO_FIL4_PIN
};

static const uint8_t col_pin[TECLADO_NCOL] =
{
    TECLADO_COL1_PIN, TECLADO_COL2_PIN, TECLADO_COL3_PIN, TECLADO_COL4_PIN
};

static const char mapa[TECLADO_NFIL][TECLADO_NCOL] =
{
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

static uint32_t ms_tick       = 20U;
static char     tecla_cand    = 0;      // candidata en filtrado de rebotes
static uint8_t  n_igual       = 0U;     // veces seguidas que se ha leido la candidata
static char     tecla_est     = 0;      // tecla estable (0 = ninguna)
static char     tecla_ultima  = 0;
static uint32_t t_ms          = 0U;
static bool     larga_enviada = false;

// Barre las 4 columnas. Devuelve la tecla leida, o 0 si no hay ninguna
// o hay mas de una a la vez.
static char Escanear(void)
{
    uint32_t c, f;
    uint32_t v;
    uint32_t n = 0U;
    char t = 0;

    for (c = 0U; c < TECLADO_NCOL; c++)
    {
        // Columna activa: se configura como salida y se escribe un 0
        // explicitamente (mismo orden que en la prueba que si funciono).
        GPIOPinTypeGPIOOutput(TECLADO_COL_BASE, col_pin[c]);
        GPIOPinWrite(TECLADO_COL_BASE, col_pin[c], 0U);
        SysCtlDelay(TECLADO_RETARDO);

        v = (uint32_t)GPIOPinRead(TECLADO_FIL_BASE, TECLADO_FIL_PINES);

        // Se vuelve a alta impedancia (entrada sin pull-up).
        GPIOPinTypeGPIOInput(TECLADO_COL_BASE, col_pin[c]);

        for (f = 0U; f < TECLADO_NFIL; f++)
        {
            if ((v & fil_pin[f]) == 0U)     // fila a 0 -> tecla pulsada
            {
                n++;
                t = mapa[f][c];
            }
        }
    }

    return (n == 1U) ? t : 0;
}

void TECLADO_Init(uint32_t ms)
{
    ms_tick = ms;

    SysCtlPeripheralEnable(TECLADO_FIL_PERIPH);
    SysCtlPeripheralEnable(TECLADO_COL_PERIPH);
    while (!SysCtlPeripheralReady(TECLADO_FIL_PERIPH));
    while (!SysCtlPeripheralReady(TECLADO_COL_PERIPH));

    // Filas: entradas con pull-up interno.
    GPIOPinTypeGPIOInput(TECLADO_FIL_BASE, TECLADO_FIL_PINES);
    GPIOPadConfigSet(TECLADO_FIL_BASE, TECLADO_FIL_PINES,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    // Columnas: se mantienen como entrada (alta impedancia); solo pasan
    // a salida a 0 mientras se barren (ver Escanear).
    GPIOPinTypeGPIOInput(TECLADO_COL_BASE, TECLADO_COL_PINES);

    tecla_cand    = 0;
    n_igual       = 0U;
    tecla_est     = 0;
    tecla_ultima  = 0;
    t_ms          = 0U;
    larga_enviada = false;
}

teclado_evento_t TECLADO_Actualizar(void)
{
    teclado_evento_t ev = TECLADO_EV_NINGUNO;
    char lec = Escanear();

    // Filtro de rebotes: la lectura debe repetirse TECLADO_N_ESTABLE veces.
    if (lec == tecla_cand)
    {
        if (n_igual < TECLADO_N_ESTABLE)
            n_igual++;
    }
    else
    {
        tecla_cand = lec;
        n_igual = 1U;
    }

    if ((n_igual >= TECLADO_N_ESTABLE) && (tecla_cand != tecla_est))
    {
        tecla_est = tecla_cand;

        if (tecla_est != 0)
        {
            ev = TECLADO_EV_PULSADA;
            tecla_ultima = tecla_est;
            t_ms = 0U;
            larga_enviada = false;
        }
        else
        {
            ev = TECLADO_EV_SOLTADA;
        }
    }

    // Tiempo pulsada y pulsacion larga.
    if (tecla_est != 0)
    {
        if (ev != TECLADO_EV_PULSADA)
            t_ms += ms_tick;

        if ((!larga_enviada) && (t_ms >= TECLADO_MS_LARGA) && (ev == TECLADO_EV_NINGUNO))
        {
            larga_enviada = true;
            ev = TECLADO_EV_LARGA;
        }
    }

    return ev;
}

char TECLADO_Tecla(void)
{
    return tecla_est;
}

char TECLADO_UltimaTecla(void)
{
    return tecla_ultima;
}

bool TECLADO_Pulsada(void)
{
    return (tecla_est != 0);
}

uint32_t TECLADO_TiempoPulsada_ms(void)
{
    return t_ms;
}
