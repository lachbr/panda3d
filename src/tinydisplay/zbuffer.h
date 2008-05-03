#ifndef _tgl_zbuffer_h_
#define _tgl_zbuffer_h_

/*
 * Z buffer
 */

#include "zfeatures.h"

#define ZB_Z_BITS 16

#define ZB_POINT_Z_FRAC_BITS 14

#define ZB_POINT_S_MIN ( (1<<13) )
#define ZB_POINT_S_MAX ( (1<<22)-(1<<13) )
#define ZB_POINT_T_MIN ( (1<<21) )
#define ZB_POINT_T_MAX ( (1<<30)-(1<<21) )

#define ZB_POINT_RED_MIN   0x0000
#define ZB_POINT_RED_MAX   0xff00
#define ZB_POINT_GREEN_MIN 0x0000
#define ZB_POINT_GREEN_MAX 0xff00
#define ZB_POINT_BLUE_MIN  0x0000
#define ZB_POINT_BLUE_MAX  0xff00
#define ZB_POINT_ALPHA_MIN 0x0000
#define ZB_POINT_ALPHA_MAX 0xff00

/* display modes */
#define ZB_MODE_5R6G5B  1  /* true color 16 bits */
#define ZB_MODE_INDEX   2  /* color index 8 bits */
#define ZB_MODE_RGBA    3  /* 32 bit rgba mode */
#define ZB_MODE_RGB24   4  /* 24 bit rgb mode */
#define ZB_NB_COLORS    225 /* number of colors for 8 bit display */

#define RGB_TO_PIXEL(r,g,b) \
  ((((unsigned int)(r) << 8) & 0xff0000) | ((unsigned int)(g) & 0xff00) | ((unsigned int)(b) >> 8))
#define RGBA_TO_PIXEL(r,g,b,a)                                   \
  ((((unsigned int)(a) << 16) & 0xff000000) | (((unsigned int)(r) << 8) & 0xff0000) | ((unsigned int)(g) & 0xff00) | ((unsigned int)(b) >> 8))
#define PIXEL_R(p) (((unsigned int)(p) & 0xff0000) >> 8)
#define PIXEL_G(p) ((unsigned int)(p) & 0xff00)
#define PIXEL_B(p) (((unsigned int)(p) & 0x00ff) << 8)
#define PIXEL_A(p) (((unsigned int)(p) & 0xff000000) >> 16)
typedef unsigned int PIXEL;
#define PSZB 4
#define PSZSH 5

#define PIXEL_MULT(p1, p2) \
  RGB_TO_PIXEL((PIXEL_R(p1) * PIXEL_R(p2)) >> 16, \
               (PIXEL_G(p1) * PIXEL_G(p2)) >> 16, \
               (PIXEL_B(p1) * PIXEL_B(p2)) >> 16)

#define PIXEL_BLEND(r1, g1, b1, r2, g2, b2, a2) \
  RGBA_TO_PIXEL(((r1) * (0xff00 - (a2)) + (r2) * (a2)) >> 16,   \
                ((g1) * (0xff00 - (a2)) + (g2) * (a2)) >> 16,   \
                ((b1) * (0xff00 - (a2)) + (b2) * (a2)) >> 16,   \
                a2)
#define PIXEL_BLEND_RGB(rgb, r, g, b, a) \
  PIXEL_BLEND(PIXEL_R(rgb), PIXEL_G(rgb), PIXEL_B(rgb), r, g, b, a)


typedef struct {
    int xsize,ysize;
    int linesize; /* line size, in bytes */
    int mode;
    
    unsigned short *zbuf;
    PIXEL *pbuf;
    int frame_buffer_allocated;
    
    int nb_colors;
    unsigned char *dctable;
    int *ctable;
    PIXEL *current_texture;
    unsigned int reference_alpha;
} ZBuffer;

typedef struct {
  int x,y,z;     /* integer coordinates in the zbuffer */
  int s,t;       /* coordinates for the mapping */
  int r,g,b,a;     /* color indexes */
  
  float sz,tz;   /* temporary coordinates for mapping */
} ZBufferPoint;

/* zbuffer.c */

ZBuffer *ZB_open(int xsize,int ysize,int mode,
		 int nb_colors,
		 unsigned char *color_indexes,
		 int *color_table,
		 void *frame_buffer);


void ZB_close(ZBuffer *zb);

void ZB_resize(ZBuffer *zb,void *frame_buffer,int xsize,int ysize);
void ZB_clear(ZBuffer *zb,int clear_z,int z,
	      int clear_color,int r,int g,int b);
/* linesize is in BYTES */
void ZB_copyFrameBuffer(ZBuffer *zb,void *buf,int linesize);

/* zdither.c */

void ZB_initDither(ZBuffer *zb,int nb_colors,
		   unsigned char *color_indexes,int *color_table);
void ZB_closeDither(ZBuffer *zb);
void ZB_ditherFrameBuffer(ZBuffer *zb,unsigned char *dest,
			  int linesize);

/* zline.c */

void ZB_plot(ZBuffer *zb,ZBufferPoint *p);
void ZB_line(ZBuffer *zb,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_line_z(ZBuffer * zb, ZBufferPoint * p1, ZBufferPoint * p2);

/* ztriangle.c */

void ZB_fillTriangleFlat_xx_zon_noblend_anone_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_noblend_anone_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_noblend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_noblend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_noblend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_noblend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_noblend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_noblend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_noblend_anone_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_noblend_anone_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_noblend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_noblend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_noblend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_noblend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_noblend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_noblend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_noblend_aless_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_noblend_aless_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_noblend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_noblend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_noblend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_noblend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_noblend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_noblend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_noblend_aless_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_noblend_aless_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_noblend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_noblend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_noblend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_noblend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_noblend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_noblend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_noblend_amore_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_noblend_amore_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_noblend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_noblend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_noblend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_noblend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_noblend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_noblend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_noblend_amore_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_noblend_amore_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_noblend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_noblend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_noblend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_noblend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_noblend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_noblend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_blend_anone_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_blend_anone_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_blend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_blend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_blend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_blend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_blend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_blend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_blend_anone_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_blend_anone_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_blend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_blend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_blend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_blend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_blend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_blend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_blend_aless_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_blend_aless_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_blend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_blend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_blend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_blend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_blend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_blend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_blend_aless_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_blend_aless_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_blend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_blend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_blend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_blend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_blend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_blend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_blend_amore_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_blend_amore_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_blend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_blend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_blend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_blend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_blend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_blend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_blend_amore_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_blend_amore_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_blend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_blend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_blend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_blend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_blend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_blend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_nocolor_anone_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_nocolor_anone_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_nocolor_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_nocolor_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_nocolor_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_nocolor_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_nocolor_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_nocolor_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_nocolor_anone_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_nocolor_anone_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_nocolor_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_nocolor_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_nocolor_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_nocolor_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_nocolor_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_nocolor_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_nocolor_aless_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_nocolor_aless_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_nocolor_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_nocolor_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_nocolor_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_nocolor_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_nocolor_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_nocolor_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_nocolor_aless_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_nocolor_aless_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_nocolor_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_nocolor_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_nocolor_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_nocolor_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_nocolor_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_nocolor_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_nocolor_amore_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_nocolor_amore_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_nocolor_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_nocolor_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_nocolor_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_nocolor_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_nocolor_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_nocolor_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zon_nocolor_amore_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zon_nocolor_amore_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zon_nocolor_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zon_nocolor_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zon_nocolor_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zon_nocolor_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zon_nocolor_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zon_nocolor_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_noblend_anone_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_noblend_anone_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_noblend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_noblend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_noblend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_noblend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_noblend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_noblend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_noblend_anone_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_noblend_anone_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_noblend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_noblend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_noblend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_noblend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_noblend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_noblend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_noblend_aless_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_noblend_aless_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_noblend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_noblend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_noblend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_noblend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_noblend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_noblend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_noblend_aless_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_noblend_aless_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_noblend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_noblend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_noblend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_noblend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_noblend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_noblend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_noblend_amore_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_noblend_amore_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_noblend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_noblend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_noblend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_noblend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_noblend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_noblend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_noblend_amore_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_noblend_amore_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_noblend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_noblend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_noblend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_noblend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_noblend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_noblend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_blend_anone_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_blend_anone_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_blend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_blend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_blend_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_blend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_blend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_blend_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_blend_anone_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_blend_anone_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_blend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_blend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_blend_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_blend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_blend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_blend_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_blend_aless_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_blend_aless_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_blend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_blend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_blend_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_blend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_blend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_blend_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_blend_aless_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_blend_aless_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_blend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_blend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_blend_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_blend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_blend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_blend_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_blend_amore_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_blend_amore_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_blend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_blend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_blend_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_blend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_blend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_blend_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_blend_amore_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_blend_amore_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_blend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_blend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_blend_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_blend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_blend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_blend_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_nocolor_anone_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_nocolor_anone_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_nocolor_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_nocolor_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_nocolor_anone_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_nocolor_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_nocolor_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_nocolor_anone_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_nocolor_anone_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_nocolor_anone_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_nocolor_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_nocolor_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_nocolor_anone_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_nocolor_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_nocolor_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_nocolor_anone_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_nocolor_aless_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_nocolor_aless_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_nocolor_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_nocolor_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_nocolor_aless_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_nocolor_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_nocolor_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_nocolor_aless_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_nocolor_aless_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_nocolor_aless_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_nocolor_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_nocolor_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_nocolor_aless_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_nocolor_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_nocolor_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_nocolor_aless_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_nocolor_amore_znone(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_nocolor_amore_znone(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_nocolor_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_nocolor_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_nocolor_amore_znone(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_nocolor_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_nocolor_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_nocolor_amore_znone(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

void ZB_fillTriangleFlat_xx_zoff_nocolor_amore_zless(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleSmooth_xx_zoff_nocolor_amore_zless(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMapping_xx_zoff_nocolor_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingFlat_xx_zoff_nocolor_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
void ZB_fillTriangleMappingSmooth_xx_zoff_nocolor_amore_zless(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleMappingPerspective_xx_zoff_nocolor_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveFlat_xx_zoff_nocolor_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_fillTriangleMappingPerspectiveSmooth_xx_zoff_nocolor_amore_zless(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);


typedef void (*ZB_fillTriangleFunc)(ZBuffer  *,
	    ZBufferPoint *,ZBufferPoint *,ZBufferPoint *);

/* memory.c */
void gl_free(void *p);
void *gl_malloc(int size);
void *gl_zalloc(int size);

#endif /* _tgl_zbuffer_h_ */
