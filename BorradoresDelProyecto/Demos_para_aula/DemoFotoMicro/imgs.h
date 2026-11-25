#ifndef IMGS_H
#define IMGS_H

//Funciones para trabajar con las imagenes
void Dibuja_imagen(void);
void ComVertex2II(int x, int y, int handle, int cell);
int ComLoadImageJPEG(unsigned long ptr, unsigned long options, const unsigned char *data, unsigned long length);
int EsperaHueco(unsigned int n);
int EsperaCoprocesador(void);
int JPEG_Medidas(const unsigned char *d, unsigned long n, unsigned int *w, unsigned int *h);



#endif
