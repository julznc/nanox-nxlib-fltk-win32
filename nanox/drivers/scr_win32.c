/*
 * Copyright (c) 2000, 2001, 2003, 2005, 2010 Greg Haerr <greg@censoft.com>
 *
 * Microsoft Windows screen driver for Microwindows
 *	Tested in NONETWORK mode only
 *
 * by Wilson Loi
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <wingdi.h>
#include <assert.h>
#include "device.h"
#include "fb.h"
#include "genmem.h"
#include "genfont.h"

#define APP_NAME "Nano-X Window"

/* default screen size */
#define DEFAULT_SCREEN_WIDTH	240
#define DEFAULT_SCREEN_HEIGHT	320

#ifndef MWPIXEL_FORMAT
#define MWPIXEL_FORMAT	MWPF_TRUECOLOR8888
#endif

/* specific driver entry points*/
static PSD win32_open(PSD psd);
static void win32_close(PSD psd);
static void win32_getscreeninfo(PSD psd, PMWSCREENINFO psi);
static void win32_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height);

SCREENDEVICE scrdev = {
	0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
	gen_fonts,
	win32_open,
	win32_close,
	NULL,				/* SetPalette*/
	win32_getscreeninfo,
	gen_allocatememgc,
	gen_mapmemgc,
	gen_freememgc,
	NULL,				/* SetPortrait */
	win32_update,
	NULL				/* PreSelect*/
};

HWND winRootWindow = NULL;
static HDC dcBuffer = NULL;
static HBITMAP dcBitmap = NULL;
static HBITMAP dcOldBitmap;
static HANDLE dummyEvent = NULL;
static BITMAPV4HEADER bmpInfo;
static int scr_width;
static int scr_height;

LRESULT
myWindowProc(HWND hWnd,	UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HDC dc;
	switch (Msg) {
	case WM_CREATE:
		break;
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP: 
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
		break;
	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
		break;
	case WM_PAINT:
		dc = GetDC(winRootWindow);
		BitBlt(dc, 0, 0, scr_width, scr_height, dcBuffer, 0, 0, SRCCOPY);
		ReleaseDC(winRootWindow, dc);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		SelectObject(dcBuffer, dcOldBitmap);
		DeleteDC(dcBuffer);	
		DeleteObject(dcBitmap);
		winRootWindow = NULL;
		CloseHandle(dummyEvent);
		break;
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
	return 0;
}
 
/* called from keyboard/mouse/screen */
static PSD
win32_open(PSD psd)
{
	HANDLE hInstance = GetModuleHandle(NULL);
	RECT rect;
	PSUBDRIVER subdriver;
	WNDCLASSEX wincl;

	scr_width = getenv("NXSCREEN_WIDTH")? atoi(getenv("NXSCREEN_WIDTH")) : DEFAULT_SCREEN_WIDTH;
	scr_height = getenv("NXSCREEN_HEIGHT")? atoi(getenv("NXSCREEN_HEIGHT")) : DEFAULT_SCREEN_HEIGHT;
	
	if( 60 > scr_width || scr_width > 1920 )
		scr_width = DEFAULT_SCREEN_WIDTH;
		
	if( 80 > scr_height || scr_height > 1080 )
		scr_height = DEFAULT_SCREEN_HEIGHT;
	
	psd->xres = psd->xvirtres = scr_width;
	psd->yres = psd->yvirtres = scr_height;
	psd->planes = 1;
	psd->pixtype = MWPIXEL_FORMAT;
#if (MWPIXEL_FORMAT == MWPF_TRUECOLOR8888) || (MWPIXEL_FORMAT == MWPF_TRUECOLORABGR)
	psd->bpp = 32;
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR888)
	psd->bpp = 24;
#elif (MWPIXEL_FORMAT == MWPF_TRUECOLOR565) || (MWPIXEL_FORMAT == MWPF_TRUECOLOR555)
	psd->bpp = 16;
#else
#error "No support bpp < 16"
#endif 
	/* set standard data format from bpp and pixtype*/
	psd->data_format = set_data_format(psd);

	/* Calculate size and pitch*/
	GdCalcMemGCAlloc(psd, psd->xres, psd->yres, psd->planes, psd->bpp,
		&psd->size, &psd->pitch);
	if ((psd->addr = malloc(psd->size)) == NULL)
		return NULL;
	psd->ncolors = psd->bpp >= 24 ? (1 << 24) : (1 << psd->bpp);
	psd->flags = PSF_SCREEN | PSF_ADDRMALLOC;
	psd->portrait = MWPORTRAIT_NONE;
DPRINTF("win32 emulated bpp %d\n", psd->bpp);

	/* select an fb subdriver matching our planes and bpp for backing store*/
	subdriver = select_fb_subdriver(psd);
	if (!subdriver)
		return NULL;

	/* set subdriver into screen driver*/
	set_subdriver(psd, subdriver);

	/* The Window structure */
	wincl.hInstance = hInstance;
	wincl.lpszClassName = APP_NAME;
	wincl.lpfnWndProc = (WNDPROC)myWindowProc;
	wincl.style = CS_OWNDC;
	wincl.cbSize = sizeof (WNDCLASSEX);

	wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
	wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
	wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

	if (!RegisterClassEx (&wincl)) {
		free(psd->addr);
		return NULL;
	}
	
	winRootWindow = CreateWindowEx ( 0, APP_NAME, APP_NAME,
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
		scr_width, scr_height, HWND_DESKTOP, NULL, hInstance, NULL );
	
	if (winRootWindow) {
		HDC dc = GetDC(winRootWindow);

		GetClientRect(winRootWindow, &rect);
		dcBitmap = CreateCompatibleBitmap(dc, rect.right-rect.left,rect.bottom-rect.top);   
		dcBuffer = CreateCompatibleDC(dc);
		dcOldBitmap = SelectObject(dcBuffer, dcBitmap);
		ReleaseDC(winRootWindow, dc);
		dummyEvent = CreateEvent(NULL, TRUE, FALSE, "");
		ShowWindow(winRootWindow, SW_SHOWNORMAL);//SW_SHOWDEFAULT);
		UpdateWindow(winRootWindow);
	}
		
	memset(&bmpInfo, 0, sizeof(bmpInfo));
	bmpInfo.bV4Size = sizeof(bmpInfo);
	bmpInfo.bV4Width = psd->xres;
	bmpInfo.bV4Height = -psd->yres; /* top-down image */
	bmpInfo.bV4Planes = 1;
	bmpInfo.bV4BitCount = psd->bpp;
	bmpInfo.bV4SizeImage = psd->size;
        
	bmpInfo.bV4AlphaMask = 0xff000000;
	bmpInfo.bV4RedMask  = 0xff0000;
	bmpInfo.bV4GreenMask= 0x00ff00;
	bmpInfo.bV4BlueMask = 0x0000ff;
	bmpInfo.bV4V4Compression = BI_BITFIELDS;

	bmpInfo.bV4XPelsPerMeter = 3078;
	bmpInfo.bV4YPelsPerMeter = 3078;
	bmpInfo.bV4ClrUsed = 0;
	bmpInfo.bV4ClrImportant = 0;   
	bmpInfo.bV4CSType = LCS_CALIBRATED_RGB;
	
	return &scrdev;
}

static void
win32_close(PSD psd)
{
	if (winRootWindow)
		SendMessage(winRootWindow, WM_DESTROY, 0, 0);
	if(psd->addr)
		free(psd->addr);
}


static void
win32_getscreeninfo(PSD psd, PMWSCREENINFO psi)
{
	gen_getscreeninfo(psd, psi);
	psi->fbdriver = FALSE;	/* not running fb driver, no direct map */
}

static void
win32_update(PSD psd, MWCOORD x, MWCOORD y, MWCOORD width, MWCOORD height)
{
#if 1// full area update
	SetDIBitsToDevice(dcBuffer, 0, 0, psd->xres, psd->yres, 0, 0, 0, psd->yres,
			psd->addr, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS);
#else // fix me
	unsigned char *addr = psd->addr + ((y * psd->xres * psd->bpp) >> 3);
	
	SetDIBitsToDevice(dcBuffer, x, y, psd->xres, psd->yres, x, y, y, psd->yres,
			addr, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS);
#endif
}
