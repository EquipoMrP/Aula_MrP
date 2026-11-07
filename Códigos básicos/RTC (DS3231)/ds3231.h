#ifndef DS3231_H_
#define DS3231_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t  seconds;
    uint8_t  minutes;
    uint8_t  hours;
    uint8_t  dayOfWeek;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;       /* 2000..2199 */
} DS3231_DateTime;

typedef enum
{
    DS3231_ERROR_NONE = 0,
    DS3231_ERROR_I2C_ADDRESS_NACK = 1,
    DS3231_ERROR_I2C_DATA_NACK = 2,
    DS3231_ERROR_I2C_ARBITRATION_LOST = 3,
    DS3231_ERROR_I2C_OTHER = 4,
    DS3231_ERROR_TIMEOUT = 5,
    DS3231_ERROR_INVALID_DATA = 6,
    DS3231_ERROR_INVALID_ARGUMENT = 7,
    DS3231_ERROR_BUS_BUSY = 8
} DS3231_Error;

/* Requiere que main.c haya inicializado I2C2 hardware (100 kHz). */
bool DS3231_ReadDateTime(DS3231_DateTime *dateTime);
bool DS3231_ReadRawDateTime(uint8_t raw[7]);
bool DS3231_ReadRegisters(uint8_t reg, uint8_t *buffer, uint8_t length); /* diagnostico */
bool DS3231_WriteRegisterByte(uint8_t reg, uint8_t value);                  /* diagnostico */
bool DS3231_WriteDateTime(const DS3231_DateTime *dateTime);
bool DS3231_ReadOscillatorStopFlag(bool *stopped);
bool DS3231_ClearOscillatorStopFlag(void);
DS3231_Error DS3231_GetLastError(void);

#endif /* DS3231_H_ */
