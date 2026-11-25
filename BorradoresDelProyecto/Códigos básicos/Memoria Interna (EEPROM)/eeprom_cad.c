#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"
#include "driverlib/eeprom.h"
#include "eeprom_cad.h"

#define EECAD_MAGIC         0xC0DE5A5BU     // marca de "EEPROM con formato valido"
#define EECAD_LEN_INVALIDA  0xFFFFFFFFU     // hueco en escritura / sin datos

// Palabras de 32 bits necesarias para EECAD_MAX_LEN caracteres.
#define EECAD_PALABRAS      ((EECAD_MAX_LEN + 3U) / 4U)

#define EECAD_DIR_MAGIC     0U              // byte 0
#define EECAD_DIR_TOTAL     4U              // byte 4
#define EECAD_DIR_HUECOS    8U              // byte 8
#define EECAD_BYTES_HUECO   (4U + 4U * EECAD_PALABRAS)

#define DIR_HUECO(i)        (EECAD_DIR_HUECOS + (i) * EECAD_BYTES_HUECO)

static bool iniciada = false;

// Total de cadenas guardadas desde el ultimo borrado (0 si no hay formato).
static uint32_t Total(void)
{
    uint32_t cab[2];

    EEPROMRead(cab, EECAD_DIR_MAGIC, 8U);
    if (cab[0] != EECAD_MAGIC || cab[1] == 0xFFFFFFFFU)
        return 0U;
    return cab[1];
}

bool EECAD_Init(void)
{
    uint32_t r;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_EEPROM0));

    r = EEPROMInit();
    if (r == EEPROM_INIT_RETRY)         // escritura anterior interrumpida
        r = EEPROMInit();

    iniciada = (r == EEPROM_INIT_OK);
    return iniciada;
}

uint32_t EECAD_TamBytes(void)
{
    return EEPROMSizeGet();
}

uint32_t EECAD_NumCadenas(void)
{
    uint32_t total;

    if (!iniciada)
        return 0U;

    total = Total();
    return (total > EECAD_MAX_CADENAS) ? EECAD_MAX_CADENAS : total;
}

bool EECAD_Guardar(const char *cad)
{
    uint32_t datos[EECAD_PALABRAS];
    uint32_t cab[2];
    uint32_t invalida = EECAD_LEN_INVALIDA;
    uint32_t len = 0U;
    uint32_t nPal;
    uint32_t total;
    uint32_t dir;
    uint32_t i;
    uint8_t *p = (uint8_t *)datos;

    if (!iniciada || cad == 0)
        return false;

    // Longitud de la cadena y comprobacion de que cabe.
    while (cad[len] != '\0')
    {
        if (len >= EECAD_MAX_LEN)
            return false;
        len++;
    }

    // Se empaquetan los caracteres en palabras (relleno con ceros).
    for (i = 0U; i < EECAD_PALABRAS; i++)
        datos[i] = 0U;
    for (i = 0U; i < len; i++)
        p[i] = (uint8_t)cad[i];
    nPal = (len + 3U) / 4U;

    // Hueco que toca escribir: el siguiente; si esta lleno, el mas antiguo.
    total = Total();
    dir = DIR_HUECO(total % EECAD_MAX_CADENAS);

    // 1) Se marca el hueco como no valido (por si se corta la alimentacion).
    if (EEPROMProgram(&invalida, dir, 4U) != 0U)
        return false;

    // 2) Se escriben los caracteres.
    if (nPal > 0U)
    {
        if (EEPROMProgram(datos, dir + 4U, nPal * 4U) != 0U)
            return false;
    }

    // 3) Se escribe la longitud: el hueco ya es valido.
    if (EEPROMProgram(&len, dir, 4U) != 0U)
        return false;

    // 4) Se actualiza el total: la cadena nueva ya cuenta.
    cab[0] = EECAD_MAGIC;
    cab[1] = total + 1U;
    if (EEPROMProgram(cab, EECAD_DIR_MAGIC, 8U) != 0U)
        return false;

    return true;
}

int32_t EECAD_Leer(uint32_t indice, char *buf, uint32_t tam)
{
    uint32_t datos[EECAD_PALABRAS];
    uint32_t total;
    uint32_t primera;
    uint32_t dir;
    uint32_t len;
    uint32_t nPal;
    uint32_t i;
    uint8_t *p = (uint8_t *)datos;

    if (!iniciada || buf == 0 || tam == 0U)
        return -1;

    total = Total();
    if (indice >= EECAD_NumCadenas())
        return -1;

    // Numero global de la cadena mas antigua que se conserva.
    primera = (total > EECAD_MAX_CADENAS) ? (total - EECAD_MAX_CADENAS) : 0U;
    dir = DIR_HUECO((primera + indice) % EECAD_MAX_CADENAS);

    EEPROMRead(&len, dir, 4U);
    if (len > EECAD_MAX_LEN)
        return -1;

    nPal = (len + 3U) / 4U;
    if (nPal > 0U)
        EEPROMRead(datos, dir + 4U, nPal * 4U);

    if (len > (tam - 1U))               // se recorta si buf es pequeno
        len = tam - 1U;

    for (i = 0U; i < len; i++)
        buf[i] = (char)p[i];
    buf[len] = '\0';

    return (int32_t)len;
}

bool EECAD_Borrar(void)
{
    uint32_t cab[2];

    if (!iniciada)
        return false;

    cab[0] = EECAD_MAGIC;
    cab[1] = 0U;
    return (EEPROMProgram(cab, EECAD_DIR_MAGIC, 8U) == 0U);
}
