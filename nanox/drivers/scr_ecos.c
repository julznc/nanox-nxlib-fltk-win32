/*
 * Copyright (c) 1999, 2000, 2010 Greg Haerr <greg@censoft.com>
 *
 * Microwindows Screen Driver for eCos
 *
 * Portions used from Ben Pfaff's BOGL <pfaffben@debian.org>
 *
 * Modified for eCos by
 *   Gary Thomas <gthomas@redhat.com>
 *   Richard Panton <richard.panton@3glab.org>
 * 
 * Note: modify select_fb_driver() to add new framebuffer subdrivers
 */
#define _GNU_SOURCE 1
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <cyg/hal/drv_api.h>
#include <cyg/io/io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"
#include <cyg/hal/lcd_support.h>
#include <cyg/infra/diag.h>

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);
static void fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette);

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	fb_open,
	fb_close,
	fb_setpalette,
	gen_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	gen_setportrait,
	NULL,				/* Update*/
	NULL				/* PreSelect*/
};

/* static variables*/
static int status;		/* 0=never inited, 1=once inited, 2=inited. */
//static short saved_red[256];	/* original hw palette*/
//static short saved_green[256];
//static short saved_blue[256];

/* local functions*/
static void	set_directcolor_palette(PSD psd);

/* init framebuffer*/
static PSD
fb_open(PSD psd)
{
    PSUBDRIVER subdriver;
    struct lcd_info li;

    assert(status < 2);

    // Initialize LCD screen
    lcd_init(16);
    lcd_getinfo(&li);

	psd->portrait = MWPORTRAIT_NONE;
    psd->xres = psd->xvirtres = li.width;
    psd->yres = psd->yvirtres = li.height;
    psd->planes = 1;
    psd->bpp = li.bpp;
    psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);
	psd->pitch = li.rlen;
	psd->size = psd->yres * psd->pitch;
    psd->flags = PSF_SCREEN;

    /* set pixel format*/
    switch (li.type) {
    case FB_TRUE_RGB565:
        psd->pixtype = MWPF_TRUECOLOR565;
        break;
    default:
        EPRINTF("Unsupported display type: %d\n", li.type);
        goto fail;
    }
#if 0
    if(1 /*visual == FB_VISUAL_TRUECOLOR || visual == FB_VISUAL_DIRECTCOLOR*/) {
        switch(psd->bpp) {
        case 8:
            psd->pixtype = MWPF_TRUECOLOR332;
            break;
        case 16:
            psd->pixtype = MWPF_TRUECOLOR565;
            break;
        case 24:
            psd->pixtype = MWPF_TRUECOLOR888;
            break;
        case 32:
            psd->pixtype = MWPF_TRUECOLOR8888;
            break;
        default:
            EPRINTF("Unsupported %d color (%d bpp) truecolor framebuffer\n", psd->ncolors, psd->bpp);
            goto fail;
        }
    } else psd->pixtype = MWPF_PALETTE;
#endif

    diag_printf("%dx%dx%d pitch %d type %d bpp %d\n", psd->xres,
      psd->yres, psd->ncolors, psd->pitch, li.type, psd->bpp);

	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

    /* select a framebuffer subdriver based on planes and bpp*/
    subdriver = select_fb_subdriver(psd);
    if (!subdriver) {
        EPRINTF("No driver for screen bpp %d\n", psd->bpp);
        goto fail;
    }

	/* set subdriver into screen driver */
	set_subdriver(psd, subdriver);

    /* mmap framebuffer into this address space*/
    psd->addr = li.fb;
    if(psd->addr == NULL || psd->addr == (unsigned char *)-1) {
//        EPRINTF("Error mmaping %s: %m\n", env);
        goto fail;
    }

#if 0    /* FIXME */
    /* save original palette*/
    ioctl_getpalette(0, 16, saved_red, saved_green, saved_blue);

    /* setup direct color palette if required (ATI cards)*/
    if(visual == FB_VISUAL_DIRECTCOLOR)
        set_directcolor_palette(psd);
#endif

    status = 2;
    return psd;	/* success*/

 fail:
    return NULL;
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);
#if 0
	int	tty;

	/* if not opened, return*/
	if(status != 2)
		return;
	status = 1;

  	/* reset hw palette*/
	ioctl_setpalette(0, 256, saved_red, saved_green, saved_blue);
  
	/* unmap framebuffer*/
	// munmap(psd->addr, psd->size);
  
	/* close framebuffer*/
	close(fb);
#endif
}

/* setup directcolor palette - required for ATI cards*/
static void
set_directcolor_palette(PSD psd)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);  while (1) ;
#if 0
	int i;
	short r[256], g[256], b[256];

	/* 16bpp uses 32 palette entries*/
	if(psd->bpp == 16) {
		/* FIXME: this still doesn't work*/
		for(i=0; i<32; ++i) {
			//r[i] = g[i] = b[i] = ((i<<11)|(i<<6)|i)<<8;
			//r[i] = g[i] = b[i] = ((i<<5)|i)<<10;
			//r[i] = g[i] = b[i] = i<<11;
			//r[i] = g[i] = b[i] = (i<<11) | (i<<3);
			r[i] = g[i] = b[i] = (i<<11);
			//r[i] = i << 8;
			//g[i] = i << 10;
			//b[i] = i << 8;
		}
		ioctl_setpalette(0, 32, r, g, b);
	} else {
		/* 32bpp uses 256 entries*/
		for(i=0; i<256; ++i)
			r[i] = i<<8;
		ioctl_setpalette(0, 256, r, r, r);
	}
#endif
}

static int fade = 100;

/* convert Microwindows palette to framebuffer format and set it*/
static void
fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);  while (1) ;
#if 0
	int 	i;
	unsigned short 	red[count];
	unsigned short 	green[count];
	unsigned short 	blue[count];

	/* convert palette to framebuffer format*/
	for(i=0; i < count; i++) {
		MWPALENTRY *p = &palette[i];

		/* grayscale computation:
		 * red[i] = green[i] = blue[i] =
		 *	(p->r * 77 + p->g * 151 + p->b * 28);
		 */
		red[i] = (p->r * fade / 100) << 8;
		green[i] = (p->g * fade / 100) << 8;
		blue[i] = (p->b * fade / 100) << 8;
	}
	ioctl_setpalette(first, count, red, green, blue);
#endif
}

/* get framebuffer palette*/
void
ioctl_getpalette(int start, int len, short *red, short *green, short *blue)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);  while (1) ;
#if 0
	struct fb_cmap cmap;
	cyg_uint32 sz = sizeof(cmap);

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

	cyg_io_get_config( fb_handle, CYG_IO_GET_CONFIG_FB_PALETTE, &cmap, &sz );
	// ioctl(fb, FBIOGETCMAP, &cmap);
#endif
}

/* set framebuffer palette*/
void
ioctl_setpalette(int start, int len, short *red, short *green, short *blue)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);  while (1) ;
#if 0
	struct fb_cmap cmap;
	cyg_uint32 sz = sizeof(cmap);

	cmap.start = start;
	cmap.len = len;
	cmap.red = red;
	cmap.green = green;
	cmap.blue = blue;
	cmap.transp = NULL;

	cyg_io_set_config( fb_handle, CYG_IO_SET_CONFIG_FB_PALETTE, &cmap, &sz );
	// ioctl(fb, FBIOPUTCMAP, &cmap);
#endif
}

/* experimental palette animation*/
void
setfadelevel(PSD psd, int f)
{
    diag_printf("%s - NOT IMPLEMENTED\n", __FUNCTION__);  while (1) ;
#if 0
	int 		i;
	unsigned short 	r[256], g[256], b[256];
	extern MWPALENTRY gr_palette[256];

	if(psd->pixtype != MWPF_PALETTE)
		return;

	fade = f;
	if(fade > 100)
		fade = 100;
	for(i=0; i<256; ++i) {
		r[i] = (gr_palette[i].r * fade / 100) << 8;
		g[i] = (gr_palette[i].g * fade / 100) << 8;
		b[i] = (gr_palette[i].b * fade / 100) << 8;
	}
	ioctl_setpalette(0, 256, r, g, b);
#endif
}
