/*
 * tsop1838.h
 *
 * Biblioteca para el receptor infrarrojo TSOP1838 con mando (protocolo NEC)
 * en el TM4C1294.
 *
 * Conexion:  VCC -> 3.3V,  GND -> GND,  OUT -> PC6
 * (el pin y los temporizadores se pueden cambiar en los #define de tsop1838.c)
 *
 * Funcionamiento:
 *  - PC6 genera una interrupcion en cada flanco de bajada de la salida del TSOP.
 *  - Timer1 (32 bits, a la frecuencia del sistema) se usa solo como cronometro
 *    para medir el tiempo entre flancos.
 *  - La interrupcion decodifica la trama NEC completa. El programa principal
 *    no hace polling ni pierde tiempo: solo pregunta con IR_Lee() si hay tecla nueva.
 *  - IR_Init() deja habilitados GPIOC y Timer1 en modo SLEEP, de forma que el
 *    micro puede dormir con SysCtlSleep() y se despierta con cada flanco.
 */

#ifndef TSOP1838_H_
#define TSOP1838_H_

#include <stdint.h>
#include <stdbool.h>

// Codigos de comando de las teclas del mando (direccion 0x00)
#define IR_TECLA_1            0x45
#define IR_TECLA_2            0x46
#define IR_TECLA_3            0x47
#define IR_TECLA_4            0x44
#define IR_TECLA_5            0x40
#define IR_TECLA_6            0x43
#define IR_TECLA_7            0x07
#define IR_TECLA_8            0x15
#define IR_TECLA_9            0x09
#define IR_TECLA_ASTERISCO    0x16
#define IR_TECLA_0            0x19
#define IR_TECLA_ALMOHADILLA  0x0D
#define IR_TECLA_ARRIBA       0x18
#define IR_TECLA_IZQUIERDA    0x08
#define IR_TECLA_OK           0x1C
#define IR_TECLA_DERECHA      0x5A
#define IR_TECLA_ABAJO        0x52

typedef struct {
    uint8_t direccion;    // Byte de direccion del mando
    uint8_t comando;      // Codigo de la tecla pulsada
    bool    repeticion;   // true si es una repeticion (tecla mantenida pulsada)
} IR_Trama;

// Configura el pin, la interrupcion y el Timer1.
// 'reloj' es la frecuencia del sistema en Hz (la que devuelve SysCtlClockFreqSet).
// Las interrupciones globales se habilitan aparte con IntMasterEnable().
void IR_Init(uint32_t reloj);

// Devuelve true si ha llegado una tecla (o repeticion) nueva desde la ultima
// llamada, y la copia en *trama. Si no hay nada nuevo devuelve false.
bool IR_Lee(IR_Trama *trama);

// Devuelve el nombre de la tecla ("1", "OK", "ARRIBA"...) a partir del codigo de comando,
// o NULL (0) si el codigo no corresponde a ninguna tecla conocida.
const char* IR_NombreTecla(uint8_t comando);

#endif /* TSOP1838_H_ */
