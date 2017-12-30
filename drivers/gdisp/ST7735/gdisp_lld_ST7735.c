/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gfx.h"

#if GFX_USE_GDISP

#if defined(GDISP_SCREEN_HEIGHT) || defined(GDISP_SCREEN_HEIGHT)
	#if GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_DIRECT
		#warning "GDISP: This low level driver does not support setting a screen size. It is being ignored."
	#elif GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_MACRO
		COMPILER_WARNING("GDISP: This low level driver does not support setting a screen size. It is being ignored.")
	#endif
	#undef GDISP_SCREEN_WIDTH
	#undef GDISP_SCREEN_HEIGHT
#endif

#define GDISP_DRIVER_VMT			GDISPVMT_ST7735
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

#include "board_ST7735.h"

#if (ST7735_TYPE_RED != TRUE) && (ST7735_TYPE_GREEN != TRUE) && (ST7735_TYPE_ST7735B != TRUE)
	#if GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_DIRECT
		#warning "GDISP: This low level driver requires at least one screen type to be defined (ST7735_TYPE_RED, ST7735_TYPE_GREEN, ST7735_TYPE_ST7735B)."
	#elif GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_MACRO
		COMPILER_WARNING("GDISP: This low level driver requires at least one screen type to be defined (ST7735_TYPE_RED, ST7735_TYPE_GREEN, ST7735_TYPE_ST7735B).")
	#endif
#endif

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
	#define GDISP_SCREEN_HEIGHT		160
#endif
#ifndef GDISP_SCREEN_WIDTH
	#define GDISP_SCREEN_WIDTH		128
#endif
#ifndef GDISP_INITIAL_CONTRAST
	#define GDISP_INITIAL_CONTRAST	50
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
	#define GDISP_INITIAL_BACKLIGHT	100
#endif

#include "ST7735.h"

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

// Some common routines and macros
#define dummy_read(g)				{ volatile uint16_t dummy; dummy = read_data(g); (void) dummy; }
#define write_reg(g, reg, data)		{ write_index(g, reg); write_data(g, data); }
#define write_data16(g, data)		{ write_data(g, data >> 8); write_data(g, (uint8_t)data); }
#define delay(us)					gfxSleepMicroseconds(us)
#define delayms(ms)					gfxSleepMilliseconds(ms)

static void set_viewport(GDisplay *g) {
	write_index(g, 0x2A);
	write_data(g, (g->p.x >> 8));
	write_data(g, (uint8_t) g->p.x);
	write_data(g, (g->p.x + g->p.cx - 1) >> 8);
	write_data(g, (uint8_t) (g->p.x + g->p.cx - 1));

	write_index(g, 0x2B);
	write_data(g, (g->p.y >> 8));
	write_data(g, (uint8_t) g->p.y);
	write_data(g, (g->p.y + g->p.cy - 1) >> 8);
	write_data(g, (uint8_t) (g->p.y + g->p.cy - 1));
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/
#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04
LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
	// No private area for this controller
	g->priv = 0;

	// Initialise the board interface
	init_board(g);

	// Hardware reset
	setpin_reset(g, TRUE);
	gfxSleepMilliseconds(20);
	setpin_reset(g, FALSE);
	gfxSleepMilliseconds(20);

	// Get the bus for the following initialisation commands
	acquire_bus(g);

	#if ST7735_TYPE_ST7735B == TRUE
	write_index(g, ST7735_SWRESET);	// software reset
	gfxSleepMilliseconds(50);
	write_index(g, ST7735_SLPOUT);	// out of sleep mode
	gfxSleepMilliseconds(500);
	write_index(g, ST7735_COLMOD);	// set colour mode
	write_data(g, 0x05);						// 16 bit colour
	gfxSleepMilliseconds(10);
	write_index(g, ST7735_FRMCTR1);	// Frame rate control
	write_data(g, 0x00);						// fastest refresh
	write_data(g, 0x06);						// 6 lines front porch
	write_data(g, 0x03);						// 3 lines back porch
	gfxSleepMilliseconds(10);
	write_index(g, ST7735_MADCTL);	// Memory access control
	write_data(g, 0x08);						// row addr/col addr, bottom to top refresh
	write_index(g, ST7735_DISSET5);	// Display settings
	write_data(g, 0x15);						// 1 cycle nonoverlap, 2 cycle gate rise, 3 cycle osc equalise
	write_data(g, 0x02);						// fix on vtl
	write_index(g, ST7735_INVCTR);	// Display inversion control
	write_data(g, 0x00);						// Line inversion
	write_index(g, ST7735_PWCTR1);	// Power control
	write_data(g, 0x02);						// GVDD = 4.7V
	write_data(g, 0x70);						// 1uA
	gfxSleepMilliseconds(10);
	write_index(g, ST7735_PWCTR2);	// power control
	write_data(g, 0x05);						// VGH = 14.7V, VGL = -7.35V
	write_index(g, ST7735_PWCTR3);	// power control
	write_data(g, 0x01);						// Opamp current small
	write_data(g, 0x02);						// boost frequency
	write_index(g, ST7735_VMCTR1);	// power control
	write_data(g, 0x3C);						// VCOMH = 4V
	write_data(g, 0x38);						// VCOML = -1.1V
	gfxSleepMilliseconds(10);
	write_index(g, ST7735_PWCTR6);	// power control
	write_data(g, 0x11);
	write_data(g, 0x15);
	write_index(g, ST7735_GMCTRP1); // magic unicorn dust
	write_data(g, 0x09);
	write_data(g, 0x16);
	write_data(g, 0x09);
	write_data(g, 0x20);
	write_data(g, 0x21);
	write_data(g, 0x1B);
	write_data(g, 0x13);
	write_data(g, 0x19);
	write_data(g, 0x17);
	write_data(g, 0x15);
	write_data(g, 0x1E);
	write_data(g, 0x2B);
	write_data(g, 0x04);
	write_data(g, 0x05);
	write_data(g, 0x02);
	write_data(g, 0x0E);
	write_index(g, ST7735_GMCTRN1); // sparkles and rainbows
	write_data(g, 0x0B);
	write_data(g, 0x14);
	write_data(g, 0x08);
	write_data(g, 0x1E);
	write_data(g, 0x22);
	write_data(g, 0x1D);
	write_data(g, 0x18);
	write_data(g, 0x1E);
	write_data(g, 0x1B);
	write_data(g, 0x1A);
	write_data(g, 0x24);
	write_data(g, 0x2B);
	write_data(g, 0x06);
	write_data(g, 0x06);
	write_data(g, 0x02);
	write_data(g, 0x0F);
	gfxSleepMicroseconds(10);
	write_index(g, ST7735_CASET); // column addr set
	write_data(g, 0x00);
	write_data(g, 0x02);					// XSTART=2
	write_data(g, 0x00);
	write_data(g, 0x81);					// XEND=129
	write_index(g, ST7735_RASET);	// row addr set
	write_data(g, 0x00);
	write_data(g, 0x02);					// XSTART=1
	write_data(g, 0x00);
	write_data(g, 0x81);					// XEND=160
	write_index(g, ST7735_NORON);	// Normal display on
	gfxSleepMilliseconds(10);
	write_index(g, ST7735_DISPON);	// main screen turn on
	gfxSleepMilliseconds(500);
	
	write_index(g, ST7735_MADCTL);
	write_data(g, MADCTL_MX | MADCTL_MY | MADCTL_RGB);
	#endif
	
	// borrowed from the adafruit library...
	#if ST7735_TYPE_RED == TRUE || ST7735_TYPE_GREEN == TRUE
 	write_index(g, ST7735_SWRESET);
	gfxSleepMilliseconds(150);
	write_index(g, ST7735_SLPOUT);
	gfxSleepMilliseconds(500);
	write_index(g, ST7735_FRMCTR1);
	write_data(g, 0x01);
	write_data(g, 0x2C);
	write_data(g, 0x2D);
	write_index(g, ST7735_FRMCTR2);
	write_data(g, 0x01);
	write_data(g, 0x2C);
	write_data(g, 0x2D);
	write_index(g, ST7735_FRMCTR3);
	write_data(g, 0x01);
	write_data(g, 0x2C);
	write_data(g, 0x2D);
	write_data(g, 0x01);
	write_data(g, 0x2C);
	write_data(g, 0x2D);
	write_index(g, ST7735_INVCTR);
	write_data(g, 0x07);
	write_index(g, ST7735_PWCTR1);
	write_data(g, 0xA2);
	write_data(g, 0x02);
	write_data(g, 0x84);
	write_index(g, ST7735_PWCTR2);
	write_data(g, 0xC5);
	write_index(g, ST7735_PWCTR3);
	write_data(g, 0x0A);
	write_data(g, 0x00);
	write_index(g, ST7735_PWCTR4);
	write_data(g, 0x8A);
	write_data(g, 0x2A);
	write_index(g, ST7735_PWCTR5);
	write_data(g, 0x8A);
	write_data(g, 0xEE);
	write_index(g, ST7735_VMCTR1);
	write_data(g, 0x0E);
	write_index(g, ST7735_INVOFF);
	write_index(g, ST7735_MADCTL);
	write_data(g, 0xC8);
	write_index(g, ST7735_COLMOD);
	write_data(g, 0x05);
	#if ST7735_TYPE_RED == TRUE
	write_index(g, ST7735_CASET);
	write_data(g, 0x00);
	write_data(g, 0x00);
	write_data(g, 0x00);
	write_data(g, 0x7F);
	write_index(g, ST7735_RASET);
	write_data(g, 0x00);
	write_data(g, 0x00);
	write_data(g, 0x00);
	write_data(g, 0x9F);
	#endif
	#if ST7735_TYPE_GREEN == TRUE
	write_index(g, ST7735_CASET);
	write_data(g, 0x00);
	write_data(g, 0x02);
	write_data(g, 0x00);
	write_data(g, 0x7F+0x02);
	write_index(g, ST7735_RASET);
	write_data(g, 0x00);
	write_data(g, 0x01);
	write_data(g, 0x00);
	write_data(g, 0x9F+0x01);
	#endif
	write_index(g, ST7735_GMCTRP1);
	write_data(g, 0x02);
	write_data(g, 0x01);
	write_data(g, 0x07);
	write_data(g, 0x12);
	write_data(g, 0x37);
	write_data(g, 0x32);
	write_data(g, 0x29);
	write_data(g, 0x2D);
	write_data(g, 0x29);
	write_data(g, 0x25);
	write_data(g, 0x2B);
	write_data(g, 0x39);
	write_data(g, 0x00);
	write_data(g, 0x01);
	write_data(g, 0x03);
	write_data(g, 0x10);
	write_index(g, ST7735_GMCTRN1);
	write_data(g, 0x03);
	write_data(g, 0x1D);
	write_data(g, 0x07);
	write_data(g, 0x06);
	write_data(g, 0x2E);
	write_data(g, 0x2C);
	write_data(g, 0x29);
	write_data(g, 0x2D);
	write_data(g, 0x2E);
	write_data(g, 0x37);
	write_data(g, 0x3F);
	write_data(g, 0x00);
	write_data(g, 0x00);
	write_data(g, 0x02);
	write_data(g, 0x10);
	write_index(g, ST7735_NORON);
	gfxSleepMilliseconds(10);
	write_index(g, ST7735_DISPON);
	gfxSleepMilliseconds(100);
	
	// set screen rotation?
	write_index(g, ST7735_MADCTL);
	write_data(g, MADCTL_MX | MADCTL_MY | MADCTL_RGB);
	#endif

	// Finish Init
	post_init_board(g);

 	// Release the bus
	release_bus(g);
	
	/* Turn on the back-light */
	set_backlight(g, GDISP_INITIAL_BACKLIGHT);

	/* Initialise the GDISP structure */
	g->g.Width = GDISP_SCREEN_WIDTH;
	g->g.Height = GDISP_SCREEN_HEIGHT;
	g->g.Orientation = GDISP_ROTATE_0;
	g->g.Powermode = powerOn;
	g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
	g->g.Contrast = GDISP_INITIAL_CONTRAST;
	return TRUE;
}

#if GDISP_HARDWARE_STREAM_WRITE
	LLDSPEC	void gdisp_lld_write_start(GDisplay *g) {
		acquire_bus(g);
		set_viewport(g);
		write_index(g, 0x2C);
	}
	LLDSPEC	void gdisp_lld_write_color(GDisplay *g) {
		write_data16(g, gdispColor2Native(g->p.color));
	}
	LLDSPEC	void gdisp_lld_write_stop(GDisplay *g) {
		release_bus(g);
	}
#endif

#if GDISP_HARDWARE_STREAM_READ
	LLDSPEC	void gdisp_lld_read_start(GDisplay *g) {
		acquire_bus(g);
		set_viewport(g);
		write_index(g, 0x2E);
		setreadmode(g);
		dummy_read(g);
	}
	LLDSPEC	color_t gdisp_lld_read_color(GDisplay *g) {
		uint16_t	data;

		data = read_data(g);
		return gdispNative2Color(data);
	}
	LLDSPEC	void gdisp_lld_read_stop(GDisplay *g) {
		setwritemode(g);
		release_bus(g);
	}
#endif

#if GDISP_NEED_CONTROL && GDISP_HARDWARE_CONTROL
	LLDSPEC void gdisp_lld_control(GDisplay *g) {
		switch(g->p.x) {
		case GDISP_CONTROL_POWER:
			if (g->g.Powermode == (powermode_t)g->p.ptr)
				return;
			switch((powermode_t)g->p.ptr) {
			case powerOff:
			case powerSleep:
			case powerDeepSleep:
				acquire_bus(g);
				write_reg(g, 0x0010, 0x0001);	/* enter sleep mode */
				release_bus(g);
				break;
			case powerOn:
				acquire_bus(g);
				write_reg(g, 0x0010, 0x0000);	/* leave sleep mode */
				release_bus(g);
				break;
			default:
				return;
			}
			g->g.Powermode = (powermode_t)g->p.ptr;
			return;

		case GDISP_CONTROL_ORIENTATION:
			if (g->g.Orientation == (orientation_t)g->p.ptr)
				return;
			switch((orientation_t)g->p.ptr) {
			case GDISP_ROTATE_0:
				acquire_bus(g);
				write_reg(g, 0x36, 0x48);	/* X and Y axes non-inverted */
				release_bus(g);
				g->g.Height = GDISP_SCREEN_HEIGHT;
				g->g.Width = GDISP_SCREEN_WIDTH;
				break;
			case GDISP_ROTATE_90:
				acquire_bus(g);
				write_reg(g, 0x36, 0xE8);	/* Invert X and Y axes */
				release_bus(g);
				g->g.Height = GDISP_SCREEN_WIDTH;
				g->g.Width = GDISP_SCREEN_HEIGHT;
				break;
			case GDISP_ROTATE_180:
				acquire_bus(g);
				write_reg(g, 0x36, 0x88);		/* X and Y axes non-inverted */
				release_bus(g);
				g->g.Height = GDISP_SCREEN_HEIGHT;
				g->g.Width = GDISP_SCREEN_WIDTH;
				break;
			case GDISP_ROTATE_270:
				acquire_bus(g);
				write_reg(g, 0x36, 0x28);	/* Invert X and Y axes */
				release_bus(g);
				g->g.Height = GDISP_SCREEN_WIDTH;
				g->g.Width = GDISP_SCREEN_HEIGHT;
				break;
			default:
				return;
			}
			g->g.Orientation = (orientation_t)g->p.ptr;
			return;

        case GDISP_CONTROL_BACKLIGHT:
            if ((unsigned)g->p.ptr > 100)
            	g->p.ptr = (void *)100;
            set_backlight(g, (unsigned)g->p.ptr);
            g->g.Backlight = (unsigned)g->p.ptr;
            return;

		//case GDISP_CONTROL_CONTRAST:
        default:
            return;
		}
	}
#endif

#endif /* GFX_USE_GDISP */
