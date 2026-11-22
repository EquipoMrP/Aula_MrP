#ifndef MrpEscenariosLib
#define MrpEscenariosLib
#include <stdint.h>

//Funcion para dibujar el fondo
void dibujarFondo(int id);

//Funcion para dibujar a MrP
void dibujarMrP(int x, int y, char idEmocion, char gafas, char cuerpo, char idAnimacion);
//Para poder gestionar el color de ropa de MrP
extern uint32_t colorRopaMrP;  // 1001 0110, 1001 0110, 1001 0110 = 150 150 150 -> Gris default
#define mascaraRojo 16711680    // 1111 1111, 0000 0000, 0000 0000 = 255   0   0
#define mascaraVerde 65280      // 0000 0000, 1111 1111, 0000 0000 =   0 255   0
#define mascaraAzul 255         // 0000 0000, 0000 0000, 1111 1111 =   0   0 255



#endif
