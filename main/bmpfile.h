#ifndef __bmpfile_h__
#define __bmpfile_h__

typedef struct {
  uint8_t magic[2];   /* the magic number used to identify the BMP file:
                         0x42 0x4D (Hex code points for B and M).
                         The following entries are possible:
                         BM - Windows 3.1x, 95, NT, ... etc
                         BA - OS/2 Bitmap Array
                         CI - OS/2 Color Icon
                         CP - OS/2 Color Pointer
                         IC - OS/2 Icon
                         PT - OS/2 Pointer. */
  uint32_t filesz;    /* the size of the BMP file in bytes */
  uint16_t creator1;  /* reserved. */
  uint16_t creator2;  /* reserved. */
  uint32_t offset;    /* the offset, i.e. starting address,
                         of the byte where the bitmap data can be found. */
}__attribute__((packed)) bmp_header_t;

typedef struct {
  uint32_t header_sz;     /* the size of this header (40 bytes) */
  uint32_t width;         /* the bitmap width in pixels */
  uint32_t height;        /* the bitmap height in pixels */
  uint16_t nplanes;       /* the number of color planes being used.  Must be set to 1. */
  uint16_t depth;         /* the number of bits per pixel, which is the color depth of the image.  Typical values are 1, 4, 8, 16, 24 and 32. */
  uint32_t compress_type; /* the compression method being used.  See also bmp_compression_method_t. */
  uint32_t bmp_bytesz;    /* the image size. This is the size of the raw bitmap data (see below), and should not be confused with the file size. */
  uint32_t hres;          /* the horizontal resolution of the image.  (pixel per meter) */
  uint32_t vres;          /* the vertical resolution of the image.  (pixel per meter) */
  uint32_t ncolors;       /* the number of colors in the color palette, or 0 to default to 2<sup><i>n</i></sup>. */
  uint32_t nimpcolors;    /* the number of important colors used, or 0 when every color is important; generally ignored. */
}__attribute__((packed)) bmp_dib_v3_header_t;

/*
        <+++++++++ BMP Header +++++++++++> <==
0000000 4d42 1e82 0000 0000 0000 0082 0000 006c
        =======================================
0000010 0000 00f0 0000 00f0 0000 0001 0001 0000
        =======================================
0000020 0000 1e00 0000 0ec3 0000 0ec3 0000 0002
        =======================================
0000030 0000 0002 0000 0000 00ff ff00 0000 00ff
        =======================================
0000040 0000 0000 ff00 4742 7352 0000 0000 0000
        =======================================
0000050 0000 0000 4000 0000 0000 0000 0000 0000
        =======================================
0000060 4000 0000 0000 0000 0000 0000 4000 0000
        =======================> <----PALET---
0000070 0000 0000 0000 0000 0000 0000 0000 ffff
        ---> <---Pixel Data
0000080 00ff ffff ffff ffff ffff ffff ffff ffff
*/

typedef struct {
  uint32_t header_sz;     /* the size of this header (40 bytes) */
  uint32_t width;         /* the bitmap width in pixels */
  uint32_t height;        /* the bitmap height in pixels */
  uint16_t nplanes;       /* the number of color planes being used.  Must be set to 1. */
  uint16_t depth;         /* the number of bits per pixel, which is the color depth of the image.  Typical values are 1, 4, 8, 16, 24 and 32. */
  uint32_t compress_type; /* the compression method being used.  See also bmp_compression_method_t. */
  uint32_t bmp_bytesz;    /* the image size. This is the size of the raw bitmap data (see below), and should not be confused with the file size. */
  uint32_t hres;          /* the horizontal resolution of the image.  (pixel per meter) */
  uint32_t vres;          /* the vertical resolution of the image.  (pixel per meter) */
  uint32_t ncolors;       /* the number of colors in the color palette, or 0 to default to 2<sup><i>n</i></sup>. */
  uint32_t nimpcolors;    /* the number of important colors used, or 0 when every color is important; generally ignored. */
  uint32_t rmask;         /* 00 00 FF 00 */
  uint32_t gmask;         /* 00 FF 00 00 */
  uint32_t bmask;         /* FF 00 00 00 */
  uint32_t amask;         /* 00 00 00 FF */
  uint32_t cstype;        /* 42 47 52 73 */
  uint8_t space[36];      /* CIEXYZTRIPLE Color Space endpoints */
  uint32_t rgamma;        /* 00 00 00 00 */
  uint32_t ggamma;        /* 00 00 00 00 */
  uint32_t bgamma;        /* 00 00 00 00 */
}__attribute__((packed)) bmp_dib_v4_header_t;


typedef struct {
  bmp_header_t header;
  //bmp_dib_v3_header_t dib;
  bmp_dib_v4_header_t dib;
} bmpfile_t;

#endif /* __bmpfile_h__ */
