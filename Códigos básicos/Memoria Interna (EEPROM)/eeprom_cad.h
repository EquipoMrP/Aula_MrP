#ifndef EEPROM_CAD_H
#define EEPROM_CAD_H

#include <stdint.h>
#include <stdbool.h>

/************************************************************
 * Biblioteca para guardar las ultimas EECAD_MAX_CADENAS
 * cadenas en la EEPROM interna del TM4C1294.
 * Cuando esta llena, la cadena nueva sustituye a la mas antigua.
 *
 * Disposicion en la EEPROM (direcciones en bytes):
 *   0 : marca de validez (4 bytes)
 *   4 : total de cadenas guardadas desde el ultimo borrado (4 bytes)
 *   8 : EECAD_MAX_CADENAS huecos; cada hueco = longitud (4 bytes)
 *       + caracteres (multiplo de 4 bytes)
 ************************************************************/

// Maximo de caracteres de cada cadena (sin contar el '\0').
#define EECAD_MAX_LEN       127U

// Numero de cadenas que se conservan.
#define EECAD_MAX_CADENAS   5U

// Habilita y inicializa la EEPROM. Devuelve true si todo va bien.
bool EECAD_Init(void);

// Tamano total de la EEPROM en bytes (6144 en el TM4C1294).
uint32_t EECAD_TamBytes(void);

// Numero de cadenas guardadas ahora mismo (0 ... EECAD_MAX_CADENAS).
uint32_t EECAD_NumCadenas(void);

// Guarda una cadena nueva. Si ya hay EECAD_MAX_CADENAS, sustituye a la
// mas antigua. Devuelve false si no cabe (> EECAD_MAX_LEN) o si falla.
bool EECAD_Guardar(const char *cad);

// Copia en buf la cadena numero 'indice' (tam = tamano de buf, con el '\0').
// indice 0 = la mas antigua, NumCadenas-1 = la mas reciente.
// Devuelve el numero de caracteres copiados, o -1 si el indice no existe
// o la cadena no es valida.
int32_t EECAD_Leer(uint32_t indice, char *buf, uint32_t tam);

// Borra todas las cadenas.
bool EECAD_Borrar(void);

#endif
