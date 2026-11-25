#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "driverlib2.h"
#include "utils/uartstdio.h"
#include "FT800_TIVA.h"
#include "imagen_jpeg.h"      // const unsigned char mi_imagen_jpeg[]  (JPEG baseline)

const int32_t REG_CAL[6] = {CAL_DEFAULTS};
int RELOJ;
#define FREC 20

volatile char Cambia = 0;
volatile int Flag_ints = 0;
volatile int cont_int = 0;

void SysCtlSleepFake(void)
{
    while(!Flag_ints);
    Flag_ints = 0;
}

#define SLEEP SysCtlSleepFake()
void IntTimer(void);

/* ---------------------------------------------------------------
 * Imagen JPEG con CMD_LOADIMAGE
 * - CMD_LOADIMAGE ya esta definido en FT800_TIVA.h
 * - Solo vale JPEG baseline (no progresivo). El FT800 no admite PNG.
 * - Se descomprime en RAM_G como RGB565 (2 bytes/pixel): ancho*alto*2 <= 256 KB
 * - Los datos se mandan por trozos porque el FIFO de comandos es de 4096 bytes
 * --------------------------------------------------------------- */
#define IMG_DIR     0          // direccion en RAM_G donde se guarda la imagen
#define CHUNK       512        // bytes por trozo (multiplo de 4)
#define OPT_RGB565  0

unsigned int ImgAncho, ImgAlto;

// Lee ancho y alto de la cabecera JPEG (marcador SOF0 = 0xFFC0). Devuelve 0 si no es baseline
int JPEG_Medidas(const unsigned char *d, unsigned long n, unsigned int *w, unsigned int *h)
{
    unsigned long i = 2;                       // salta SOI (FF D8)
    while(i + 9 < n)
    {
        unsigned int len;
        if(d[i] != 0xFF) return 0;
        len = ((unsigned int)d[i+2] << 8) | d[i+3];
        if(d[i+1] == 0xC0)                     // SOF0: baseline
        {
            *h = ((unsigned int)d[i+5] << 8) | d[i+6];
            *w = ((unsigned int)d[i+7] << 8) | d[i+8];
            return 1;
        }
        if(d[i+1] == 0xC2) return 0;           // progresivo: no soportado
        i += 2 + len;
    }
    return 0;
}

// Espera a que el coprocesador termine. Devuelve 0 si hay fallo (REG_CMD_READ = 0xFFF)
int EsperaCoprocesador(void)
{
    unsigned long rd, wr;
    do {
        rd = Lee_Reg(REG_CMD_READ);
        wr = Lee_Reg(REG_CMD_WRITE);
        if(rd == 0xFFF) return 0;
    } while(rd != wr);
    CMD_Offset = wr;
    return 1;
}

// Espera a que haya n bytes libres en el FIFO de comandos. Devuelve 0 si hay fallo
int EsperaHueco(unsigned int n)
{
    unsigned long rd, usados;
    do {
        rd = Lee_Reg(REG_CMD_READ);
        if(rd == 0xFFF) return 0;
        usados = (CMD_Offset - rd) & 0xFFF;
    } while((4092 - usados) < n);
    return 1;
}

// Carga un JPEG en RAM_G (se hace UNA sola vez). Devuelve 1 si va bien
int ComLoadImageJPEG(unsigned long ptr, unsigned long options, const unsigned char *data, unsigned long length)
{
    unsigned long i = 0;
    unsigned int n, j;

    if(!EsperaCoprocesador()) return 0;

    EscribeRam32(CMD_LOADIMAGE);
    EscribeRam32(ptr);
    EscribeRam32(options);

    while(i < length)
    {
        n = (length - i > CHUNK) ? CHUNK : (unsigned int)(length - i);
        if(!EsperaHueco(n)) return 0;
        for(j = 0; j < n; j++) EscribeRam8(data[i + j]);
        i += n;
        if(i >= length) PadFIFO();             // alinea a 4 bytes al final
        Ejecuta_Lista();                       // actualiza REG_CMD_WRITE
    }
    return EsperaCoprocesador();
}

// Funcion para enviar vertices de mapas de bits (VERTEX2II)
void ComVertex2II(int x, int y, int handle, int cell)
{
    unsigned long cmd = 0x80000000 | ((x & 0x1FF) << 21) | ((y & 0x1FF) << 12) | ((handle & 0x1F) << 7) | (cell & 0x7F);
    EscribeRam32(cmd);
}

// Dibuja la imagen ya cargada en el centro de la pantalla
void Dibuja_imagen(void)
{
    int x = (HSIZE - (int)ImgAncho) / 2;
    int y = (VSIZE - (int)ImgAlto) / 2;

    Nueva_pantalla(0, 0, 0);                   // ComEsperaFin + CMD_DLSTART + clear negro

    // Como se cargo con OPT_NODL hay que decir donde esta y como es el bitmap
    EscribeRam32(0x01000000 | IMG_DIR);                                          // BITMAP_SOURCE
    EscribeRam32(0x07000000 | (7UL << 19) | ((ImgAncho * 2UL) << 9) | ImgAlto);  // BITMAP_LAYOUT (RGB565 = 7)
    EscribeRam32(0x08000000 | ((unsigned long)ImgAncho << 9) | ImgAlto);         // BITMAP_SIZE

    EscribeRam32(CMD_BEGIN_BMP);
    ComVertex2II(x, y, 0, 0);
    EscribeRam32(CMD_END);

    Dibuja();                                  // DISPLAY + SWAP + ejecutar lista
}

int main(void)
{
    int i;

    // 1. Reloj del sistema a 120 MHz
    RELOJ = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    // 2. Timer0 para interrupciones periodicas (20 Hz)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, (RELOJ/FREC)-1);
    TimerIntRegister(TIMER0_BASE, TIMER_A, IntTimer);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);

    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralClockGating(true);

    // 3. Pantalla FT800 por SPI
    SysCtlDelay(RELOJ/3);
    HAL_Init_SPI(2, RELOJ);
    Inicia_pantalla();

    // 4. Retroiluminacion
    Esc_Reg(REG_PWM_DUTY, 128);

    // 5. Calibracion tactil inicial
    for(i = 0; i < 6; i++) {
        Esc_Reg(REG_TOUCH_TRANSFORM_A + 4 * i, REG_CAL[i]);
    }

    // 6. Medidas de la imagen y carga en RAM_G (una sola vez)
    if(!JPEG_Medidas(mi_imagen_jpeg, sizeof(mi_imagen_jpeg), &ImgAncho, &ImgAlto)) while(1);   // JPEG no valido
    if((unsigned long)ImgAncho * ImgAlto * 2 > 262144UL) while(1);                             // no cabe en RAM_G
    if(!ComLoadImageJPEG(IMG_DIR, OPT_RGB565 | OPT_NODL, mi_imagen_jpeg, sizeof(mi_imagen_jpeg))) while(1);  // fallo del coprocesador

    // 7. Dibujar la imagen centrada
    Dibuja_imagen();

    while(1) {
        SLEEP;

        if(Cambia == 1) {
            Cambia = 0;
            // Aqui iria lo que cambie con el tiempo; si redibujas, llama a Dibuja_imagen()
        }
    }
}

void IntTimer(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    SysCtlDelay(20);
    Flag_ints++;
    cont_int++;
    if(cont_int >= 10){
        cont_int = 0;
        Cambia = 1;
    }
}
