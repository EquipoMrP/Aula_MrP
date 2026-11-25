#ifndef HCSR04_H_
#define HCSR04_H_

#include <stdint.h>
#include <stdbool.h>

/************************************************************
 * Biblioteca HC-SR04 para TM4C1294
 *
 *   - TRIG: salida GPIO (pulso de ~12 us).
 *   - ECHO: entrada GPIO con interrupcion por ambos flancos.
 *   - Timer1 (32 bits) en marcha libre: mide el ancho del eco.
 *
 * Los pines y el timer se cambian en la zona de configuracion
 * del principio de hcsr04.c.
 ************************************************************/

typedef enum
{
    HCSR04_INACTIVO = 0,    // todavia no se ha lanzado ninguna medida
    HCSR04_ESPERANDO,       // pulso TRIG enviado, esperando flanco de subida
    HCSR04_MIDIENDO,        // eco en curso (pin ECHO a nivel alto)
    HCSR04_LISTO,           // medida valida disponible
    HCSR04_FUERA_RANGO,     // eco demasiado largo (> ~4 m o sin objeto)
    HCSR04_TIMEOUT          // no llego eco (solo la usa HCSR04_Medir)
} hcsr04_estado_t;

// Configura GPIO, interrupcion y Timer1. Llamar tras SysCtlClockFreqSet().
// 'frecuencia_reloj' es el valor devuelto por SysCtlClockFreqSet (RELOJ).
void HCSR04_Init(uint32_t frecuencia_reloj);

// Lanza una medida SIN bloquear: envia el pulso TRIG y vuelve.
// El resultado lo deja la interrupcion; consultar con HCSR04_Estado().
// Dejar al menos 60 ms entre dos medidas consecutivas.
void HCSR04_Disparar(void);

// Estado de la ultima medida lanzada.
hcsr04_estado_t HCSR04_Estado(void);

// true si hay una medida valida (estado LISTO).
bool HCSR04_Listo(void);

// Resultado de la ultima medida valida.
uint32_t HCSR04_LeerUS(void);   // ancho del eco en microsegundos
uint32_t HCSR04_LeerMM(void);   // distancia en milimetros
uint32_t HCSR04_LeerCM(void);   // distancia en centimetros (entero)

// Numero de medidas que se quedaron sin eco (diagnostico).
uint32_t HCSR04_Fallos(void);

// Version BLOQUEANTE: dispara y espera (maximo ~40 ms).
// Devuelve true y la distancia en mm si la medida es valida.
bool HCSR04_Medir(uint32_t *distancia_mm);

#endif /* HCSR04_H_ */
