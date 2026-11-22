//////////////////////////////////////////////////////////////////////////////////////
////       //////     //////      ///      ///  ////  /////    ////       ///////////////////////////////////////////////////
////  //////////  ////////  ///////// ////////   ///  ///  ///  //   ///////////////////////////////////
////       //////   /////  //////////     ////  / //  ///       ////     ///////////////////////////////////////////////
////  /////////////  /////  ///////// ////////  // /  ///  ///  ///////  //////////////////////////////
////       /////    ///////      ////       //  ///   ///  ///  ///     ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

/* ---------------------------------------------------------------------------------------------------\\
|| En esta libreria se incluyen todas las funciones necesarias para cualquier dibujo "general"        ||
|| al programa de las clases:                                                                         ||
|| 1. Dibujar clases                                                                                  ||
|| 2. Dibujar a Mr P                                                                                  ||
\\----------------------------------------------------------------------------------------------------*/


/// 0. Librerias
#include <MrpEscenariosLib.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "FT800_TIVA.h"


/// 1. Dibujar escenario
void dibujarFondo(int id){

    switch(id){
    case 0: //LECCION TEORIA
        //Fondo
        ComColor(245, 228, 156); //Caqui
        ComLineWidth(5);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);

        //Borde pizarra
        ComColor(156, 90, 60); //Marron
        ComRect(17, 17, HSIZE-17, VSIZE-17, true);

        //Relleno pizarra
        ComColor(13, 77, 53); //Verde oscuro
        ComRect(23, 23, HSIZE-23, VSIZE-23, true);

        ComLineWidth(1);
        //Borrador
        ComColor(70, 70, 70); //Gris oscuro
        ComRect(HSIZE-43, VSIZE-23, HSIZE-63, VSIZE-26, true);
        ComColor(79, 41, 25); //Marron oscuro
        ComRect(HSIZE-43, VSIZE-28, HSIZE-63, VSIZE-33, true);

        //Tizas
        ComColor(255,255,255);
        ComRect(HSIZE-79, VSIZE-24, HSIZE-89, VSIZE-25, true);
        ComRect(HSIZE-104, VSIZE-24, HSIZE-108, VSIZE-25, true);

        //Retoques
        ComColor(13, 77, 53); //Verde oscuro
        ComRect(HSIZE-76, VSIZE-28, HSIZE-111, VSIZE-30, true);

        break;

    case 1://LECCION EJEMPLO
        //Fondo
        ComColor(245, 228, 156); //Caqui
        ComLineWidth(5);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);

        //Borde pizarra
        ComColor(0, 0, 0); //Negro
        ComRect(17, 17, HSIZE-17, 33, true);
        ComRect(23, 23, HSIZE-23, VSIZE-23, true);


        //Borde abajo pizarra
        ComRect(20, VSIZE-26, HSIZE-20, VSIZE-18, true);

        //Relleno pizarra
        ComColor(255, 255, 255); //Blanco
        ComRect(25, 23, HSIZE-25, VSIZE-23, true);

        break;

    default:
        ComColor(245, 228, 156); //Caqui
        ComLineWidth(5);
        ComRect(10, 10, HSIZE-10, VSIZE-10, true);
        break;

    }
}

//2. Dibujar a MrP
uint32_t colorRopaMrP=9868950;

void dibujarMrP(int x, int y, char idEmocion, char gafas, char cuerpo, char idAnimacion){

    //Variables para el control de la posicion y todo lo demas
    static int xC=0;        //Eje de revolucion de la cabeza, x
    static int yC=0;        //Eje de revolucion de la cabeza, y

    static int temp=0;    //Temporizado

    static int xML=0;       //Posicion mano izquierda
    static int yML=0;
    static int xMR=0;       // ''       ''  derecha
    static int yMR=0;
    static float ang=0;     //Control del angulo de las manos

    static char estadoAnim=0;    //Para gestionar las animaciones
    static char idAnim_p=0;      //Para poder transicionar correctamente entre animaciones

    if(idAnim_p!=idAnimacion){
        estadoAnim=0;
        idAnim_p=idAnimacion;
    }

    temp++;
    if(temp>200){ //Saturacion del temporizador, de forma que no fastidie los movimientos circulares
        temp=0;
    }

    //Animacion (se hace ahora porque toca el resto de variables)
    switch (idAnimacion){

    default: //Default es el case 0, sin Animacion
        xC=x;
        yC=y+135;
        xML=x-80; yML=y+225;
        xMR=x+80; yMR=y+225;

        break;

    case 1: //Idle normal, cabeza sube y baja, pero poco
        switch(estadoAnim){
        case 0:
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;

        case 1: // CORREGIDO: el cambio de estado solo ocurre al llegar al limite
            if (yC>y+132){
                if(temp%5==0) yC--;
            } else {  estadoAnim=2;   }
            break;

        case 2:
            if (yC<y+139){
                if(temp%5==0) yC++;
            } else { estadoAnim=1; }
            break;

        }
        break;

    case 2: //Emocionado

        switch (estadoAnim){
        case 0: //Inicio, llegar al "reposo"
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;
        case 1:
            if(yML>y+100){
                yML-=5;
                yMR-=5;
            } else {
                estadoAnim=2;
            }
            break;

        case 2:
            if(yML>y+100){
                yML--;
                yMR--;
            } else {
                estadoAnim=3;
            }
            break;

        case 3:
            if(yML<y+120){
                yML++;
                yMR++;
            } else {
                estadoAnim=2;
            }
            break;
        }
        break;

    case 3: //Hablando

        switch (estadoAnim){
        case 0: //Inicio, llegar al "reposo"
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;
        case 1:
            if(yML>y+150){
                yML-=5;
                yMR-=5;
            } else {
                estadoAnim=2;
            }
            break;

        case 2:
            if(yML>y+150){
                yML--;
                yMR--;
            } else {
                estadoAnim=3;
            }
            break;

        case 3:
            if(yML<y+170){
                yML++;
                yMR++;
            } else {
                estadoAnim=2;
            }
            break;
        }
        break;

    case 4: //Risa
        switch(estadoAnim){
        case 0:
            xC=x;
            yC=y+135;
            xML=x-15; yML=yC-15;
            xMR=x+15; yMR=yC-15;
            estadoAnim=1;
            break;

        case 1: // CORREGIDO: igual que en la animacion 1
            if (yC>y+132){
                if(temp%3==0){
                    yC--;
                    yML=yC-15;
                    yMR=yML;
                }
            } else {  estadoAnim=2;   }
            break;

        case 2:
            if (yC<y+139){
                if(temp%3==0){
                    yC++;
                    yML=yC-15;
                    yMR=yML;
                }
            } else { estadoAnim=1; }
            break;

        }
        break;

    case 5: //Palmada

        switch (estadoAnim){
        case 0: //Inicio, llegar al "reposo"
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;
        case 1:
            if(yML>190){
                yML-=5;
                yMR-=5;
            } else {
                estadoAnim=2;
            }
            break;
        case 2:
            if(xML<x-10){
                xML+=3;
            }

            if(xMR>x+10){
                xMR-=3;
            } else {
                estadoAnim=3;
            }
            break;

        case 3:
            if(xMR<x+50){
                xMR+=3;
            }

            if(xML>x-50){
                xML-=3;
            } else {
                estadoAnim=2;
            }
            break;

        }
        break;

    case 6: //Dormir

        switch (estadoAnim){
        case 0: //Inicio, llegar al "reposo"
            xC=x;
            yC=y+135;
            xML=x-60; yML=y+205;
            xMR=x+60; yMR=y+205;
            estadoAnim=1;
            break;

        case 1: //Moverse hacia abajo y las manos a la barbilla
            if(yC<y+155){    yC++;  }
            if(xML<x-12){ xML+=4;  }
            if(xMR>x+12){  xMR-=4;  }
            if(yML>y+160){  yML-=4;  }
            yMR=yML;

            break;
        }

        break;

    case 7: //Pensando
        switch(estadoAnim){
        case 0:
            xC=x;
            yC=y+135;
            xML=x-30; yML=yC-20;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;

        case 1: // CORREGIDO: igual que en la animacion 1
            if (xC>x-5){
                if(temp%3==0){
                    xC--;
                    xML=xC-30;
                }
            } else {  estadoAnim=2;   }
            break;

        case 2:
            if (xC<x+5){
                if(temp%3==0){
                    xC++;
                    xML=xC-30;
                }
            } else { estadoAnim=1; }
            break;

        }
        break;

    case 8: //Saludando
        switch(estadoAnim){
        case 0:
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;

        case 1:
            if (ang>0){
                ang-=0.04;
                xML=xC-100.*cos(ang);
                yML=yC-100.*sin(ang);
            } else {  estadoAnim=2;   }
            break;

        case 2:
            if (ang<M_PI/2){
                ang+=0.04;
                xML=xC-100.*cos(ang);
                yML=yC-100.*sin(ang);
            } else { estadoAnim=1; }
            break;

        }
        break;

    case 9: //Movimiento circulero
        switch(estadoAnim){
        case 0:
            xC=x;
            yC=y+135;
            xML=x-80; yML=y+225;
            xMR=x+80; yMR=y+225;
            estadoAnim=1;
            break;
        case 1:
            xC=x+10.*cos((float)temp/10./M_PI);
            yC=y+135+10.*sin((float)temp/10./M_PI);
            break;
        }
        break;
    }

    //Si el cuerpo es visible, lo dibujamos
    if(cuerpo){
        //Dibujamos la "camisa"
        ComColor((colorRopaMrP & mascaraRojo)>>16,(colorRopaMrP & mascaraVerde)>>8,(colorRopaMrP & mascaraAzul)); //Color cami
        int i=0;
        for(i=-2;i<3;i++){
            ComLine(x,y+175,x+(i*15),y+300,30);
        }
    }

    //Dibujamos el pelo pt 1
    ComColor(61, 43, 26); //Color marron
    ComLine(xC-22,yC-105,xC+22,yC-105,30);

    //Dibujamos la piel de la cara
    ComColor(255,181,144);//Color piel
    ComCirculo(xC,yC-75,43);
    ComCirculo(xC,yC-50,43);
    ComLine(xC-33, yC-75,xC-33, yC-50,10);
    ComLine(xC+33, yC-75,xC+33, yC-50,10);
    ComLine(xC-28, yC-63,xC+28, yC-63,15);

    //Dibujamos el pelo pt 2
    ComColor(61, 43, 26); //Color marron
    ComLine(xC-25,yC-106,xC+25,yC-106,12);
    ComColor(107, 91, 57); //Color marron claro
    ComLine(xC-5,yC-115,xC+5,yC-115,7);

    //Dibujamos la nariz
    ComColor(0,0,0); //Negro
    ComCirculo(xC,yC-38,13);
    ComColor(255,181,144);//Color piel
    ComLine(xC-13,yC-36,xC+13,yC-36,12);

    switch (idEmocion){
    case 0: //Contento
        //Ojos
        if(gafas){
            ComColor(0,0,0); //Negro
            ComLine(xC-27,yC-67,xC-27,yC-62,4);
            ComLine(xC+27,yC-67,xC+27,yC-62,4);
        } else { //Si no tiene gafas, ojos en forma de 3
            ComColor(0,0,0); //Negro
            ComCirculo(xC+22,yC-68,6); // Up R
            ComCirculo(xC+22,yC-61,6); // Down R
            ComCirculo(xC-22,yC-68,6); // Up L
            ComCirculo(xC-22,yC-61,6); // Down L
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+22,yC-68,3); // Up R
            ComCirculo(xC+22,yC-61,3); // Down R
            ComCirculo(xC-22,yC-68,3); // Up L
            ComCirculo(xC-22,yC-61,3); // Down L
            ComLine(xC-26,yC-66,xC-26,yC-64,4); //Recorte L
            ComLine(xC+18,yC-66,xC+18,yC-64,4); //Recorte R
        }

        // Boca
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-38,13);
            ComColor(255,181,144);//Color piel
            ComLine(xC-13,yC-40,xC+13,yC-40,8);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
        break;

    case 1: //Shock
        //Ojos (En shock no dependen de si gafas o no) (Aun asi, si no hay gafas se dibujan mas cerca entre si)
        if(gafas){
            ComColor(0,0,0); //Negro - Borde
            ComCirculo(xC-27,yC-65,13);
            ComCirculo(xC+27,yC-65,13);
            ComColor(255,255,255); //Blanco - Blanco
            ComCirculo(xC+27,yC-65,12);
            ComCirculo(xC-27,yC-65,12);
            ComColor(0,0,0); //Negro - Pupila
            ComCirculo(xC-27,yC-65,5);
            ComCirculo(xC+27,yC-65,5);
        } else {
            ComColor(0,0,0); //Negro - Borde
            ComCirculo(xC-24,yC-65,13);
            ComCirculo(xC+24,yC-65,13);
            ComColor(255,255,255); //Blanco - Blanco
            ComCirculo(xC+24,yC-65,12);
            ComCirculo(xC-24,yC-65,12);
            ComColor(0,0,0); //Negro - Pupila
            ComCirculo(xC-24,yC-65,5);
            ComCirculo(xC+24,yC-65,5);

        }
        //Boca
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,7);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
        break;

    case 2: //Dormido
        //Ojos (En dormido no dependen de si gafas o no) (Aun asi, sin gafas se dibujan mas cerca entre si)
        if(gafas){
            ComColor(0,0,0); //Negro
            ComCirculo(xC+27,yC-74,12);
            ComCirculo(xC-27,yC-74,12);
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+27,yC-74,10);
            ComCirculo(xC-27,yC-74,10);
        } else {
            ComColor(0,0,0); //Negro
            ComCirculo(xC+24,yC-74,12);
            ComCirculo(xC-24,yC-74,12);
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+24,yC-74,10);
            ComCirculo(xC-24,yC-74,10);
        }
        ComLine(xC-32,yC-80,xC+32,yC-80,11);

        //Boca
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-38,13);
            ComColor(255,181,144);//Color piel
            ComLine(xC-13,yC-40,xC+13,yC-40,8);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
        break;

    case 3: //Triste
        //Ojos
        if (gafas){
            ComColor(0,0,0); //Negro
            ComLine(xC-27,yC-67,xC-27,yC-62,4);
            ComLine(xC+27,yC-67,xC+27,yC-62,4);
            ComColor(255,181,144);//Color piel;
            ComLine(xC-32,yC-70,xC-23,yC-74,4);
            ComLine(xC+32,yC-70,xC+23,yC-74,4);
        } else { //Si no tiene gafas, ojos en forma de 3
            ComColor(0,0,0); //Negro
            ComLine(xC-27,yC-63,xC-18,yC-67,2); //Ceja L
            ComLine(xC+27,yC-63,xC+18,yC-67,2); //Ceja R
        }

        //Boca
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-20,12);
            ComColor(255,181,144);//Color piel
            ComLine(xC-7,yC-17,xC+7,yC-17,9);
            ComLine(xC,yC-16,xC,yC-16,9);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
        break;

    case 4: //Enfadado
        //Ojos
        if(gafas){
            ComColor(0,0,0); //Negro
            ComLine(xC-27,yC-67,xC-27,yC-62,4);
            ComLine(xC+27,yC-67,xC+27,yC-62,4);
            ComLine(xC-23,yC-70,xC-32,yC-74,2);
            ComLine(xC+23,yC-70,xC+32,yC-74,2);
        } else { //Si no tiene gafas, ojos en forma de 3
            ComColor(0,0,0); //Negro
            ComCirculo(xC+22,yC-68,6); // Up R
            ComCirculo(xC+22,yC-61,6); // Down R
            ComCirculo(xC-22,yC-68,6); // Up L
            ComCirculo(xC-22,yC-61,6); // Down L
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+22,yC-68,3); // Up R
            ComCirculo(xC+22,yC-61,3); // Down R
            ComCirculo(xC-22,yC-68,3); // Up L
            ComCirculo(xC-22,yC-61,3); // Down L
            ComLine(xC-26,yC-66,xC-26,yC-64,4); //Recorte L
            ComLine(xC+18,yC-66,xC+18,yC-64,4); //Recorte R
            ComColor(0,0,0);
            ComLine(xC-17,yC-74,xC-28,yC-76,2); //Ceja L
            ComLine(xC+17,yC-74,xC+28,yC-76,2); //Ceja R
        }

        //Boca
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-20,12);
            ComColor(255,181,144);//Color piel
            ComLine(xC-7,yC-17,xC+7,yC-17,9);
            ComLine(xC,yC-16,xC,yC-16,9);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
        break;


    case 5: //Desafiante
        //Ojos
        if(gafas){
            ComColor(0,0,0); //Negro
            ComLine(xC-27,yC-67,xC-27,yC-62,4);
            ComLine(xC+27,yC-67,xC+27,yC-62,4);
            ComLine(xC-23,yC-70,xC-32,yC-74,2);
            ComLine(xC+23,yC-70,xC+32,yC-74,2);
        } else { //Si no tiene gafas, ojos en forma de 3
            ComColor(0,0,0); //Negro
            ComCirculo(xC+22,yC-68,6); // Up R
            ComCirculo(xC+22,yC-61,6); // Down R
            ComCirculo(xC-22,yC-68,6); // Up L
            ComCirculo(xC-22,yC-61,6); // Down L
            ComColor(255,181,144);//Color piel
            ComCirculo(xC+22,yC-68,3); // Up R
            ComCirculo(xC+22,yC-61,3); // Down R
            ComCirculo(xC-22,yC-68,3); // Up L
            ComCirculo(xC-22,yC-61,3); // Down L
            ComLine(xC-26,yC-66,xC-26,yC-64,4); //Recorte L
            ComLine(xC+18,yC-66,xC+18,yC-64,4); //Recorte R
            ComColor(0,0,0);
            ComLine(xC-17,yC-74,xC-28,yC-76,2); //Ceja L
            ComLine(xC+17,yC-74,xC+28,yC-76,2); //Ceja R
        }

        //Boca
        if(idAnimacion!=3 || temp%20>10){
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-38,13);
            ComColor(255,181,144);//Color piel
            ComLine(xC-13,yC-40,xC+13,yC-40,8);
        } else { //Boca hablando
            ComColor(0,0,0); //Negro
            ComCirculo(xC,yC-32,4);
        }
        break;

    default:
        idEmocion=0;
        break;
    }


    //Gafas
    if(gafas){
        ComColor(0,0,80); //Azul oscuro
            //Izquierda
        ComLine(xC-47,yC-78,xC-8 ,yC-78,2); //Arriba
        ComLine(xC-47,yC-52,xC-8 ,yC-52,2); //Abajo
        ComLine(xC-8 ,yC-78,xC-8 ,yC-52,2); //Derecha
        ComLine(xC-47,yC-78,xC-47,yC-52,2); //Izquierda
            //Derecha
        ComLine(xC+47,yC-78,xC+8 ,yC-78,2); //Arriba
        ComLine(xC+47,yC-52,xC+8 ,yC-52,2); //Abajo
        ComLine(xC+8 ,yC-78,xC+8 ,yC-52,2); //Derecha
        ComLine(xC+47,yC-78,xC+47,yC-52,2); //Izquierda
            //Centro
        ComLine(xC-7 ,yC-65,xC+7 ,yC-65,3);
    }

    if(cuerpo){
        // Dibujamos manos (color de la camisa mas oscuro, sin pasar por debajo de 0)
        ComColor(((((colorRopaMrP & mascaraRojo)>>16) > 10) ? (((colorRopaMrP & mascaraRojo)>>16) - 10) : 0)    ,
                 ((((colorRopaMrP & mascaraVerde)>>8) > 10) ? (((colorRopaMrP & mascaraVerde)>>8) - 10) : 0)    ,
                 (( (colorRopaMrP & mascaraAzul)      > 10) ? (( colorRopaMrP & mascaraAzul) - 10)      : 0)    );
        ComCirculo(xML, yML, 20); //Izquierda
        ComCirculo(xMR, yMR, 20); //Derecha
    }
}

