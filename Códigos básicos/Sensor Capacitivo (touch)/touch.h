#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>
#include <stdbool.h>

/************************************************************
 * Biblioteca para sensor capacitivo touch de 3 pines
 * (VCC, GND, salida digital) en TM4C1294.
 *
 *   - La salida del sensor se conecta a un pin GPIO de entrada.
 *   - TOUCH_Actualizar() se llama UNA vez por interrupcion del
 *     timer (periodo fijo): filtra rebotes, detecta toque,
 *     soltado y pulsacion larga, y cuenta los toques.
 *
 * Conexion por defecto:
 *   VCC  -> 3V3
 *   GND  -> GND
 *   I/O  -> PP5  (BoosterPack 2, columna D2, pin 3; se cambia
 *                 con las macros de abajo)
 *
 * Esta biblioteca necesita que driverlib2.h este incluido
 * en touch.c (ya lo hace).
 ************************************************************/

// ---- Pin del sensor (modificable) ----
#define TOUCH_PERIPH        SYSCTL_PERIPH_GPIOP
#define TOUCH_BASE          GPIO_PORTP_BASE
#define TOUCH_PIN           GPIO_PIN_5

// Nivel de la salida cuando se toca: 1 = alto (lo normal), 0 = bajo.
#define TOUCH_ACTIVO_ALTO   1

// ---- Tiempos (modificables) ----
#define TOUCH_MS_ANTIREBOTE 40U     // El nivel debe mantenerse este tiempo
#define TOUCH_MS_LARGA      1000U   // Tiempo para considerar pulsacion larga

// Eventos que devuelve TOUCH_Actualizar().
typedef enum
{
    TOUCH_EV_NADA = 0,      // No ha ocurrido nada en esta llamada
    TOUCH_EV_TOQUE,         // Se acaba de tocar el sensor
    TOUCH_EV_LARGO,         // Se mantiene tocado mas de TOUCH_MS_LARGA (una vez)
    TOUCH_EV_SOLTADO        // Se acaba de soltar el sensor
} touch_evento_t;

// Configura el GPIO. ms_por_tick = periodo con el que se llamara a
// TOUCH_Actualizar() (por ejemplo 20 para el Timer0A a 50 Hz).
void TOUCH_Init(uint32_t ms_por_tick);

// Lee el pin directamente, sin filtrar (true = tocado).
bool TOUCH_LeerCrudo(void);

// Llamar una vez por periodo del timer. Devuelve el evento ocurrido.
touch_evento_t TOUCH_Actualizar(void);

// Estado filtrado actual (true = tocado).
bool TOUCH_Tocado(void);

// Tiempo que lleva tocado en ms (0 si no esta tocado).
uint32_t TOUCH_TiempoTocado_ms(void);

// Numero de toques desde el arranque o desde TOUCH_ResetContador().
uint32_t TOUCH_Contador(void);
void     TOUCH_ResetContador(void);

#endif // TOUCH_H
