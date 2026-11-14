#ifndef TECLADO_H_
#define TECLADO_H_

#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"

/************************************************************
 * Biblioteca para teclado matricial 4x4
 *
 *   - 4 filas (entradas con pull-up interno) y 4 columnas.
 *   - Barrido por software: se baja una columna cada vez y se
 *     leen las 4 filas. Las columnas no activas quedan en alta
 *     impedancia (entrada), asi no hay cortocircuitos si se
 *     pulsan dos teclas a la vez.
 *   - TECLADO_Actualizar() se llama una vez por tick (p. ej.
 *     cada 20 ms desde el bucle principal tras SLEEP). Filtra
 *     rebotes y devuelve un evento por llamada.
 *   - Si se detecta mas de una tecla a la vez se ignora la
 *     lectura (no se puede distinguir con una matriz sin diodos).
 *
 *   Disposicion de teclas (la habitual de los 4x4):
 *
 *            Col1 Col2 Col3 Col4
 *     Fila 1   1    2    3    A
 *     Fila 2   4    5    6    B
 *     Fila 3   7    8    9    C
 *     Fila 4   *    0    #    D
 *
 *   Conexion (BoosterPack 2, nombres de la guia EK-TM4C1294XL):
 *
 *     Fila 1    -> PK0   (B2 pin 5)
 *     Fila 2    -> PK1   (B2 pin 6)
 *     Fila 3    -> PK2   (B2 pin 7)
 *     Fila 4    -> PK3   (B2 pin 8)
 *     Columna 1 -> PK4   (C2 pin 2)
 *     Columna 2 -> PK5   (C2 pin 3)
 *     Columna 3 -> PK6   (C2 pin 9)
 *     Columna 4 -> PK7   (C2 pin 10)
 *
 *   El teclado es pasivo: no necesita VCC. Solo 8 cables.
 *   Los pines se cambian en las macros de abajo.
 ************************************************************/

// ---------- Pines (cambiar aqui) ----------
#define TECLADO_FIL_PERIPH  SYSCTL_PERIPH_GPIOK
#define TECLADO_FIL_BASE    GPIO_PORTK_BASE
#define TECLADO_FIL1_PIN    GPIO_PIN_0
#define TECLADO_FIL2_PIN    GPIO_PIN_1
#define TECLADO_FIL3_PIN    GPIO_PIN_2
#define TECLADO_FIL4_PIN    GPIO_PIN_3

#define TECLADO_COL_PERIPH  SYSCTL_PERIPH_GPIOK
#define TECLADO_COL_BASE    GPIO_PORTK_BASE
#define TECLADO_COL1_PIN    GPIO_PIN_4
#define TECLADO_COL2_PIN    GPIO_PIN_5
#define TECLADO_COL3_PIN    GPIO_PIN_6
#define TECLADO_COL4_PIN    GPIO_PIN_7

// ---------- Parametros ----------
#define TECLADO_NFIL        4U
#define TECLADO_NCOL        4U
#define TECLADO_N_ESTABLE   2U      // lecturas iguales seguidas para dar una tecla por valida
#define TECLADO_MS_LARGA    1000U   // tiempo para considerar pulsacion larga (ms)
#define TECLADO_RETARDO     1200U   // espera tras bajar la columna (SysCtlDelay, ~30 us a 120 MHz)

// ---------- Eventos ----------
typedef enum
{
    TECLADO_EV_NINGUNO = 0,
    TECLADO_EV_PULSADA,     // tecla nueva (ya sin rebotes)
    TECLADO_EV_LARGA,       // la tecla sigue pulsada tras TECLADO_MS_LARGA
    TECLADO_EV_SOLTADA      // se ha soltado la tecla
} teclado_evento_t;

// ---------- Funciones ----------

// Configura los pines. ms_tick = ms que pasan entre llamadas a TECLADO_Actualizar().
void TECLADO_Init(uint32_t ms_tick);

// Llamar una vez por tick. Devuelve el evento ocurrido en este tick.
teclado_evento_t TECLADO_Actualizar(void);

// Tecla pulsada ahora ('0'-'9', 'A'-'D', '*', '#') o 0 si no hay ninguna.
char TECLADO_Tecla(void);

// Ultima tecla que se pulso (sigue valida despues de soltarla). 0 si no se ha pulsado ninguna.
char TECLADO_UltimaTecla(void);

// true si hay una tecla pulsada.
bool TECLADO_Pulsada(void);

// Tiempo (ms) que ha estado pulsada la ultima tecla. Se conserva al soltarla.
uint32_t TECLADO_TiempoPulsada_ms(void);

#endif /* TECLADO_H_ */
