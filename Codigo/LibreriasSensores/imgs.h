#ifndef IMGS_H
#define IMGS_H

//Variables y tal
#define IMG_DIR     0          // direccion en RAM_G donde se guarda la imagen
#define CHUNK       512        // bytes por trozo (multiplo de 4)
#define OPT_RGB565  0

extern unsigned int ImgAncho, ImgAlto;

extern const unsigned char mi_imagen_jpeg[];
extern const unsigned int mi_imagen_jpeg_len;

extern int hasImgLoaded;
//Funciones para trabajar con las imagenes
void ComVertex2II(int x, int y, int handle, int cell);
int ComLoadImageJPEG(unsigned long ptr, unsigned long options, const unsigned char *data, unsigned long length);
int EsperaHueco(unsigned int n);
int EsperaCoprocesador(void);
int JPEG_Medidas(const unsigned char *d, unsigned long n, unsigned int *w, unsigned int *h);
void Dibuja_QR(void);



#endif
