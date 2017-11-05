/*

Miscellaneous fractal-specific code (formerly in CALCFRAC.C)

*/
#include <algorithm>
#include <vector>

#include <limits.h>
#include <stdlib.h>
#include <string.h>

// see Fractint.c for a description of the "include"  hierarchy
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "targa_lc.h"
#include "drivers.h"

// routines in this module

static void set_Plasma_palette();
static U16 adjust(int xa, int ya, int x, int y, int xb, int yb);
static void subDivide(int x1, int y1, int x2, int y2);
static void verhulst();
static void Bif_Period_Init();
static bool Bif_Periodic(long time);
static void set_Cellular_palette();

U16(*getpix)(int, int)  = (U16(*)(int, int))getcolor;

typedef void (*PLOT)(int, int, int);

//**************** standalone engine for "test" *******************

int test()
{
    int startrow, startpass, numpasses;
    startpass = 0;
    startrow = startpass;
    if (resuming)
    {
        start_resume();
        get_resume(sizeof(startrow), &startrow, sizeof(startpass), &startpass, 0);
        end_resume();
    }
    if (teststart())   // assume it was stand-alone, doesn't want passes logic
    {
        return (0);
    }
    numpasses = (stdcalcmode == '1') ? 0 : 1;
    for (passes = startpass; passes <= numpasses ; passes++)
    {
        for (row = startrow; row <= iystop; row = row+1+numpasses)
        {
            for (col = 0; col <= ixstop; col++)       // look at each point on screen
            {
                int color;
                g_init.x = dxpixel();
                g_init.y = dypixel();
                if (driver_key_pressed())
                {
                    testend();
                    alloc_resume(20, 1);
                    put_resume(sizeof(row), &row, sizeof(passes), &passes, 0);
                    return (-1);
                }
                color = testpt(g_init.x, g_init.y, parm.x, parm.y, maxit, g_inside);
                if (color >= g_colors)
                {
                    // avoid trouble if color is 0
                    if (g_colors < 16)
                    {
                        color &= g_and_color;
                    }
                    else
                    {
                        color = ((color-1) % g_and_color) + 1; // skip color zero
                    }
                }
                (*plot)(col, row, color);
                if (numpasses && (passes == 0))
                {
                    (*plot)(col, row+1, color);
                }
            }
        }
        startrow = passes + 1;
    }
    testend();
    return (0);
}

//**************** standalone engine for "plasma" *******************

static int iparmx;      // iparmx = parm.x * 8
static int shiftvalue;  // shift based on #colors
static int recur1 = 1;
static int pcolors;
static int recur_level = 0;
U16 max_plasma;

// returns a random 16 bit value that is never 0
U16 rand16()
{
    U16 value;
    value = (U16)rand15();
    value <<= 1;
    value = (U16)(value + (rand15()&1));
    if (value < 1)
    {
        value = 1;
    }
    return (value);
}

void putpot(int x, int y, U16 color)
{
    if (color < 1)
    {
        color = 1;
    }
    putcolor(x, y, (color >> 8) ? (color >> 8) : 1);  // don't write 0
    /* we don't write this if driver_diskp() because the above putcolor
          was already a "writedisk" in that case */
    if (!driver_diskp())
    {
        writedisk(x+sxoffs, y+syoffs, color >> 8);    // upper 8 bits
    }
    writedisk(x+sxoffs, y+sydots+syoffs, color&255); // lower 8 bits
}

// fixes border
void putpotborder(int x, int y, U16 color)
{
    if ((x == 0) || (y == 0) || (x == xdots-1) || (y == ydots-1))
    {
        color = (U16)outside;
    }
    putpot(x, y, color);
}

// fixes border
void putcolorborder(int x, int y, int color)
{
    if ((x == 0) || (y == 0) || (x == xdots-1) || (y == ydots-1))
    {
        color = outside;
    }
    if (color < 1)
    {
        color = 1;
    }
    putcolor(x, y, color);
}

U16 getpot(int x, int y)
{
    U16 color;

    color = (U16)readdisk(x+sxoffs, y+syoffs);
    color = (U16)((color << 8) + (U16) readdisk(x+sxoffs, y+sydots+syoffs));
    return (color);
}

static int plasma_check;                        // to limit kbd checking

static U16 adjust(int xa, int ya, int x, int y, int xb, int yb)
{
    S32 pseudorandom;
    pseudorandom = ((S32)iparmx)*((rand15()-16383));
    pseudorandom = pseudorandom * recur1;
    pseudorandom = pseudorandom >> shiftvalue;
    pseudorandom = (((S32)getpix(xa, ya)+(S32)getpix(xb, yb)+1) >> 1)+pseudorandom;
    if (max_plasma == 0)
    {
        if (pseudorandom >= pcolors)
        {
            pseudorandom = pcolors-1;
        }
    }
    else if (pseudorandom >= (S32)max_plasma)
    {
        pseudorandom = max_plasma;
    }
    if (pseudorandom < 1)
    {
        pseudorandom = 1;
    }
    plot(x, y, (U16)pseudorandom);
    return ((U16)pseudorandom);
}


static bool new_subD(int x1, int y1, int x2, int y2, int recur)
{
    int x, y;
    int nx1;
    int nx;
    int ny1, ny;
    S32 i, v;

    struct sub
    {
        BYTE t; // top of stack
        int v[16]; // subdivided value
        BYTE r[16];  // recursion level
    };

    static sub subx, suby;

    recur1 = (int)(320L >> recur);
    suby.t = 2;
    suby.v[0] = y2;
    ny   = suby.v[0];
    suby.v[2] = y1;
    ny1 = suby.v[2];
    suby.r[2] = 0;
    suby.r[0] = suby.r[2];
    suby.r[1] = 1;
    suby.v[1] = (ny1 + ny) >> 1;
    y = suby.v[1];

    while (suby.t >= 1)
    {
        if ((++plasma_check & 0x0f) == 1)
        {
            if (driver_key_pressed())
            {
                plasma_check--;
                return true;
            }
        }
        while (suby.r[suby.t-1] < (BYTE)recur)
        {
            //     1.  Create new entry at top of the stack
            //     2.  Copy old top value to new top value.
            //            This is largest y value.
            //     3.  Smallest y is now old mid point
            //     4.  Set new mid point recursion level
            //     5.  New mid point value is average
            //            of largest and smallest

            suby.t++;
            suby.v[suby.t] = suby.v[suby.t-1];
            ny1  = suby.v[suby.t];
            ny   = suby.v[suby.t-2];
            suby.r[suby.t] = suby.r[suby.t-1];
            suby.v[suby.t-1]   = (ny1 + ny) >> 1;
            y    = suby.v[suby.t-1];
            suby.r[suby.t-1]   = (BYTE)(std::max(suby.r[suby.t], suby.r[suby.t-2])+1);
        }
        subx.t = 2;
        subx.v[0] = x2;
        nx  = subx.v[0];
        subx.v[2] = x1;
        nx1 = subx.v[2];
        subx.r[2] = 0;
        subx.r[0] = subx.r[2];
        subx.r[1] = 1;
        subx.v[1] = (nx1 + nx) >> 1;
        x = subx.v[1];

        while (subx.t >= 1)
        {
            while (subx.r[subx.t-1] < (BYTE)recur)
            {
                subx.t++; // move the top ofthe stack up 1
                subx.v[subx.t] = subx.v[subx.t-1];
                nx1  = subx.v[subx.t];
                nx   = subx.v[subx.t-2];
                subx.r[subx.t] = subx.r[subx.t-1];
                subx.v[subx.t-1]   = (nx1 + nx) >> 1;
                x    = subx.v[subx.t-1];
                subx.r[subx.t-1]   = (BYTE)(std::max(subx.r[subx.t], subx.r[subx.t-2])+1);
            }

            i = getpix(nx, y);
            if (i == 0)
            {
                i = adjust(nx, ny1, nx, y , nx, ny);
            }
            // cppcheck-suppress AssignmentIntegerToAddress
            v = i;
            i = getpix(x, ny);
            if (i == 0)
            {
                i = adjust(nx1, ny, x , ny, nx, ny);
            }
            v += i;
            if (getpix(x, y) == 0)
            {
                i = getpix(x, ny1);
                if (i == 0)
                {
                    i = adjust(nx1, ny1, x , ny1, nx, ny1);
                }
                v += i;
                i = getpix(nx1, y);
                if (i == 0)
                {
                    i = adjust(nx1, ny1, nx1, y , nx1, ny);
                }
                v += i;
                plot(x, y, (U16)((v + 2) >> 2));
            }

            if (subx.r[subx.t-1] == (BYTE)recur)
            {
                subx.t = (BYTE)(subx.t - 2);
            }
        }

        if (suby.r[suby.t-1] == (BYTE)recur)
        {
            suby.t = (BYTE)(suby.t - 2);
        }
    }
    return false;
}

static void subDivide(int x1, int y1, int x2, int y2)
{
    int x, y;
    S32 v, i;
    if ((++plasma_check & 0x7f) == 1)
    {
        if (driver_key_pressed())
        {
            plasma_check--;
            return;
        }
    }
    if (x2-x1 < 2 && y2-y1 < 2)
    {
        return;
    }
    recur_level++;
    recur1 = (int)(320L >> recur_level);

    x = (x1+x2) >> 1;
    y = (y1+y2) >> 1;
    v = getpix(x, y1);
    if (v == 0)
    {
        v = adjust(x1, y1, x , y1, x2, y1);
    }
    i = v;
    v = getpix(x2, y);
    if (v == 0)
    {
        v = adjust(x2, y1, x2, y , x2, y2);
    }
    i += v;
    v = getpix(x, y2);
    if (v == 0)
    {
        v = adjust(x1, y2, x , y2, x2, y2);
    }
    i += v;
    v = getpix(x1, y);
    if (v == 0)
    {
        v = adjust(x1, y1, x1, y , x1, y2);
    }
    i += v;

    if (getpix(x, y) == 0)
    {
        plot(x, y, (U16)((i+2) >> 2));
    }

    subDivide(x1, y1, x , y);
    subDivide(x , y1, x2, y);
    subDivide(x , y , x2, y2);
    subDivide(x1, y , x , y2);
    recur_level--;
}


int plasma()
{
    U16 rnd[4];
    bool OldPotFlag = false;
    bool OldPot16bit = false;
    plasma_check = 0;

    if (g_colors < 4)
    {
        stopmsg(STOPMSG_NONE,
                "Plasma Clouds can currently only be run in a 4-or-more-color video\n"
                "mode (and color-cycled only on VGA adapters [or EGA adapters in their\n"
                "640x350x16 mode]).");
        return (-1);
    }
    iparmx = (int)(param[0] * 8);
    if (parm.x <= 0.0)
    {
        iparmx = 0;
    }
    if (parm.x >= 100)
    {
        iparmx = 800;
    }
    param[0] = (double)iparmx / 8.0;  // let user know what was used
    if (param[1] < 0)
    {
        param[1] = 0;  // limit parameter values
    }
    if (param[1] > 1)
    {
        param[1] = 1;
    }
    if (param[2] < 0)
    {
        param[2] = 0;  // limit parameter values
    }
    if (param[2] > 1)
    {
        param[2] = 1;
    }
    if (param[3] < 0)
    {
        param[3] = 0;  // limit parameter values
    }
    if (param[3] > 1)
    {
        param[3] = 1;
    }

    if (!rflag && param[2] == 1)
    {
        --rseed;
    }
    if (param[2] != 0 && param[2] != 1)
    {
        rseed = (int)param[2];
    }
    max_plasma = (U16)param[3];  // max_plasma is used as a flag for potential

    if (max_plasma != 0)
    {
        if (pot_startdisk() >= 0)
        {
            max_plasma = 0xFFFF;
            if (outside >= COLOR_BLACK)
            {
                plot    = (PLOT)putpotborder;
            }
            else
            {
                plot    = (PLOT)putpot;
            }
            getpix =  getpot;
            OldPotFlag = potflag;
            OldPot16bit = pot16bit;
        }
        else
        {
            max_plasma = 0;        // can't do potential (startdisk failed)
            param[3]   = 0;
            if (outside >= COLOR_BLACK)
            {
                plot    = putcolorborder;
            }
            else
            {
                plot    = putcolor;
            }
            getpix  = (U16(*)(int, int))getcolor;
        }
    }
    else
    {
        if (outside >= COLOR_BLACK)
        {
            plot    = putcolorborder;
        }
        else
        {
            plot    = putcolor;
        }
        getpix  = (U16(*)(int, int))getcolor;
    }
    srand(rseed);
    if (!rflag)
    {
        ++rseed;
    }

    if (g_colors == 256)                     // set the (256-color) palette
    {
        set_Plasma_palette();             // skip this if < 256 colors
    }

    if (g_colors > 16)
    {
        shiftvalue = 18;
    }
    else
    {
        if (g_colors > 4)
        {
            shiftvalue = 22;
        }
        else
        {
            if (g_colors > 2)
            {
                shiftvalue = 24;
            }
            else
            {
                shiftvalue = 25;
            }
        }
    }
    if (max_plasma != 0)
    {
        shiftvalue = 10;
    }

    if (max_plasma == 0)
    {
        pcolors = std::min(g_colors, max_colors);
        for (auto &elem : rnd)
        {
            elem = (U16)(1+(((rand15()/pcolors)*(pcolors-1)) >> (shiftvalue-11)));
        }
    }
    else
    {
        for (auto &elem : rnd)
        {
            elem = rand16();
        }
    }
    if (g_debug_flag == debug_flags::prevent_plasma_random)
    {
        for (auto &elem : rnd)
        {
            elem = 1;
        }
    }

    plot(0,      0,  rnd[0]);
    plot(xdots-1,      0,  rnd[1]);
    plot(xdots-1, ydots-1,  rnd[2]);
    plot(0, ydots-1,  rnd[3]);

    int n;
    recur_level = 0;
    if (param[1] == 0)
    {
        subDivide(0, 0, xdots-1, ydots-1);
    }
    else
    {
        int i = 1;
        int k = 1;
        recur1 = 1;
        while (new_subD(0, 0, xdots-1, ydots-1, i) == 0)
        {
            k = k * 2;
            if (k  >(int)std::max(xdots-1, ydots-1))
            {
                break;
            }
            if (driver_key_pressed())
            {
                n = 1;
                goto done;
            }
            i++;
        }
    }
    if (!driver_key_pressed())
    {
        n = 0;
    }
    else
    {
        n = 1;
    }
done:
    if (max_plasma != 0)
    {
        potflag = OldPotFlag;
        pot16bit = OldPot16bit;
    }
    plot    = putcolor;
    getpix  = (U16(*)(int, int))getcolor;
    return (n);
}

static void set_Plasma_palette()
{
    static BYTE const Red[3]   = { 63, 0, 0 };
    static BYTE const Green[3] = { 0, 63, 0 };
    static BYTE const Blue[3]  = { 0,  0, 63 };

    if (map_specified || g_colors_preloaded)
    {
        return;    // map= specified
    }

    g_dac_box[0][0] = 0;
    g_dac_box[0][1] = 0;
    g_dac_box[0][2] = 0;
    for (int i = 1; i <= 85; i++)
    {
        g_dac_box[i][0] = (BYTE)((i*Green[0] + (86-i)*Blue[0])/85);
        g_dac_box[i][1] = (BYTE)((i*Green[1] + (86-i)*Blue[1])/85);
        g_dac_box[i][2] = (BYTE)((i*Green[2] + (86-i)*Blue[2])/85);

        g_dac_box[i+85][0] = (BYTE)((i*Red[0] + (86-i)*Green[0])/85);
        g_dac_box[i+85][1] = (BYTE)((i*Red[1] + (86-i)*Green[1])/85);
        g_dac_box[i+85][2] = (BYTE)((i*Red[2] + (86-i)*Green[2])/85);
        g_dac_box[i+170][0] = (BYTE)((i*Blue[0] + (86-i)*Red[0])/85);
        g_dac_box[i+170][1] = (BYTE)((i*Blue[1] + (86-i)*Red[1])/85);
        g_dac_box[i+170][2] = (BYTE)((i*Blue[2] + (86-i)*Red[2])/85);
    }
    spindac(0, 1);
}

//**************** standalone engine for "diffusion" *******************

#define RANDOM(x)  (rand()%(x))

int diffusion()
{
    int xmax, ymax, xmin, ymin;     // Current maximum coordinates
    int border;   // Distance between release point and fractal
    int mode;     // Determines diffusion type:  0 = central (classic)
    //                             1 = falling particles
    //                             2 = square cavity
    int colorshift; // If zero, select colors at random, otherwise shift the color every colorshift points
    int colorcount, currentcolor;
    double cosine, sine, angle;
    int x, y;
    float r, radius;

    if (driver_diskp())
    {
        notdiskmsg();
    }

    y = -1;
    x = y;
    bitshift = 16;
    g_fudge_factor = 1L << 16;

    border = (int)param[0];
    mode = (int)param[1];
    colorshift = (int)param[2];

    colorcount = colorshift; // Counts down from colorshift
    currentcolor = 1;  // Start at color 1 (color 0 is probably invisible)

    if (mode > 2)
    {
        mode = 0;
    }

    if (border <= 0)
    {
        border = 10;
    }

    srand(rseed);
    if (!rflag)
    {
        ++rseed;
    }

    if (mode == 0)
    {
        xmax = xdots / 2 + border;  // Initial box
        xmin = xdots / 2 - border;
        ymax = ydots / 2 + border;
        ymin = ydots / 2 - border;
    }
    if (mode == 1)
    {
        xmax = xdots / 2 + border;  // Initial box
        xmin = xdots / 2 - border;
        ymin = ydots - border;
    }
    if (mode == 2)
    {
        if (xdots > ydots)
        {
            radius = (float)(ydots - border);
        }
        else
        {
            radius = (float)(xdots - border);
        }
    }
    if (resuming) // restore worklist, if we can't the above will stay in place
    {
        start_resume();
        if (mode != 2)
        {
            get_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin, sizeof(ymax), &ymax,
                       sizeof(ymin), &ymin, 0);
        }
        else
        {
            get_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin, sizeof(ymax), &ymax,
                       sizeof(radius), &radius, 0);
        }
        end_resume();
    }

    switch (mode)
    {
    case 0: // Single seed point in the center
        putcolor(xdots / 2, ydots / 2, currentcolor);
        break;
    case 1: // Line along the bottom
        for (int i = 0; i <= xdots; i++)
        {
            putcolor(i, ydots-1, currentcolor);
        }
        break;
    case 2: // Large square that fills the screen
        if (xdots > ydots)
        {
            for (int i = 0; i < ydots; i++)
            {
                putcolor(xdots/2-ydots/2 , i , currentcolor);
                putcolor(xdots/2+ydots/2 , i , currentcolor);
                putcolor(xdots/2-ydots/2+i , 0 , currentcolor);
                putcolor(xdots/2-ydots/2+i , ydots-1 , currentcolor);
            }
        }
        else
        {
            for (int i = 0; i < xdots; i++)
            {
                putcolor(0 , ydots/2-xdots/2+i , currentcolor);
                putcolor(xdots-1 , ydots/2-xdots/2+i , currentcolor);
                putcolor(i , ydots/2-xdots/2 , currentcolor);
                putcolor(i , ydots/2+xdots/2 , currentcolor);
            }
        }
        break;
    }

    while (1)
    {
        switch (mode)
        {
        case 0: // Release new point on a circle inside the box
            angle = 2*(double)rand()/(RAND_MAX/PI);
            FPUsincos(&angle, &sine, &cosine);
            x = (int)(cosine*(xmax-xmin) + xdots);
            y = (int)(sine  *(ymax-ymin) + ydots);
            x = x >> 1; // divide by 2
            y = y >> 1;
            break;
        case 1: /* Release new point on the line ymin somewhere between xmin
                 and xmax */
            y = ymin;
            x = RANDOM(xmax-xmin) + (xdots-xmax+xmin)/2;
            break;
        case 2: /* Release new point on a circle inside the box with radius
                 given by the radius variable */
            angle = 2*(double)rand()/(RAND_MAX/PI);
            FPUsincos(&angle, &sine, &cosine);
            x = (int)(cosine*radius + xdots);
            y = (int)(sine  *radius + ydots);
            x = x >> 1;
            y = y >> 1;
            break;
        }

        // Loop as long as the point (x,y) is surrounded by color 0
        // on all eight sides

        while ((getcolor(x+1, y+1) == 0) && (getcolor(x+1, y) == 0) &&
                (getcolor(x+1, y-1) == 0) && (getcolor(x  , y+1) == 0) &&
                (getcolor(x  , y-1) == 0) && (getcolor(x-1, y+1) == 0) &&
                (getcolor(x-1, y) == 0) && (getcolor(x-1, y-1) == 0))
        {
            // Erase moving point
            if (show_orbit)
            {
                putcolor(x, y, 0);
            }

            if (mode == 0)
            {
                // Make sure point is inside the box
                if (x == xmax)
                {
                    x--;
                }
                else if (x == xmin)
                {
                    x++;
                }
                if (y == ymax)
                {
                    y--;
                }
                else if (y == ymin)
                {
                    y++;
                }
            }

            if (mode == 1) /* Make sure point is on the screen below ymin, but
                    we need a 1 pixel margin because of the next random step.*/
            {
                if (x >= xdots-1)
                {
                    x--;
                }
                else if (x <= 1)
                {
                    x++;
                }
                if (y < ymin)
                {
                    y++;
                }
            }

            // Take one random step
            x += RANDOM(3) - 1;
            y += RANDOM(3) - 1;

            // Check keyboard
            if ((++plasma_check & 0x7f) == 1)
            {
                if (check_key())
                {
                    alloc_resume(20, 1);
                    if (mode != 2)
                    {
                        put_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin,
                                   sizeof(ymax), &ymax, sizeof(ymin), &ymin, 0);
                    }
                    else
                    {
                        put_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin,
                                   sizeof(ymax), &ymax, sizeof(radius), &radius, 0);
                    }

                    plasma_check--;
                    return 1;
                }
            }

            // Show the moving point
            if (show_orbit)
            {
                putcolor(x, y, RANDOM(g_colors-1)+1);
            }

        } // End of loop, now fix the point

        /* If we're doing colorshifting then use currentcolor, otherwise
           pick one at random */
        putcolor(x, y, colorshift?currentcolor:RANDOM(g_colors-1)+1);

        // If we're doing colorshifting then check to see if we need to shift
        if (colorshift)
        {
            if (!--colorcount)
            {
                // If the counter reaches zero then shift
                currentcolor++;      // Increase the current color and wrap
                currentcolor %= g_colors;  // around skipping zero
                if (!currentcolor)
                {
                    currentcolor++;
                }
                colorcount = colorshift;  // and reset the counter
            }
        }

        /* If the new point is close to an edge, we may need to increase
           some limits so that the limits expand to match the growing
           fractal. */

        switch (mode)
        {
        case 0:
            if (((x+border) > xmax) || ((x-border) < xmin)
                    || ((y-border) < ymin) || ((y+border) > ymax))
            {
                // Increase box size, but not past the edge of the screen
                ymin--;
                ymax++;
                xmin--;
                xmax++;
                if ((ymin == 0) || (xmin == 0))
                {
                    return 0;
                }
            }
            break;
        case 1: // Decrease ymin, but not past top of screen
            if (y-border < ymin)
            {
                ymin--;
            }
            if (ymin == 0)
            {
                return 0;
            }
            break;
        case 2: /* Decrease the radius where points are released to stay away
                 from the fractal.  It might be decreased by 1 or 2 */
            r = sqr((float)x-xdots/2) + sqr((float)y-ydots/2);
            if (r <= border*border)
            {
                return 0;
            }
            while ((radius-border)*(radius-border) > r)
            {
                radius--;
            }
            break;
        }
    }
}



//*********** standalone engine for "bifurcation" types **************

//*************************************************************
// The following code now forms a generalised Fractal Engine
// for Bifurcation fractal typeS.  By rights it now belongs in
// CALCFRACT.C, but it's easier for me to leave it here !

// Besides generalisation, enhancements include Periodicity
// Checking during the plotting phase (AND halfway through the
// filter cycle, if possible, to halve calc times), quicker
// floating-point calculations for the standard Verhulst type,
// and new bifurcation types (integer bifurcation, f.p & int
// biflambda - the real equivalent of complex Lambda sets -
// and f.p renditions of bifurcations of r*sin(Pi*p), which
// spurred Mitchel Feigenbaum on to discover his Number).

// To add further types, extend the fractalspecific[] array in
// usual way, with Bifurcation as the engine, and the name of
// the routine that calculates the next bifurcation generation
// as the "orbitcalc" routine in the fractalspecific[] entry.

// Bifurcation "orbitcalc" routines get called once per screen
// pixel column.  They should calculate the next generation
// from the doubles Rate & Population (or the longs lRate &
// lPopulation if they use integer math), placing the result
// back in Population (or lPopulation).  They should return 0
// if all is ok, or any non-zero value if calculation bailout
// is desirable (e.g. in case of errors, or the series tending
// to infinity).                Have fun !
//*************************************************************

#define DEFAULTFILTER 1000     /* "Beauty of Fractals" recommends using 5000
                               (p.25), but that seems unnecessary. Can
                               override this value with a nonzero param1 */

#define SEED 0.66               // starting value for population

static std::vector<int> verhulst_array;
unsigned long filter_cycles;
static bool half_time_check = false;
static long   lPopulation, lRate;
double Population,  Rate;
static bool mono = false;
static int outside_x = 0;
static long   LPI;

int Bifurcation()
{
    int x = 0;
    if (resuming)
    {
        start_resume();
        get_resume(sizeof(x), &x, 0);
        end_resume();
    }
    bool resized = false;
    try
    {
        verhulst_array.resize(iystop + 1);
        resized = true;
    }
    catch (std::bad_alloc const&)
    {
    }
    if (!resized)
    {
        stopmsg(STOPMSG_NONE, "Insufficient free memory for calculation.");
        return (-1);
    }

    LPI = (long)(PI * g_fudge_factor);

    for (int y = 0; y <= iystop; y++)   // should be iystop
    {
        verhulst_array[y] = 0;
    }

    mono = false;
    if (g_colors == 2)
    {
        mono = true;
    }
    if (mono)
    {
        if (g_inside != COLOR_BLACK)
        {
            outside_x = 0;
            g_inside = 1;
        }
        else
        {
            outside_x = 1;
        }
    }

    filter_cycles = (parm.x <= 0) ? DEFAULTFILTER : (long)parm.x;
    half_time_check = false;
    if (periodicitycheck && (unsigned long)maxit < filter_cycles)
    {
        filter_cycles = (filter_cycles - maxit + 1) / 2;
        half_time_check = true;
    }

    if (g_integer_fractal)
    {
        linit.y = ymax - iystop*dely;            // Y-value of
    }
    else
    {
        g_init.y = (double)(yymax - iystop*delyy); // bottom pixels
    }

    while (x <= ixstop)
    {
        if (driver_key_pressed())
        {
            verhulst_array.clear();
            alloc_resume(10, 1);
            put_resume(sizeof(x), &x, 0);
            return (-1);
        }

        if (g_integer_fractal)
        {
            lRate = xmin + x*delx;
        }
        else
        {
            Rate = (double)(xxmin + x*delxx);
        }
        verhulst();        // calculate array once per column

        for (int y = iystop; y >= 0; y--) // should be iystop & >=0
        {
            int color;
            color = verhulst_array[y];
            if (color && mono)
            {
                color = g_inside;
            }
            else if ((!color) && mono)
            {
                color = outside_x;
            }
            else if (color>=g_colors)
            {
                color = g_colors-1;
            }
            verhulst_array[y] = 0;
            (*plot)(x, y, color); // was row-1, but that's not right?
        }
        x++;
    }
    verhulst_array.clear();
    return (0);
}

static void verhulst()          // P. F. Verhulst (1845)
{
    unsigned int pixel_row;

    if (g_integer_fractal)
    {
        lPopulation = (parm.y == 0) ? (long)(SEED*g_fudge_factor) : (long)(parm.y*g_fudge_factor);
    }
    else
    {
        Population = (parm.y == 0) ? SEED : parm.y;
    }

    overflow = false;

    for (unsigned long counter = 0UL; counter < filter_cycles ; counter++)
    {
        if (curfractalspecific->orbitcalc())
        {
            return;
        }
    }
    if (half_time_check) // check for periodicity at half-time
    {
        Bif_Period_Init();
        unsigned long counter;
        for (counter = 0; counter < (unsigned long)maxit ; counter++)
        {
            if (curfractalspecific->orbitcalc())
            {
                return;
            }
            if (periodicitycheck && Bif_Periodic(counter))
            {
                break;
            }
        }
        if (counter >= (unsigned long)maxit)   // if not periodic, go the distance
        {
            for (counter = 0; counter < filter_cycles ; counter++)
            {
                if (curfractalspecific->orbitcalc())
                {
                    return;
                }
            }
        }
    }

    if (periodicitycheck)
    {
        Bif_Period_Init();
    }
    for (unsigned long counter = 0UL; counter < (unsigned long)maxit ; counter++)
    {
        if (curfractalspecific->orbitcalc())
        {
            return;
        }

        // assign population value to Y coordinate in pixels
        if (g_integer_fractal)
        {
            pixel_row = iystop - (int)((lPopulation - linit.y) / dely); // iystop
        }
        else
        {
            pixel_row = iystop - (int)((Population - g_init.y) / delyy);
        }

        // if it's visible on the screen, save it in the column array
        if (pixel_row <= (unsigned int)iystop)
        {
            verhulst_array[ pixel_row ] ++;
        }
        if (periodicitycheck && Bif_Periodic(counter))
        {
            if (pixel_row <= (unsigned int)iystop)
            {
                verhulst_array[ pixel_row ] --;
            }
            break;
        }
    }
}
static  long    lBif_closenuf, lBif_savedpop;   // poss future use
static  double   Bif_closenuf,  Bif_savedpop;
static  int      Bif_savedinc;
static  long     Bif_savedand;

static void Bif_Period_Init()
{
    Bif_savedinc = 1;
    Bif_savedand = 1;
    if (g_integer_fractal)
    {
        lBif_savedpop = -1;
        lBif_closenuf = dely / 8;
    }
    else
    {
        Bif_savedpop = -1.0;
        Bif_closenuf = (double)delyy / 8.0;
    }
}

// Bifurcation Population Periodicity Check
// Returns : true if periodicity found, else false
static bool Bif_Periodic(long time)
{
    if ((time & Bif_savedand) == 0)      // time to save a new value
    {
        if (g_integer_fractal)
        {
            lBif_savedpop = lPopulation;
        }
        else
        {
            Bif_savedpop =  Population;
        }
        if (--Bif_savedinc == 0)
        {
            Bif_savedand = (Bif_savedand << 1) + 1;
            Bif_savedinc = 4;
        }
    }
    else                         // check against an old save
    {
        if (g_integer_fractal)
        {
            if (labs(lBif_savedpop-lPopulation) <= lBif_closenuf)
            {
                return true;
            }
        }
        else
        {
            if (fabs(Bif_savedpop-Population) <= Bif_closenuf)
            {
                return true;
            }
        }
    }
    return false;
}

//********************************************************************
/*                                                                                                    */
// The following are Bifurcation "orbitcalc" routines...
/*                                                                                                    */
//********************************************************************
#if defined(XFRACT) || defined(_WIN32)
int BifurcLambda() // Used by lyanupov
{
    Population = Rate * Population * (1 - Population);
    return (fabs(Population) > BIG);
}
#endif

#define LCMPLXtrig0(arg, out) Arg1->l = (arg); ltrig0(); (out) = Arg1->l
#define  CMPLXtrig0(arg, out) Arg1->d = (arg); dtrig0(); (out) = Arg1->d

int BifurcVerhulstTrig()
{
    //  Population = Pop + Rate * fn(Pop) * (1 - fn(Pop))
    tmp.x = Population;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population += Rate * tmp.x * (1 - tmp.x);
    return (fabs(Population) > BIG);
}

int LongBifurcVerhulstTrig()
{
#if !defined(XFRACT)
    ltmp.x = lPopulation;
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    ltmp.y = ltmp.x - multiply(ltmp.x, ltmp.x, bitshift);
    lPopulation += multiply(lRate, ltmp.y, bitshift);
#endif
    return (overflow);
}

int BifurcStewartTrig()
{
    //  Population = (Rate * fn(Population) * fn(Population)) - 1.0
    tmp.x = Population;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population = (Rate * tmp.x * tmp.x) - 1.0;
    return (fabs(Population) > BIG);
}

int LongBifurcStewartTrig()
{
#if !defined(XFRACT)
    ltmp.x = lPopulation;
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    lPopulation = multiply(ltmp.x, ltmp.x, bitshift);
    lPopulation = multiply(lPopulation, lRate,      bitshift);
    lPopulation -= g_fudge_factor;
#endif
    return (overflow);
}

int BifurcSetTrigPi()
{
    tmp.x = Population * PI;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population = Rate * tmp.x;
    return (fabs(Population) > BIG);
}

int LongBifurcSetTrigPi()
{
#if !defined(XFRACT)
    ltmp.x = multiply(lPopulation, LPI, bitshift);
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    lPopulation = multiply(lRate, ltmp.x, bitshift);
#endif
    return (overflow);
}

int BifurcAddTrigPi()
{
    tmp.x = Population * PI;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population += Rate * tmp.x;
    return (fabs(Population) > BIG);
}

int LongBifurcAddTrigPi()
{
#if !defined(XFRACT)
    ltmp.x = multiply(lPopulation, LPI, bitshift);
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    lPopulation += multiply(lRate, ltmp.x, bitshift);
#endif
    return (overflow);
}

int BifurcLambdaTrig()
{
    //  Population = Rate * fn(Population) * (1 - fn(Population))
    tmp.x = Population;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population = Rate * tmp.x * (1 - tmp.x);
    return (fabs(Population) > BIG);
}

int LongBifurcLambdaTrig()
{
#if !defined(XFRACT)
    ltmp.x = lPopulation;
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    ltmp.y = ltmp.x - multiply(ltmp.x, ltmp.x, bitshift);
    lPopulation = multiply(lRate, ltmp.y, bitshift);
#endif
    return (overflow);
}

#define LCMPLXpwr(arg1, arg2, out)    Arg2->l = (arg1); Arg1->l = (arg2);\
         lStkPwr(); Arg1++; Arg2++; (out) = Arg2->l

long beta;

int BifurcMay()
{
    /* X = (lambda * X) / (1 + X)^beta, from R.May as described in Pickover,
            Computers, Pattern, Chaos, and Beauty, page 153 */
    tmp.x = 1.0 + Population;
    tmp.x = pow(tmp.x, -beta); // pow in math.h included with mpmath.h
    Population = (Rate * Population) * tmp.x;
    return (fabs(Population) > BIG);
}

int LongBifurcMay()
{
#if !defined(XFRACT)
    ltmp.x = lPopulation + g_fudge_factor;
    ltmp.y = 0;
    lparm2.x = beta * g_fudge_factor;
    LCMPLXpwr(ltmp, lparm2, ltmp);
    lPopulation = multiply(lRate, lPopulation, bitshift);
    lPopulation = divide(lPopulation, ltmp.x, bitshift);
#endif
    return (overflow);
}

bool BifurcMaySetup()
{

    beta = (long)param[2];
    if (beta < 2)
    {
        beta = 2;
    }
    param[2] = (double)beta;

    timer(0, curfractalspecific->calctype);
    return false;
}

// Here Endeth the Generalised Bifurcation Fractal Engine

// END Phil Wilson's Code (modified slightly by Kev Allen et. al. !)


//****************** standalone engine for "popcorn" *******************

int popcorn()   // subset of std engine
{
    int start_row;
    start_row = 0;
    if (resuming)
    {
        start_resume();
        get_resume(sizeof(start_row), &start_row, 0);
        end_resume();
    }
    keyboard_check_interval = max_keyboard_check_interval;
    plot = noplot;
    ltempsqrx = 0;
    tempsqrx = ltempsqrx;
    for (row = start_row; row <= iystop; row++)
    {
        reset_periodicity = true;
        for (col = 0; col <= ixstop; col++)
        {
            if (standard_fractal() == -1) // interrupted
            {
                alloc_resume(10, 1);
                put_resume(sizeof(row), &row, 0);
                return (-1);
            }
            reset_periodicity = false;
        }
    }
    g_calc_status = calc_status_value::COMPLETED;
    return (0);
}

//****************** standalone engine for "lyapunov" ********************
//** save_release behavior:                                             **
//**    1730 & prior: ignores inside=, calcmode='1', (a,b)->(x,y)       **
//**    1731: other calcmodes and inside=nnn                            **
//**    1732: the infamous axis swap: (b,a)->(x,y),                     **
//**            the order parameter becomes a long int                  **
//************************************************************************
int lyaLength, lyaSeedOK;
int lyaRxy[34];

#define WES 1   // define WES to be 0 to use Nick's lyapunov.obj
#if WES
int lyapunov_cycles(double, double);
#else
int lyapunov_cycles(int, double, double, double);
#endif

int lyapunov_cycles_in_c(long, double, double);

int lyapunov()
{
    double a, b;

    if (driver_key_pressed())
    {
        return -1;
    }
    overflow = false;
    if (param[1] == 1)
    {
        Population = (1.0+rand())/(2.0+RAND_MAX);
    }
    else if (param[1] == 0)
    {
        if (fabs(Population)>BIG || Population == 0 || Population == 1)
        {
            Population = (1.0+rand())/(2.0+RAND_MAX);
        }
    }
    else
    {
        Population = param[1];
    }
    (*plot)(col, row, 1);
    if (invert != 0)
    {
        invertz2(&g_init);
        a = g_init.y;
        b = g_init.x;
    }
    else
    {
        a = dypixel();
        b = dxpixel();
    }
#if !defined(XFRACT) && !defined(_WIN32)
    /*  the assembler routines don't work for a & b outside the
        ranges 0 < a < 4 and 0 < b < 4. So, fall back on the C
        routines if part of the image sticks out.
        */
#if WES
    color = lyapunov_cycles(a, b);
#else
    if (lyaSeedOK && a > 0 && b > 0 && a <= 4 && b <= 4)
    {
        color = lyapunov_cycles(filter_cycles, Population, a, b);
    }
    else
    {
        color = lyapunov_cycles_in_c(filter_cycles, a, b);
    }
#endif
#else
    g_color = lyapunov_cycles_in_c(filter_cycles, a, b);
#endif
    if (g_inside > COLOR_BLACK && g_color == 0)
    {
        g_color = g_inside;
    }
    else if (g_color>=g_colors)
    {
        g_color = g_colors-1;
    }
    (*plot)(col, row, g_color);
    return g_color;
}


bool lya_setup()
{
    /* This routine sets up the sequence for forcing the Rate parameter
        to vary between the two values.  It fills the array lyaRxy[] and
        sets lyaLength to the length of the sequence.

        The sequence is coded in the bit pattern in an integer.
        Briefly, the sequence starts with an A the leading zero bits
        are ignored and the remaining bit sequence is decoded.  The
        sequence ends with a B.  Not all possible sequences can be
        represented in this manner, but every possible sequence is
        either represented as itself, as a rotation of one of the
        representable sequences, or as the inverse of a representable
        sequence (swapping 0s and 1s in the array.)  Sequences that
        are the rotation and/or inverses of another sequence will generate
        the same lyapunov exponents.

        A few examples follow:
            number    sequence
                0       ab
                1       aab
                2       aabb
                3       aaab
                4       aabbb
                5       aabab
                6       aaabb (this is a duplicate of 4, a rotated inverse)
                7       aaaab
                8       aabbbb  etc.
         */

    long i;

    filter_cycles = (long)param[2];
    if (filter_cycles == 0)
    {
        filter_cycles = maxit/2;
    }
    lyaSeedOK = param[1] > 0 && param[1] <= 1 && g_debug_flag != debug_flags::force_standard_fractal;
    lyaLength = 1;

    i = (long)param[0];
#if !defined(XFRACT)
    if (save_release < 1732)
    {
        i &= 0x0FFFFL;    // make it a short to reproduce prior stuff
    }
#endif
    lyaRxy[0] = 1;
    int t;
    for (t = 31; t >= 0; t--)
    {
        if (i & (1 << t))
        {
            break;
        }
    }
    for (; t >= 0; t--)
    {
        lyaRxy[lyaLength++] = (i & (1<<t)) != 0;
    }
    lyaRxy[lyaLength++] = 0;
    if (save_release < 1732)                // swap axes prior to 1732
    {
        for (t = lyaLength; t >= 0; t--)
        {
            lyaRxy[t] = !lyaRxy[t];
        }
    }
    if (save_release < 1731)
    {
        // ignore inside=, stdcalcmode
        stdcalcmode = '1';
        if (g_inside == 1)
        {
            g_inside = COLOR_BLACK;
        }
    }
    if (g_inside < COLOR_BLACK)
    {
        stopmsg(STOPMSG_NONE,
            "Sorry, inside options other than inside=nnn are not supported by the lyapunov");
        g_inside = 1;
    }
    if (usr_stdcalcmode == 'o')
    {
        // Oops,lyapunov type
        usr_stdcalcmode = '1';  // doesn't use new & breaks orbits
        stdcalcmode = '1';
    }
    return true;
}

int lyapunov_cycles_in_c(long filter_cycles, double a, double b)
{
    int color, lnadjust;
    double total;
    double temp;
    // e10=22026.4657948  e-10=0.0000453999297625

    total = 1.0;
    lnadjust = 0;
    long i;
    for (i = 0; i < filter_cycles; i++)
    {
        for (int count = 0; count < lyaLength; count++)
        {
            Rate = lyaRxy[count] ? a : b;
            if (curfractalspecific->orbitcalc())
            {
                overflow = true;
                goto jumpout;
            }
        }
    }
    for (i = 0; i < maxit/2; i++)
    {
        for (int count = 0; count < lyaLength; count++)
        {
            Rate = lyaRxy[count] ? a : b;
            if (curfractalspecific->orbitcalc())
            {
                overflow = true;
                goto jumpout;
            }
            temp = fabs(Rate-2.0*Rate*Population);
            total *= temp;
            if (total == 0)
            {
                overflow = true;
                goto jumpout;
            }
        }
        while (total > 22026.4657948)
        {
            total *= 0.0000453999297625;
            lnadjust += 10;
        }
        while (total < 0.0000453999297625)
        {
            total *= 22026.4657948;
            lnadjust -= 10;
        }
    }

jumpout:
    if (overflow || total <= 0 || (temp = log(total) + lnadjust) > 0)
    {
        color = 0;
    }
    else
    {
        double lyap;
        if (LogFlag)
        {
            lyap = -temp/((double) lyaLength*i);
        }
        else
        {
            lyap = 1 - exp(temp/((double) lyaLength*i));
        }
        color = 1 + (int)(lyap * (g_colors-1));
    }
    return color;
}


//****************** standalone engine for "cellular" *******************

#define BAD_T         1
#define BAD_MEM       2
#define STRING1       3
#define STRING2       4
#define TABLEK        5
#define TYPEKR        6
#define RULELENGTH    7
#define INTERUPT      8

#define CELLULAR_DONE 10

static std::vector<BYTE> cell_array[2];

S16 r, k_1, rule_digits;
bool lstscreenflag = false;

void abort_cellular(int err, int t)
{
    int i;
    switch (err)
    {
    case BAD_T:
    {
        char msg[30];
        sprintf(msg, "Bad t=%d, aborting\n", t);
        stopmsg(STOPMSG_NONE, msg);
    }
    break;
    case BAD_MEM:
    {
        stopmsg(STOPMSG_NONE, "Insufficient free memory for calculation");
    }
    break;
    case STRING1:
    {
        stopmsg(STOPMSG_NONE, "String can be a maximum of 16 digits");
    }
    break;
    case STRING2:
    {
        static char msg[] = {"Make string of 0's through  's" };
        msg[27] = (char)(k_1 + 48); // turn into a character value
        stopmsg(STOPMSG_NONE, msg);
    }
    break;
    case TABLEK:
    {
        static char msg[] = {"Make Rule with 0's through  's" };
        msg[27] = (char)(k_1 + 48); // turn into a character value
        stopmsg(STOPMSG_NONE, msg);
    }
    break;
    case TYPEKR:
    {
        stopmsg(STOPMSG_NONE,
            "Type must be 21, 31, 41, 51, 61, 22, 32, 42, 23, 33, 24, 25, 26, 27");
    }
    break;
    case RULELENGTH:
    {
        static char msg[] = {"Rule must be    digits long" };
        i = rule_digits / 10;
        if (i == 0)
        {
            msg[14] = (char)(rule_digits + 48);
        }
        else
        {
            msg[13] = (char)(i+48);
            msg[14] = (char)((rule_digits % 10) + 48);
        }
        stopmsg(STOPMSG_NONE, msg);
    }
    break;
    case INTERUPT:
    {
        stopmsg(STOPMSG_NONE, "Interrupted, can't resume");
    }
    break;
    case CELLULAR_DONE:
        break;
    }
}

int cellular()
{
    S16 start_row;
    S16 filled, notfilled;
    U16 cell_table[32];
    U16 init_string[16];
    U16 kr, k;
    U32 lnnmbr;
    U16 twor;
    S16 t, t2;
    S32 randparam;
    double n;
    char buf[30];

    set_Cellular_palette();

    randparam = (S32)param[0];
    lnnmbr = (U32)param[3];
    kr = (U16)param[2];
    switch (kr)
    {
    case 21:
    case 31:
    case 41:
    case 51:
    case 61:
    case 22:
    case 32:
    case 42:
    case 23:
    case 33:
    case 24:
    case 25:
    case 26:
    case 27:
        break;
    default:
        abort_cellular(TYPEKR, 0);
        return -1;
    }

    r = (S16)(kr % 10); // Number of nearest neighbors to sum
    k = (U16)(kr / 10); // Number of different states, k=3 has states 0,1,2
    k_1 = (S16)(k - 1); // Highest state value, k=3 has highest state value of 2
    rule_digits = (S16)((r * 2 + 1) * k_1 + 1); // Number of digits in the rule

    if (!rflag && randparam == -1)
    {
        --rseed;
    }
    if (randparam != 0 && randparam != -1)
    {
        n = param[0];
        sprintf(buf, "%.16g", n); // # of digits in initial string
        t = (S16)strlen(buf);
        if (t>16 || t <= 0)
        {
            abort_cellular(STRING1, 0);
            return -1;
        }
        for (auto &elem : init_string)
        {
            elem = 0; // zero the array
        }
        t2 = (S16)((16 - t)/2);
        for (int i = 0; i < t; i++)
        {
            // center initial string in array
            init_string[i+t2] = (U16)(buf[i] - 48); // change character to number
            if (init_string[i+t2]>(U16)k_1)
            {
                abort_cellular(STRING2, 0);
                return -1;
            }
        }
    }

    srand(rseed);
    if (!rflag)
    {
        ++rseed;
    }

    // generate rule table from parameter 1
#if !defined(XFRACT)
    n = param[1];
#else
    // gcc can't manage to convert a big double to an unsigned long properly.
    if (param[1]>0x7fffffff)
    {
        n = (param[1]-0x7fffffff);
        n += 0x7fffffff;
    }
    else
    {
        n = param[1];
    }
#endif
    if (n == 0)
    {
        // calculate a random rule
        n = rand()%(int)k;
        for (int i = 1; i < rule_digits; i++)
        {
            n *= 10;
            n += rand()%(int)k;
        }
        param[1] = n;
    }
    sprintf(buf, "%.*g", rule_digits , n);
    t = (S16)strlen(buf);
    if (rule_digits < t || t < 0)
    {
        // leading 0s could make t smaller
        abort_cellular(RULELENGTH, 0);
        return -1;
    }
    for (int i = 0; i < rule_digits; i++)   // zero the table
    {
        cell_table[i] = 0;
    }
    for (int i = 0; i < t; i++)
    {
        // reverse order
        cell_table[i] = (U16)(buf[t-i-1] - 48); // change character to number
        if (cell_table[i]>(U16)k_1)
        {
            abort_cellular(TABLEK, 0);
            return -1;
        }
    }


    start_row = 0;
    bool resized = false;
    try
    {
        cell_array[0].resize(ixstop+1);
        cell_array[1].resize(ixstop+1);
        resized = true;
    }
    catch (std::bad_alloc const&)
    {
    }
    if (!resized)
    {
        abort_cellular(BAD_MEM, 0);
        return (-1);
    }

    // nxtscreenflag toggled by space bar in fractint.c, true for continuous
    // false to stop on next screen

    filled = 0;
    notfilled = (S16)(1-filled);
    if (resuming && !nxtscreenflag && !lstscreenflag)
    {
        start_resume();
        get_resume(sizeof(start_row), &start_row, 0);
        end_resume();
        get_line(start_row, 0, ixstop, &cell_array[filled][0]);
    }
    else if (nxtscreenflag && !lstscreenflag)
    {
        start_resume();
        end_resume();
        get_line(iystop, 0, ixstop, &cell_array[filled][0]);
        param[3] += iystop + 1;
        start_row = -1; // after 1st iteration its = 0
    }
    else
    {
        if (rflag || randparam == 0 || randparam == -1)
        {
            for (col = 0; col <= ixstop; col++)
            {
                cell_array[filled][col] = (BYTE)(rand()%(int)k);
            }
        } // end of if random

        else
        {
            for (col = 0; col <= ixstop; col++)
            {
                // Clear from end to end
                cell_array[filled][col] = 0;
            }
            int i = 0;
            for (col = (ixstop-16)/2; col < (ixstop+16)/2; col++)
            {
                // insert initial
                cell_array[filled][col] = (BYTE)init_string[i++];    // string
            }
        } // end of if not random
        lstscreenflag = lnnmbr != 0;
        put_line(start_row, 0, ixstop, &cell_array[filled][0]);
    }
    start_row++;

    // This section calculates the starting line when it is not zero
    // This section can't be resumed since no screen output is generated
    // calculates the (lnnmbr - 1) generation
    if (lstscreenflag)   // line number != 0 & not resuming & not continuing
    {
        for (U32 big_row = (U32)start_row; big_row < lnnmbr; big_row++)
        {
            thinking(1, "Cellular thinking (higher start row takes longer)");
            if (rflag || randparam == 0 || randparam == -1)
            {
                // Use a random border
                for (int i = 0; i <= r; i++)
                {
                    cell_array[notfilled][i] = (BYTE)(rand()%(int)k);
                    cell_array[notfilled][ixstop-i] = (BYTE)(rand()%(int)k);
                }
            }
            else
            {
                // Use a zero border
                for (int i = 0; i <= r; i++)
                {
                    cell_array[notfilled][i] = 0;
                    cell_array[notfilled][ixstop-i] = 0;
                }
            }

            t = 0; // do first cell
            twor = (U16)(r+r);
            for (int i = 0; i <= twor; i++)
            {
                t = (S16)(t + (S16)cell_array[filled][i]);
            }
            if (t > rule_digits || t < 0)
            {
                thinking(0, nullptr);
                abort_cellular(BAD_T, t);
                return (-1);
            }
            cell_array[notfilled][r] = (BYTE)cell_table[t];

            // use a rolling sum in t
            for (col = r+1; col < ixstop-r; col++)
            {
                // now do the rest
                t = (S16)(t + cell_array[filled][col+r] - cell_array[filled][col-r-1]);
                if (t > rule_digits || t < 0)
                {
                    thinking(0, nullptr);
                    abort_cellular(BAD_T, t);
                    return (-1);
                }
                cell_array[notfilled][col] = (BYTE)cell_table[t];
            }

            filled = notfilled;
            notfilled = (S16)(1-filled);
            if (driver_key_pressed())
            {
                thinking(0, nullptr);
                abort_cellular(INTERUPT, 0);
                return -1;
            }
        }
        start_row = 0;
        thinking(0, nullptr);
        lstscreenflag = false;
    }

    // This section does all the work
contloop:
    for (row = start_row; row <= iystop; row++)
    {
        if (rflag || randparam == 0 || randparam == -1)
        {
            // Use a random border
            for (int i = 0; i <= r; i++)
            {
                cell_array[notfilled][i] = (BYTE)(rand()%(int)k);
                cell_array[notfilled][ixstop-i] = (BYTE)(rand()%(int)k);
            }
        }
        else
        {
            // Use a zero border
            for (int i = 0; i <= r; i++)
            {
                cell_array[notfilled][i] = 0;
                cell_array[notfilled][ixstop-i] = 0;
            }
        }

        t = 0; // do first cell
        twor = (U16)(r+r);
        for (int i = 0; i <= twor; i++)
        {
            t = (S16)(t + (S16)cell_array[filled][i]);
        }
        if (t > rule_digits || t < 0)
        {
            thinking(0, nullptr);
            abort_cellular(BAD_T, t);
            return (-1);
        }
        cell_array[notfilled][r] = (BYTE)cell_table[t];

        // use a rolling sum in t
        for (col = r+1; col < ixstop-r; col++)
        {
            // now do the rest
            t = (S16)(t + cell_array[filled][col+r] - cell_array[filled][col-r-1]);
            if (t > rule_digits || t < 0)
            {
                thinking(0, nullptr);
                abort_cellular(BAD_T, t);
                return (-1);
            }
            cell_array[notfilled][col] = (BYTE)cell_table[t];
        }

        filled = notfilled;
        notfilled = (S16)(1-filled);
        put_line(row, 0, ixstop, &cell_array[filled][0]);
        if (driver_key_pressed())
        {
            abort_cellular(CELLULAR_DONE, 0);
            alloc_resume(10, 1);
            put_resume(sizeof(row), &row, 0);
            return -1;
        }
    }
    if (nxtscreenflag)
    {
        param[3] += iystop + 1;
        start_row = 0;
        goto contloop;
    }
    abort_cellular(CELLULAR_DONE, 0);
    return 1;
}

bool CellularSetup()
{
    if (!resuming)
    {
        nxtscreenflag = false; // initialize flag
    }
    timer(0, curfractalspecific->calctype);
    return false;
}

static void set_Cellular_palette()
{
    static BYTE const Red[3]    = { 42, 0, 0 };
    static BYTE const Green[3]  = { 10, 35, 10 };
    static BYTE const Blue[3]   = { 13, 12, 29 };
    static BYTE const Yellow[3] = { 60, 58, 18 };
    static BYTE const Brown[3]  = { 42, 21, 0 };

    if (map_specified && g_color_state != 0)
    {
        return;       // map= specified
    }

    g_dac_box[0][0] = 0;
    g_dac_box[0][1] = 0;
    g_dac_box[0][2] = 0;

    g_dac_box[1][0] = Red[0];
    g_dac_box[1][1] = Red[1];
    g_dac_box[1][2] = Red[2];
    g_dac_box[2][0] = Green[0];
    g_dac_box[2][1] = Green[1];
    g_dac_box[2][2] = Green[2];
    g_dac_box[3][0] = Blue[0];
    g_dac_box[3][1] = Blue[1];
    g_dac_box[3][2] = Blue[2];
    g_dac_box[4][0] = Yellow[0];
    g_dac_box[4][1] = Yellow[1];
    g_dac_box[4][2] = Yellow[2];
    g_dac_box[5][0] = Brown[0];
    g_dac_box[5][1] = Brown[1];
    g_dac_box[5][2] = Brown[2];

    spindac(0, 1);
}

// frothy basin routines

#define FROTH_BITSHIFT      28
#define FROTH_D_TO_L(x)     ((long)((x)*(1L<<FROTH_BITSHIFT)))
#define FROTH_CLOSE         1e-6      // seems like a good value
#define FROTH_LCLOSE        FROTH_D_TO_L(FROTH_CLOSE)
#define SQRT3               1.732050807568877193
#define FROTH_SLOPE         SQRT3
#define FROTH_LSLOPE        FROTH_D_TO_L(FROTH_SLOPE)
#define FROTH_CRITICAL_A    1.028713768218725  // 1.0287137682187249127
#define froth_top_x_mapping(x)  ((x)*(x)-(x)-3*fsp.fl.f.a*fsp.fl.f.a/4)


struct froth_double_struct
{
    double a;
    double halfa;
    double top_x1;
    double top_x2;
    double top_x3;
    double top_x4;
    double left_x1;
    double left_x2;
    double left_x3;
    double left_x4;
    double right_x1;
    double right_x2;
    double right_x3;
    double right_x4;
};

struct froth_long_struct
{
    long a;
    long halfa;
    long top_x1;
    long top_x2;
    long top_x3;
    long top_x4;
    long left_x1;
    long left_x2;
    long left_x3;
    long left_x4;
    long right_x1;
    long right_x2;
    long right_x3;
    long right_x4;
};

struct froth_struct
{
    int repeat_mapping;
    int altcolor;
    int attractors;
    int shades;
    union
    {
        froth_double_struct f;
        froth_long_struct l;
    } fl;
};

namespace
{
froth_struct fsp;
}

// color maps which attempt to replicate the images of James Alexander.
static void set_Froth_palette()
{
    char const *mapname;

    if (g_color_state != 0)   // 0 means g_dac_box matches default
    {
        return;
    }
    if (g_colors >= 16)
    {
        if (g_colors >= 256)
        {
            if (fsp.attractors == 6)
            {
                mapname = "froth6.map";
            }
            else
            {
                mapname = "froth3.map";

            }
        }
        else // colors >= 16
        {
            if (fsp.attractors == 6)
            {
                mapname = "froth616.map";
            }
            else
            {
                mapname = "froth316.map";

            }
        }
        if (ValidateLuts(mapname))
        {
            return;
        }
        g_color_state = 0; // treat map as default
        spindac(0, 1);
    }
}

bool froth_setup()
{
    double sin_theta;
    double cos_theta;

    sin_theta = SQRT3/2; // sin(2*PI/3)
    cos_theta = -0.5;    // cos(2*PI/3)

    // for the all important backwards compatibility
    if (save_release <= 1821)   // book version is 18.21
    {
        // use old release parameters

        fsp.repeat_mapping = ((int)param[0] == 6 || (int)param[0] == 2); // map 1 or 2 times (3 or 6 basins)
        fsp.altcolor = (int)param[1];
        param[2] = 0; // throw away any value used prior to 18.20

        fsp.attractors = !fsp.repeat_mapping ? 3 : 6;

        // use old values */                /* old names
        fsp.fl.f.a = 1.02871376822;          // A
        fsp.fl.f.halfa = fsp.fl.f.a/2;      // A/2

        fsp.fl.f.top_x1 = -1.04368901270;    // X1MIN
        fsp.fl.f.top_x2 =  1.33928675524;    // X1MAX
        fsp.fl.f.top_x3 = -0.339286755220;   // XMIDT
        fsp.fl.f.top_x4 = -0.339286755220;   // XMIDT

        fsp.fl.f.left_x1 =  0.07639837810;   // X3MAX2
        fsp.fl.f.left_x2 = -1.11508950586;   // X2MIN2
        fsp.fl.f.left_x3 = -0.27580275066;   // XMIDL
        fsp.fl.f.left_x4 = -0.27580275066;   // XMIDL

        fsp.fl.f.right_x1 =  0.96729063460;  // X2MAX1
        fsp.fl.f.right_x2 = -0.22419724936;  // X3MIN1
        fsp.fl.f.right_x3 =  0.61508950585;  // XMIDR
        fsp.fl.f.right_x4 =  0.61508950585;  // XMIDR

    }
    else // use new code
    {
        if (param[0] != 2)
        {
            param[0] = 1;
        }
        fsp.repeat_mapping = (int)param[0] == 2;
        if (param[1] != 0)
        {
            param[1] = 1;
        }
        fsp.altcolor = (int)param[1];
        fsp.fl.f.a = param[2];

        fsp.attractors = fabs(fsp.fl.f.a) <= FROTH_CRITICAL_A ? (!fsp.repeat_mapping ? 3 : 6)
                          : (!fsp.repeat_mapping ? 2 : 3);

        // new improved values
        // 0.5 is the value that causes the mapping to reach a minimum
        double x0 = 0.5;
        // a/2 is the value that causes the y value to be invariant over the mappings
        fsp.fl.f.halfa = fsp.fl.f.a/2;
        double y0 = fsp.fl.f.halfa;
        fsp.fl.f.top_x1 = froth_top_x_mapping(x0);
        fsp.fl.f.top_x2 = froth_top_x_mapping(fsp.fl.f.top_x1);
        fsp.fl.f.top_x3 = froth_top_x_mapping(fsp.fl.f.top_x2);
        fsp.fl.f.top_x4 = froth_top_x_mapping(fsp.fl.f.top_x3);

        // rotate 120 degrees counter-clock-wise
        fsp.fl.f.left_x1 = fsp.fl.f.top_x1 * cos_theta - y0 * sin_theta;
        fsp.fl.f.left_x2 = fsp.fl.f.top_x2 * cos_theta - y0 * sin_theta;
        fsp.fl.f.left_x3 = fsp.fl.f.top_x3 * cos_theta - y0 * sin_theta;
        fsp.fl.f.left_x4 = fsp.fl.f.top_x4 * cos_theta - y0 * sin_theta;

        // rotate 120 degrees clock-wise
        fsp.fl.f.right_x1 = fsp.fl.f.top_x1 * cos_theta + y0 * sin_theta;
        fsp.fl.f.right_x2 = fsp.fl.f.top_x2 * cos_theta + y0 * sin_theta;
        fsp.fl.f.right_x3 = fsp.fl.f.top_x3 * cos_theta + y0 * sin_theta;
        fsp.fl.f.right_x4 = fsp.fl.f.top_x4 * cos_theta + y0 * sin_theta;

    }

    // if 2 attractors, use same shades as 3 attractors
    fsp.shades = (g_colors-1) / std::max(3, fsp.attractors);

    // rqlim needs to be at least sq(1+sqrt(1+sq(a))),
    // which is never bigger than 6.93..., so we'll call it 7.0
    if (rqlim < 7.0)
    {
        rqlim = 7.0;
    }
    set_Froth_palette();
    // make the best of the .map situation
    orbit_color = fsp.attractors != 6 && g_colors >= 16 ? (fsp.shades<<1)+1 : g_colors-1;

    if (g_integer_fractal)
    {
        froth_long_struct tmp_l;

        tmp_l.a        = FROTH_D_TO_L(fsp.fl.f.a);
        tmp_l.halfa    = FROTH_D_TO_L(fsp.fl.f.halfa);

        tmp_l.top_x1   = FROTH_D_TO_L(fsp.fl.f.top_x1);
        tmp_l.top_x2   = FROTH_D_TO_L(fsp.fl.f.top_x2);
        tmp_l.top_x3   = FROTH_D_TO_L(fsp.fl.f.top_x3);
        tmp_l.top_x4   = FROTH_D_TO_L(fsp.fl.f.top_x4);

        tmp_l.left_x1  = FROTH_D_TO_L(fsp.fl.f.left_x1);
        tmp_l.left_x2  = FROTH_D_TO_L(fsp.fl.f.left_x2);
        tmp_l.left_x3  = FROTH_D_TO_L(fsp.fl.f.left_x3);
        tmp_l.left_x4  = FROTH_D_TO_L(fsp.fl.f.left_x4);

        tmp_l.right_x1 = FROTH_D_TO_L(fsp.fl.f.right_x1);
        tmp_l.right_x2 = FROTH_D_TO_L(fsp.fl.f.right_x2);
        tmp_l.right_x3 = FROTH_D_TO_L(fsp.fl.f.right_x3);
        tmp_l.right_x4 = FROTH_D_TO_L(fsp.fl.f.right_x4);

        fsp.fl.l = tmp_l;
    }
    return true;
}

void froth_cleanup()
{
}


// Froth Fractal type
int calcfroth()   // per pixel 1/2/g, called with row & col set
{
    int found_attractor = 0;

    if (check_key())
    {
        return -1;
    }

    orbit_ptr = 0;
    g_color_iter = 0;
    if (show_dot >0)
    {
        (*plot)(col, row, show_dot %g_colors);
    }
    if (!g_integer_fractal) // fp mode
    {
        if (invert != 0)
        {
            invertz2(&tmp);
            old = tmp;
        }
        else
        {
            old.x = dxpixel();
            old.y = dypixel();
        }

        tempsqrx = sqr(old.x);
        tempsqry = sqr(old.y);
        while (!found_attractor
                && (tempsqrx + tempsqry < rqlim)
                && (g_color_iter < maxit))
        {
            // simple formula: z = z^2 + conj(z*(-1+ai))
            // but it's the attractor that makes this so interesting
            g_new.x = tempsqrx - tempsqry - old.x - fsp.fl.f.a*old.y;
            old.y += (old.x+old.x)*old.y - fsp.fl.f.a*old.x;
            old.x = g_new.x;
            if (fsp.repeat_mapping)
            {
                g_new.x = sqr(old.x) - sqr(old.y) - old.x - fsp.fl.f.a*old.y;
                old.y += (old.x+old.x)*old.y - fsp.fl.f.a*old.x;
                old.x = g_new.x;
            }

            g_color_iter++;

            if (show_orbit)
            {
                if (driver_key_pressed())
                {
                    break;
                }
                plot_orbit(old.x, old.y, -1);
            }

            if (fabs(fsp.fl.f.halfa-old.y) < FROTH_CLOSE
                    && old.x >= fsp.fl.f.top_x1 && old.x <= fsp.fl.f.top_x2)
            {
                if ((!fsp.repeat_mapping && fsp.attractors == 2)
                        || (fsp.repeat_mapping && fsp.attractors == 3))
                {
                    found_attractor = 1;
                }
                else if (old.x <= fsp.fl.f.top_x3)
                {
                    found_attractor = 1;
                }
                else if (old.x >= fsp.fl.f.top_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 1;
                    }
                    else
                    {
                        found_attractor = 2;
                    }
                }
            }
            else if (fabs(FROTH_SLOPE*old.x - fsp.fl.f.a - old.y) < FROTH_CLOSE
                     && old.x <= fsp.fl.f.right_x1 && old.x >= fsp.fl.f.right_x2)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 3;
                }
                else if (old.x >= fsp.fl.f.right_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 4;
                    }
                }
                else if (old.x <= fsp.fl.f.right_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 6;
                    }
                }
            }
            else if (fabs(-FROTH_SLOPE*old.x - fsp.fl.f.a - old.y) < FROTH_CLOSE
                     && old.x <= fsp.fl.f.left_x1 && old.x >= fsp.fl.f.left_x2)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 2;
                }
                else if (old.x >= fsp.fl.f.left_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 5;
                    }
                }
                else if (old.x <= fsp.fl.f.left_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 3;
                    }
                }
            }
            tempsqrx = sqr(old.x);
            tempsqry = sqr(old.y);
        }
    }
    else // integer mode
    {
        if (invert != 0)
        {
            invertz2(&tmp);
            lold.x = (long)(tmp.x * g_fudge_factor);
            lold.y = (long)(tmp.y * g_fudge_factor);
        }
        else
        {
            lold.x = lxpixel();
            lold.y = lypixel();
        }

        ltempsqrx = lsqr(lold.x);
        ltempsqry = lsqr(lold.y);
        lmagnitud = ltempsqrx + ltempsqry;
        while (!found_attractor && (lmagnitud < llimit)
                && (lmagnitud >= 0) && (g_color_iter < maxit))
        {
            // simple formula: z = z^2 + conj(z*(-1+ai))
            // but it's the attractor that makes this so interesting
            lnew.x = ltempsqrx - ltempsqry - lold.x - multiply(fsp.fl.l.a, lold.y, bitshift);
            lold.y += (multiply(lold.x, lold.y, bitshift)<<1) - multiply(fsp.fl.l.a, lold.x, bitshift);
            lold.x = lnew.x;
            if (fsp.repeat_mapping)
            {
                lmagnitud = (ltempsqrx = lsqr(lold.x)) + (ltempsqry = lsqr(lold.y));
                if ((lmagnitud > llimit) || (lmagnitud < 0))
                {
                    break;
                }
                lnew.x = ltempsqrx - ltempsqry - lold.x - multiply(fsp.fl.l.a, lold.y, bitshift);
                lold.y += (multiply(lold.x, lold.y, bitshift)<<1) - multiply(fsp.fl.l.a, lold.x, bitshift);
                lold.x = lnew.x;
            }
            g_color_iter++;

            if (show_orbit)
            {
                if (driver_key_pressed())
                {
                    break;
                }
                iplot_orbit(lold.x, lold.y, -1);
            }

            if (labs(fsp.fl.l.halfa-lold.y) < FROTH_LCLOSE
                    && lold.x > fsp.fl.l.top_x1 && lold.x < fsp.fl.l.top_x2)
            {
                if ((!fsp.repeat_mapping && fsp.attractors == 2)
                        || (fsp.repeat_mapping && fsp.attractors == 3))
                {
                    found_attractor = 1;
                }
                else if (lold.x <= fsp.fl.l.top_x3)
                {
                    found_attractor = 1;
                }
                else if (lold.x >= fsp.fl.l.top_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 1;
                    }
                    else
                    {
                        found_attractor = 2;
                    }
                }
            }
            else if (labs(multiply(FROTH_LSLOPE, lold.x, bitshift)-fsp.fl.l.a-lold.y) < FROTH_LCLOSE
                     && lold.x <= fsp.fl.l.right_x1 && lold.x >= fsp.fl.l.right_x2)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 3;
                }
                else if (lold.x >= fsp.fl.l.right_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 4;
                    }
                }
                else if (lold.x <= fsp.fl.l.right_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 6;
                    }
                }
            }
            else if (labs(multiply(-FROTH_LSLOPE, lold.x, bitshift)-fsp.fl.l.a-lold.y) < FROTH_LCLOSE)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 2;
                }
                else if (lold.x >= fsp.fl.l.left_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 5;
                    }
                }
                else if (lold.x <= fsp.fl.l.left_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 3;
                    }
                }
            }
            ltempsqrx = lsqr(lold.x);
            ltempsqry = lsqr(lold.y);
            lmagnitud = ltempsqrx + ltempsqry;
        }
    }
    if (show_orbit)
    {
        scrub_orbit();
    }

    realcoloriter = g_color_iter;
    if ((keyboard_check_interval -= abs((int)realcoloriter)) <= 0)
    {
        if (check_key())
        {
            return (-1);
        }
        keyboard_check_interval = max_keyboard_check_interval;
    }

    // inside - Here's where non-palette based images would be nice.  Instead,
    // we'll use blocks of (colors-1)/3 or (colors-1)/6 and use special froth
    // color maps in attempt to replicate the images of James Alexander.
    if (found_attractor)
    {
        if (g_colors >= 256)
        {
            if (!fsp.altcolor)
            {
                if (g_color_iter > fsp.shades)
                {
                    g_color_iter = fsp.shades;
                }
            }
            else
            {
                g_color_iter = fsp.shades * g_color_iter / maxit;
            }
            if (g_color_iter == 0)
            {
                g_color_iter = 1;
            }
            g_color_iter += fsp.shades * (found_attractor-1);
        }
        else if (g_colors >= 16)
        {
            // only alternate coloring scheme available for 16 colors
            long lshade;

            // Trying to make a better 16 color distribution.
            // Since their are only a few possiblities, just handle each case.
            // This is a mostly guess work here.
            lshade = (g_color_iter<<16)/maxit;
            if (fsp.attractors != 6) // either 2 or 3 attractors
            {
                if (lshade < 2622)         // 0.04
                {
                    g_color_iter = 1;
                }
                else if (lshade < 10486)     // 0.16
                {
                    g_color_iter = 2;
                }
                else if (lshade < 23593)     // 0.36
                {
                    g_color_iter = 3;
                }
                else if (lshade < 41943L)     // 0.64
                {
                    g_color_iter = 4;
                }
                else
                {
                    g_color_iter = 5;
                }
                g_color_iter += 5 * (found_attractor-1);
            }
            else // 6 attractors
            {
                if (lshade < 10486)        // 0.16
                {
                    g_color_iter = 1;
                }
                else
                {
                    g_color_iter = 2;
                }
                g_color_iter += 2 * (found_attractor-1);
            }
        }
        else   // use a color corresponding to the attractor
        {
            g_color_iter = found_attractor;
        }
        oldcoloriter = g_color_iter;
    }
    else   // outside, or inside but didn't get sucked in by attractor.
    {
        g_color_iter = 0;
    }

    g_color = abs((int)(g_color_iter));

    (*plot)(col, row, g_color);

    return g_color;
}

/*
These last two froth functions are for the orbit-in-window feature.
Normally, this feature requires standard_fractal, but since it is the
attractor that makes the frothybasin type so unique, it is worth
putting in as a stand-alone.
*/

int froth_per_pixel()
{
    if (!g_integer_fractal) // fp mode
    {
        old.x = dxpixel();
        old.y = dypixel();
        tempsqrx = sqr(old.x);
        tempsqry = sqr(old.y);
    }
    else  // integer mode
    {
        lold.x = lxpixel();
        lold.y = lypixel();
        ltempsqrx = multiply(lold.x, lold.x, bitshift);
        ltempsqry = multiply(lold.y, lold.y, bitshift);
    }
    return 0;
}

int froth_per_orbit()
{
    if (!g_integer_fractal) // fp mode
    {
        g_new.x = tempsqrx - tempsqry - old.x - fsp.fl.f.a*old.y;
        g_new.y = 2.0*old.x*old.y - fsp.fl.f.a*old.x + old.y;
        if (fsp.repeat_mapping)
        {
            old = g_new;
            g_new.x = sqr(old.x) - sqr(old.y) - old.x - fsp.fl.f.a*old.y;
            g_new.y = 2.0*old.x*old.y - fsp.fl.f.a*old.x + old.y;
        }

        tempsqrx = sqr(g_new.x);
        tempsqry = sqr(g_new.y);
        if (tempsqrx + tempsqry >= rqlim)
        {
            return 1;
        }
        old = g_new;
    }
    else  // integer mode
    {
        lnew.x = ltempsqrx - ltempsqry - lold.x - multiply(fsp.fl.l.a, lold.y, bitshift);
        lnew.y = lold.y + (multiply(lold.x, lold.y, bitshift)<<1) - multiply(fsp.fl.l.a, lold.x, bitshift);
        if (fsp.repeat_mapping)
        {
            ltempsqrx = lsqr(lnew.x);
            ltempsqry = lsqr(lnew.y);
            if (ltempsqrx + ltempsqry >= llimit)
            {
                return 1;
            }
            lold = lnew;
            lnew.x = ltempsqrx - ltempsqry - lold.x - multiply(fsp.fl.l.a, lold.y, bitshift);
            lnew.y = lold.y + (multiply(lold.x, lold.y, bitshift)<<1) - multiply(fsp.fl.l.a, lold.x, bitshift);
        }
        ltempsqrx = lsqr(lnew.x);
        ltempsqry = lsqr(lnew.y);
        if (ltempsqrx + ltempsqry >= llimit)
        {
            return 1;
        }
        lold = lnew;
    }
    return 0;
}
