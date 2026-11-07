#include <stdint.h>
#include <stdbool.h>

#include "driverlib2.h"
#include "ds3231.h"

/*
 * Libreria DS3231 (modulo HW-084) para TM4C1294.
 * Usa el controlador I2C2 hardware: PN5 = SCL, PN4 = SDA (100 kHz).
 */
#define DS3231_ADDRESS             0x68U
#define DS3231_REG_SECONDS         0x00U
#define DS3231_REG_STATUS          0x0FU
#define DS3231_I2C_BASE            I2C2_BASE
#define DS3231_I2C_TIMEOUT_LOOPS   1000000UL
/*
 * A 120 MHz el controlador I2C tarda un tiempo en reaccionar a un comando:
 * el bit BUSY no se activa de inmediato (la maquina de estados avanza al
 * ritmo del reloj I2C, ~1 us por paso a 100 kHz). Si se consulta BUSY antes,
 * parece "libre" y se lee el registro de datos ANTES de que llegue el byte
 * nuevo (cada lectura devuelve el dato de la lectura anterior).
 * Por eso: (1) se espera ~30 us y (2) se espera el flag de "operacion
 * terminada" (MRIS) antes de mirar BUSY.
 * SysCtlDelay(n) tarda 3*n ciclos: 1200 -> 3600 ciclos = 30 us a 120 MHz
 * (un byte a 100 kHz dura unos 90 us, asi que BUSY seguira activo).
 */
#define DS3231_I2C_SETTLE_LOOPS    1200U

static DS3231_Error s_lastError = DS3231_ERROR_NONE;

/* Configura los pines PN5/PN4 y el controlador I2C2 en modo Standard (100 kHz). */
void DS3231_Init(uint32_t sysClock)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C2));

    GPIOPinConfigure(GPIO_PN5_I2C2SCL);
    GPIOPinConfigure(GPIO_PN4_I2C2SDA);
    GPIOPinTypeI2CSCL(GPIO_PORTN_BASE, GPIO_PIN_5);
    GPIOPinTypeI2C(GPIO_PORTN_BASE, GPIO_PIN_4);

    I2CMasterInitExpClk(DS3231_I2C_BASE, sysClock, false);
}

/* Borra el flag de fin de operacion y lanza el comando. */
static void DS3231_Control(uint32_t command)
{
    I2CMasterIntClear(DS3231_I2C_BASE);
    I2CMasterControl(DS3231_I2C_BASE, command);
}

static void DS3231_SetI2CError(uint32_t hwError)
{
    if ((hwError & I2C_MASTER_ERR_ADDR_ACK) != 0U)
    {
        s_lastError = DS3231_ERROR_I2C_ADDRESS_NACK;
    }
    else if ((hwError & I2C_MASTER_ERR_DATA_ACK) != 0U)
    {
        s_lastError = DS3231_ERROR_I2C_DATA_NACK;
    }
    else if ((hwError & I2C_MASTER_ERR_ARB_LOST) != 0U)
    {
        s_lastError = DS3231_ERROR_I2C_ARBITRATION_LOST;
    }
    else if ((hwError & I2C_MASTER_ERR_CLK_TOUT) != 0U)
    {
        s_lastError = DS3231_ERROR_TIMEOUT;
    }
    else
    {
        s_lastError = DS3231_ERROR_I2C_OTHER;
    }
}

static bool DS3231_WaitBusyOnly(void)
{
    uint32_t timeout = DS3231_I2C_TIMEOUT_LOOPS;

    /* Dar tiempo a que el controlador arranque el comando. */
    SysCtlDelay(DS3231_I2C_SETTLE_LOOPS);

    /* 1) Esperar el flag crudo de "operacion terminada" (o error). */
    while (!I2CMasterIntStatus(DS3231_I2C_BASE, false))
    {
        if (timeout == 0U)
        {
            s_lastError = DS3231_ERROR_TIMEOUT;
            return false;
        }
        timeout--;
    }

    /* 2) Esperar a que el controlador quede libre. */
    timeout = DS3231_I2C_TIMEOUT_LOOPS;
    while (I2CMasterBusy(DS3231_I2C_BASE))
    {
        if (timeout == 0U)
        {
            s_lastError = DS3231_ERROR_TIMEOUT;
            return false;
        }
        timeout--;
    }

    return true;
}

static bool DS3231_WaitCommand(void)
{
    uint32_t hwError;

    if (!DS3231_WaitBusyOnly())
    {
        return false;
    }

    hwError = I2CMasterErr(DS3231_I2C_BASE);
    if (hwError != I2C_MASTER_ERR_NONE)
    {
        DS3231_SetI2CError(hwError);
        return false;
    }

    s_lastError = DS3231_ERROR_NONE;
    return true;
}

static bool DS3231_WaitBusIdle(void)
{
    uint32_t timeout = DS3231_I2C_TIMEOUT_LOOPS;

    while (I2CMasterBusBusy(DS3231_I2C_BASE))
    {
        if (timeout == 0U)
        {
            s_lastError = DS3231_ERROR_BUS_BUSY;
            return false;
        }
        timeout--;
    }

    return true;
}

static void DS3231_StopAfterSendError(void)
{
    DS3231_Error savedError = s_lastError;

    DS3231_Control(I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
    (void)DS3231_WaitBusyOnly();
    s_lastError = savedError;
}

static void DS3231_StopAfterReceiveError(void)
{
    DS3231_Error savedError = s_lastError;

    DS3231_Control(I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP);
    (void)DS3231_WaitBusyOnly();
    s_lastError = savedError;
}

/*
 * Escritura I2C hardware.
 * buffer[0] es el registro interno del DS3231 y el resto son sus datos.
 * La secuencia es START + direccion + bytes + STOP.
 */
static bool DS3231_I2CWrite(const uint8_t *buffer, uint8_t length)
{
    uint8_t i;

    if ((buffer == 0) || (length == 0U))
    {
        s_lastError = DS3231_ERROR_INVALID_ARGUMENT;
        return false;
    }

    if (!DS3231_WaitBusIdle())
    {
        return false;
    }

    I2CMasterSlaveAddrSet(DS3231_I2C_BASE, DS3231_ADDRESS, false);

    for (i = 0U; i < length; i++)
    {
        I2CMasterDataPut(DS3231_I2C_BASE, buffer[i]);

        if (length == 1U)
        {
            DS3231_Control(I2C_MASTER_CMD_SINGLE_SEND);
        }
        else if (i == 0U)
        {
            DS3231_Control(I2C_MASTER_CMD_BURST_SEND_START);
        }
        else if (i == (uint8_t)(length - 1U))
        {
            DS3231_Control(I2C_MASTER_CMD_BURST_SEND_FINISH);
        }
        else
        {
            DS3231_Control(I2C_MASTER_CMD_BURST_SEND_CONT);
        }

        if (!DS3231_WaitCommand())
        {
            if (length > 1U && s_lastError != DS3231_ERROR_I2C_ARBITRATION_LOST)
            {
                DS3231_StopAfterSendError();
            }
            return false;
        }
    }

    s_lastError = DS3231_ERROR_NONE;
    return true;
}

/*
 * Lectura de registros del DS3231.
 * Primero escribe el puntero de registro sin STOP (BURST_SEND_START) y
 * despues genera START repetido para recibir los bytes.
 */
static bool DS3231_I2CRead(uint8_t reg, uint8_t *buffer, uint8_t length)
{
    uint8_t i;

    if ((buffer == 0) || (length == 0U))
    {
        s_lastError = DS3231_ERROR_INVALID_ARGUMENT;
        return false;
    }

    if (!DS3231_WaitBusIdle())
    {
        return false;
    }

    /* Fase de escritura: seleccionar registro y mantener el bus ocupado. */
    I2CMasterSlaveAddrSet(DS3231_I2C_BASE, DS3231_ADDRESS, false);
    I2CMasterDataPut(DS3231_I2C_BASE, reg);
    DS3231_Control(I2C_MASTER_CMD_BURST_SEND_START);

    if (!DS3231_WaitCommand())
    {
        if (s_lastError != DS3231_ERROR_I2C_ARBITRATION_LOST)
        {
            DS3231_StopAfterSendError();
        }
        return false;
    }

    /* START repetido: misma direccion, ahora en modo receptor. */
    I2CMasterSlaveAddrSet(DS3231_I2C_BASE, DS3231_ADDRESS, true);

    if (length == 1U)
    {
        DS3231_Control(I2C_MASTER_CMD_SINGLE_RECEIVE);
        if (!DS3231_WaitCommand())
        {
            if (s_lastError != DS3231_ERROR_I2C_ARBITRATION_LOST)
            {
                DS3231_StopAfterReceiveError();
            }
            return false;
        }
        buffer[0] = (uint8_t)I2CMasterDataGet(DS3231_I2C_BASE);
    }
    else
    {
        DS3231_Control(I2C_MASTER_CMD_BURST_RECEIVE_START);
        if (!DS3231_WaitCommand())
        {
            if (s_lastError != DS3231_ERROR_I2C_ARBITRATION_LOST)
            {
                DS3231_StopAfterReceiveError();
            }
            return false;
        }
        buffer[0] = (uint8_t)I2CMasterDataGet(DS3231_I2C_BASE);

        for (i = 1U; i < length; i++)
        {
            if (i == (uint8_t)(length - 1U))
            {
                DS3231_Control(I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
            }
            else
            {
                DS3231_Control(I2C_MASTER_CMD_BURST_RECEIVE_CONT);
            }

            if (!DS3231_WaitCommand())
            {
                if (s_lastError != DS3231_ERROR_I2C_ARBITRATION_LOST)
                {
                    DS3231_StopAfterReceiveError();
                }
                return false;
            }
            buffer[i] = (uint8_t)I2CMasterDataGet(DS3231_I2C_BASE);
        }
    }

    s_lastError = DS3231_ERROR_NONE;
    return true;
}

static bool DS3231_ReadRegister(uint8_t reg, uint8_t *value)
{
    return DS3231_I2CRead(reg, value, 1U);
}

static bool DS3231_WriteRegister(uint8_t reg, uint8_t value)
{
    uint8_t buffer[2];

    buffer[0] = reg;
    buffer[1] = value;
    return DS3231_I2CWrite(buffer, 2U);
}

DS3231_Error DS3231_GetLastError(void)
{
    return s_lastError;
}

static uint8_t DS3231_DecToBcd(uint8_t value)
{
    return (uint8_t)(((value / 10U) << 4) | (value % 10U));
}

static uint8_t DS3231_BcdToDec(uint8_t value)
{
    return (uint8_t)(((value >> 4) * 10U) + (value & 0x0FU));
}

static bool DS3231_IsBcd(uint8_t value)
{
    return (((value & 0x0FU) <= 9U) && (((value >> 4) & 0x0FU) <= 9U));
}

static bool DS3231_IsLeapYear(uint16_t year)
{
    return (((year % 4U) == 0U) && ((year % 100U) != 0U)) ||
           ((year % 400U) == 0U);
}

static uint8_t DS3231_DaysInMonth(uint8_t month, uint16_t year)
{
    static const uint8_t days[12] =
        {31U, 28U, 31U, 30U, 31U, 30U,
         31U, 31U, 30U, 31U, 30U, 31U};

    if ((month < 1U) || (month > 12U))
    {
        return 0U;
    }
    if ((month == 2U) && DS3231_IsLeapYear(year))
    {
        return 29U;
    }
    return days[month - 1U];
}

static bool DS3231_IsValidDateTime(const DS3231_DateTime *dateTime)
{
    if (dateTime == 0)
    {
        return false;
    }

    if ((dateTime->seconds > 59U) ||
        (dateTime->minutes > 59U) ||
        (dateTime->hours > 23U) ||
        (dateTime->dayOfWeek < 1U) || (dateTime->dayOfWeek > 7U) ||
        (dateTime->month < 1U) || (dateTime->month > 12U) ||
        (dateTime->year < 2000U) || (dateTime->year > 2199U))
    {
        return false;
    }

    if ((dateTime->day < 1U) ||
        (dateTime->day > DS3231_DaysInMonth(dateTime->month, dateTime->year)))
    {
        return false;
    }

    return true;
}

bool DS3231_ReadDateTime(DS3231_DateTime *dateTime)
{
    uint8_t raw[7];
    uint8_t hoursRaw;
    uint8_t hour;
    uint16_t year;

    if (dateTime == 0)
    {
        s_lastError = DS3231_ERROR_INVALID_ARGUMENT;
        return false;
    }

    if (!DS3231_I2CRead(DS3231_REG_SECONDS, raw, 7U))
    {
        return false;
    }

    /* Comprobar BCD y bits reservados antes de convertir. */
    if (((raw[0] & 0x80U) != 0U) ||
        ((raw[1] & 0x80U) != 0U) ||
        ((raw[2] & 0x80U) != 0U) ||
        ((raw[3] & 0xF8U) != 0U) ||
        ((raw[4] & 0xC0U) != 0U) ||
        ((raw[5] & 0x60U) != 0U) ||
        !DS3231_IsBcd((uint8_t)(raw[0] & 0x7FU)) ||
        !DS3231_IsBcd((uint8_t)(raw[1] & 0x7FU)) ||
        !DS3231_IsBcd((uint8_t)(raw[4] & 0x3FU)) ||
        !DS3231_IsBcd((uint8_t)(raw[5] & 0x1FU)) ||
        !DS3231_IsBcd(raw[6]))
    {
        s_lastError = DS3231_ERROR_INVALID_DATA;
        return false;
    }

    dateTime->seconds = DS3231_BcdToDec((uint8_t)(raw[0] & 0x7FU));
    dateTime->minutes = DS3231_BcdToDec((uint8_t)(raw[1] & 0x7FU));

    /* El registro de horas permite formatos de 12 y de 24 horas. */
    hoursRaw = raw[2];
    if ((hoursRaw & 0x40U) != 0U)
    {
        if (!DS3231_IsBcd((uint8_t)(hoursRaw & 0x1FU)))
        {
            s_lastError = DS3231_ERROR_INVALID_DATA;
            return false;
        }

        hour = DS3231_BcdToDec((uint8_t)(hoursRaw & 0x1FU));
        if ((hour < 1U) || (hour > 12U))
        {
            s_lastError = DS3231_ERROR_INVALID_DATA;
            return false;
        }
        if (hour == 12U)
        {
            hour = 0U;
        }
        if ((hoursRaw & 0x20U) != 0U)
        {
            hour = (uint8_t)(hour + 12U);
        }
    }
    else
    {
        if (!DS3231_IsBcd((uint8_t)(hoursRaw & 0x3FU)))
        {
            s_lastError = DS3231_ERROR_INVALID_DATA;
            return false;
        }
        hour = DS3231_BcdToDec((uint8_t)(hoursRaw & 0x3FU));
    }
    dateTime->hours = hour;

    dateTime->dayOfWeek = (uint8_t)(raw[3] & 0x07U);
    dateTime->day = DS3231_BcdToDec((uint8_t)(raw[4] & 0x3FU));
    dateTime->month = DS3231_BcdToDec((uint8_t)(raw[5] & 0x1FU));

    year = (uint16_t)DS3231_BcdToDec(raw[6]);
    if ((raw[5] & 0x80U) != 0U)
    {
        year = (uint16_t)(year + 100U);
    }
    dateTime->year = (uint16_t)(2000U + year);

    if (!DS3231_IsValidDateTime(dateTime))
    {
        s_lastError = DS3231_ERROR_INVALID_DATA;
        return false;
    }

    s_lastError = DS3231_ERROR_NONE;
    return true;
}

bool DS3231_WriteDateTime(const DS3231_DateTime *dateTime)
{
    uint8_t buffer[8];
    uint16_t yearOffset;

    if (!DS3231_IsValidDateTime(dateTime))
    {
        s_lastError = DS3231_ERROR_INVALID_ARGUMENT;
        return false;
    }

    yearOffset = (uint16_t)(dateTime->year - 2000U);

    /* Registro inicial + 7 bytes. */
    buffer[0] = DS3231_REG_SECONDS;
    buffer[1] = DS3231_DecToBcd(dateTime->seconds);
    buffer[2] = DS3231_DecToBcd(dateTime->minutes);
    buffer[3] = DS3231_DecToBcd(dateTime->hours);
    buffer[4] = DS3231_DecToBcd(dateTime->dayOfWeek);
    buffer[5] = DS3231_DecToBcd(dateTime->day);
    buffer[6] = (uint8_t)(DS3231_DecToBcd(dateTime->month) |
                          ((yearOffset >= 100U) ? 0x80U : 0x00U));
    buffer[7] = DS3231_DecToBcd((uint8_t)(yearOffset % 100U));

    return DS3231_I2CWrite(buffer, 8U);
}

bool DS3231_ClearOscillatorStopFlag(void)
{
    uint8_t status;

    if (!DS3231_ReadRegister(DS3231_REG_STATUS, &status))
    {
        return false;
    }

    status = (uint8_t)(status & 0x7FU);
    return DS3231_WriteRegister(DS3231_REG_STATUS, status);
}
