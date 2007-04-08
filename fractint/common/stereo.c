/*
	STEREO.C a module to view 3D images.
	Written in Borland 'C++' by Paul de Leeuw.
	From an idea in "New Scientist" 9 October 1993 pages 26 - 29.

	Change History:
		11 June 94 - Modified to reuse existing Fractint arrays        TW
		11 July 94 - Added depth parameter                             PDL
		14 July 94 - Added grayscale option and did general cleanup    TW
		19 July 94 - Fixed negative depth                              PDL
		19 July 94 - Added calibration bars, get_min_max()             TW
		24 Sep  94 - Added image save/restore, color cycle, and save   TW
		28 Sep  94 - Added image map                                   TW
		20 Mar  95 - Fixed endless loop bug with bad depth values      TW
		23 Mar  95 - Allow arbitrary dimension image maps              TW

		(TW is Tim Wegner, PDL is Paul de Leeuw)
*/

#include <string.h>
#include <time.h>

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "drivers.h"
#include "helpdefs.h"

char stereomapname[FILE_MAX_DIR + 1] = {""};
int AutoStereo_depth = 100;
double AutoStereo_width = 10;
int g_grayscale_depth = 0; /* flag to use gray value rather than color number */
char calibrate = 1;             /* add calibration bars to image */
char image_map = 0;

/* this structure permits variables to be temporarily static and visible
	to routines in this file without permanently hogging memory */

static struct static_vars
{
	long avg;
	long avgct;
	long depth;
	int barheight;
	int ground;
	int maxcc;
	int maxc;
	int minc;
	int reverse;
	int sep;
	double width;
	int x1;
	int x2;
	int xcen;
	int y;
	int y1;
	int y2;
	int ycen;
	BYTE *savedac;
} *pv;

#define AVG         (pv->avg)
#define AVGCT       (pv->avgct)
#define DEPTH       (pv->depth)
#define BARHEIGHT   (pv->barheight)
#define GROUND      (pv->ground)
#define MAXCC       (pv->maxcc)
#define MAXC        (pv->maxc)
#define MINC        (pv->minc)
#define REVERSE     (pv->reverse)
#define SEP         (pv->sep)
#define WIDTH       (pv->width)
#define X1          (pv->x1)
#define X2          (pv->x2)
#define Y           (pv->y)
#define Y1          (pv->y1)
#define Y2          (pv->y2)
#define XCEN        (pv->xcen)
#define YCEN        (pv->ycen)

/*
	The getdepth() function allows using the grayscale value of the color
	as DEPTH, rather than the color number. Maybe I got a little too
	sophisticated trying to avoid a divide, so the comment tells what all
	the multiplies and shifts are trying to do. The result should be from
	0 to 255.
*/

typedef BYTE (*DACBOX)[256][3];
#define dac   (*((DACBOX)(pv->savedac)))

static int getdepth(int xd, int yd)
{
	int pal;
	pal = getcolor(xd, yd);
	if (g_grayscale_depth)
	{
		/* effectively (30*R + 59*G + 11*B)/100 scaled 0 to 255 */
		pal = ((int) dac[pal][0]*77 +
				(int) dac[pal][1]*151 +
				(int) dac[pal][2]*28);
		pal >>= 6;
	}
	return pal;
}

/*
	Get min and max DEPTH value in picture
*/

static int get_min_max(void)
{
	int xd, yd, ldepth;
	MINC = colors;
	MAXC = 0;
	for (yd = 0; yd < ydots; yd++)
	{
		if (driver_key_pressed())
		{
			return 1;
		}
		if (yd == 20)
		{
			showtempmsg("Getting min and max");
		}
		for (xd = 0; xd < xdots; xd++)
		{
			ldepth = getdepth(xd, yd);
			if (ldepth < MINC)
			{
				MINC = ldepth;
			}
			if (ldepth > MAXC)
			{
				MAXC = ldepth;
			}
		}
	}
	cleartempmsg();
	return 0;
}

void toggle_bars(int *bars, int barwidth, int *colour)
{
	int i, j, ct;
	find_special_colors();
	ct = 0;
	for (i = XCEN; i < (XCEN) + barwidth; i++)
	{
		for (j = YCEN; j < (YCEN) + BARHEIGHT; j++)
		{
			if (*bars)
			{
				g_put_color(i + (int)(AVG), j , g_color_bright);
				g_put_color(i - (int)(AVG), j , g_color_bright);
			}
			else
			{
				g_put_color(i + (int)(AVG), j, colour[ct++]);
				g_put_color(i - (int)(AVG), j, colour[ct++]);
			}
		}
	}
	*bars = 1 - *bars;
}

int outline_stereo(BYTE *pixels, int linelen)
{
	int i, j, x, s;
	int *same = _alloca(sizeof(int)*xdots);
	int *colour = _alloca(sizeof(int)*xdots);
	if ((Y) >= ydots)
	{
		return 1;
	}

	for (x = 0; x < xdots; ++x)
	{
		same[x] = x;
	}
	for (x = 0; x < xdots; ++x)
	{
		SEP = REVERSE
			? (GROUND - (int) (DEPTH*(getdepth(x, Y) - MINC) / MAXCC))
			: (GROUND - (int) (DEPTH*(MAXCC - (getdepth(x, Y) - MINC)) / MAXCC));
		SEP =  (int)((SEP*10.0) / WIDTH);        /* adjust for media WIDTH */

		/* get average value under calibration bars */
		if (X1 <= x && x <= X2 && Y1 <= Y && Y <= Y2)
		{
			AVG += SEP;
			(AVGCT)++;
		}
		i = x - (SEP + (SEP & Y & 1)) / 2;
		j = i + SEP;
		if (0 <= i && j < xdots)
		{
			/* there are cases where next never terminates so we timeout */
			int ct = 0;
			for (s = same[i]; s != i && s != j && ct++ < xdots; s = same[i])
			{
				if (s > j)
				{
					same[i] = j;
					i = j;
					j = s;
				}
				else
				{
					i = s;
				}
			}
			same[i] = j;
		}
	}
	for (x = xdots - 1; x >= 0; x--)
	{
		colour[x] = (same[x] == x) ? (int) pixels[x % linelen] : colour[same[x]];
		g_put_color(x, Y, colour[x]);
	}
	(Y)++;
	return 0;
}


/**************************************************************************
		Convert current image into Auto Stereo Picture
**************************************************************************/

int do_AutoStereo(void)
{
	struct static_vars v;
	BYTE savedacbox[256*3];
	int oldhelpmode, ret = 0;
	int i, j, done;
	int bars, ct, kbdchar, barwidth;
	time_t ltime;
	unsigned char *buf = (unsigned char *)g_decoder_line;
	/* following two lines re-use existing arrays in Fractint */
	int *same = _alloca(sizeof(int)*xdots);
	int *colour = _alloca(sizeof(int)*xdots);

	pv = &v;   /* set static vars to stack structure */
	pv->savedac = savedacbox;

	/* Use the current time to randomize the random number sequence. */
	time(&ltime);
	srand((unsigned int)ltime);

	oldhelpmode = helpmode;
	helpmode = RDSKEYS;
	driver_save_graphics();                      /* save graphics image */
	memcpy(savedacbox, g_dac_box, 256*3);  /* save colors */

	if (xdots > OLDMAXPIXELS)
	{
		stopmsg(0, "Stereo not allowed with resolution > 2048 pixels wide");
		driver_buzzer(BUZZER_INTERRUPT);
		ret = 1;
		goto exit_stereo;
	}

	/* empircally determined adjustment to make WIDTH scale correctly */
	WIDTH = AutoStereo_width*.67;
	if (WIDTH < 1)
	{
		WIDTH = 1;
	}
	GROUND = xdots / 8;
	REVERSE = (AutoStereo_depth < 0) ? 1 : 0;
	DEPTH = ((long) xdots*(long) AutoStereo_depth) / 4000L;
	DEPTH = labs(DEPTH) + 1;
	if (get_min_max())
	{
		driver_buzzer(BUZZER_INTERRUPT);
		ret = 1;
		goto exit_stereo;
	}
	MAXCC = MAXC - MINC + 1;
	AVG = AVGCT = 0L;
	barwidth  = 1 + xdots / 200;
	BARHEIGHT = 1 + ydots / 20;
	XCEN = xdots/2;
	YCEN = (calibrate > 1) ? BARHEIGHT/2 : ydots/2;

	/* box to average for calibration bars */
	X1 = XCEN - xdots/16;
	X2 = XCEN + xdots/16;
	Y1 = YCEN - BARHEIGHT/2;
	Y2 = YCEN + BARHEIGHT/2;

	Y = 0;
	if (image_map)
	{
		g_out_line = outline_stereo;
		while ((Y) < ydots)
		{
			if (gifview())
			{
				ret = 1;
				goto exit_stereo;
			}
		}
	}
	else
	{
		while (Y < ydots)
		{
			if (driver_key_pressed())
			{
				ret = 1;
				goto exit_stereo;
			}
			for (i = 0; i < xdots; i++)
			{
				buf[i] = (unsigned char)(rand() % colors);
			}
			outline_stereo(buf, xdots);
		}
	}

	find_special_colors();
	AVG /= AVGCT;
	AVG /= 2;
	ct = 0;
	for (i = XCEN; i < XCEN + barwidth; i++)
	{
		for (j = YCEN; j < YCEN + BARHEIGHT; j++)
		{
			colour[ct++] = getcolor(i + (int)(AVG), j);
			colour[ct++] = getcolor(i - (int)(AVG), j);
		}
	}
	bars = calibrate ? 1 : 0;
	toggle_bars(&bars, barwidth, colour);
	done = 0;
	while (done == 0)
	{
		driver_wait_key_pressed(0);
		kbdchar = driver_get_key();
		switch (kbdchar)
		{
		case FIK_ENTER:   /* toggle bars */
		case FIK_SPACE:
			toggle_bars(&bars, barwidth, colour);
			break;
		case 'c':
		case '+':
		case '-':
			rotate((kbdchar == 'c') ? 0 : ((kbdchar == '+') ? 1 : -1));
			break;
		case 's':
		case 'S':
			savetodisk(g_save_name);
			break;
		default:
			if (kbdchar == FIK_ESC)   /* if ESC avoid returning to menu */
			{
				kbdchar = 255;
			}
			driver_unget_key(kbdchar);
			driver_buzzer(BUZZER_COMPLETE);
			done = 1;
			break;
		}
	}

exit_stereo:
	helpmode = oldhelpmode;
	driver_restore_graphics();
	memcpy(g_dac_box, savedacbox, 256*3);
	spindac(0, 1);
	return ret;
}
