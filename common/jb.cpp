#include <float.h>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
#include "drivers.h"

// these need to be accessed elsewhere for saving data
double mxminfp = -.83;
double myminfp = -.25;
double mxmaxfp = -.83;
double mymaxfp =  .25;

static long mxmin, mymin;
static long x_per_inch, y_per_inch, inch_per_xdot, inch_per_ydot;
static double x_per_inchfp, y_per_inchfp, inch_per_xdotfp, inch_per_ydotfp;
static int bbase;
static long xpixel, ypixel;
static double xpixelfp, ypixelfp;
static long initz, djx, djy, dmx, dmy;
static double initzfp, djxfp, djyfp, dmxfp, dmyfp;
static long jx, jy, mx, my, xoffset, yoffset;
static double jxfp, jyfp, mxfp, myfp, xoffsetfp, yoffsetfp;

struct Perspective
{
    long x, y, zx, zy;
};

struct Perspectivefp
{
    double x, y, zx, zy;
};

Perspective LeftEye, RightEye, *Per;
Perspectivefp LeftEyefp, RightEyefp, *Perfp;

LComplex jbc;
DComplex jbcfp;

#ifndef XFRACT
static double fg, fg16;
#endif
int zdots = 128;

float originfp  = 8.0F;
float heightfp  = 7.0F;
float widthfp   = 10.0F;
float g_dist_fp    = 24.0F;
float g_eyes_fp    = 2.5F;
float g_depth_fp   = 8.0F;
float brratiofp = 1.0F;
static long width, dist, depth, brratio;
#ifndef XFRACT
static long eyes;
#endif
int juli3Dmode = 0;

fractal_type neworbittype = fractal_type::JULIA;

bool
JulibrotSetup()
{
    int r = 0;
    char const *mapname;

#ifndef XFRACT
    if (g_colors < 255)
    {
        stopmsg(STOPMSG_NONE, "Sorry, but Julibrots require a 256-color video mode");
        return false;
    }
#endif

    xoffsetfp = (xxmax + xxmin) / 2;     // Calculate average
    yoffsetfp = (yymax + yymin) / 2;     // Calculate average
    dmxfp = (mxmaxfp - mxminfp) / zdots;
    dmyfp = (mymaxfp - myminfp) / zdots;
    g_float_param = &jbcfp;
    x_per_inchfp = (xxmin - xxmax) / widthfp;
    y_per_inchfp = (yymax - yymin) / heightfp;
    inch_per_xdotfp = widthfp / xdots;
    inch_per_ydotfp = heightfp / ydots;
    initzfp = originfp - (g_depth_fp / 2);
    if (juli3Dmode == 0)
    {
        RightEyefp.x = 0.0;
    }
    else
    {
        RightEyefp.x = g_eyes_fp / 2;
    }
    LeftEyefp.x = -RightEyefp.x;
    RightEyefp.y = 0;
    LeftEyefp.y = RightEyefp.y;
    RightEyefp.zx = g_dist_fp;
    LeftEyefp.zx = RightEyefp.zx;
    RightEyefp.zy = g_dist_fp;
    LeftEyefp.zy = RightEyefp.zy;
    bbase = 128;

#ifndef XFRACT
    if (fractalspecific[static_cast<int>(fractype)].isinteger > 0)
    {
        long jxmin, jxmax, jymin, jymax, mxmax, mymax;
        if (fractalspecific[static_cast<int>(neworbittype)].isinteger == 0)
        {
            stopmsg(STOPMSG_NONE, "Julibrot orbit type isinteger mismatch");
        }
        if (fractalspecific[static_cast<int>(neworbittype)].isinteger > 1)
        {
            bitshift = fractalspecific[static_cast<int>(neworbittype)].isinteger;
        }
        fg = (double)(1L << bitshift);
        fg16 = (double)(1L << 16);
        jxmin = (long)(xxmin * fg);
        jxmax = (long)(xxmax * fg);
        xoffset = (jxmax + jxmin) / 2;    // Calculate average
        jymin = (long)(yymin * fg);
        jymax = (long)(yymax * fg);
        yoffset = (jymax + jymin) / 2;    // Calculate average
        mxmin = (long)(mxminfp * fg);
        mxmax = (long)(mxmaxfp * fg);
        mymin = (long)(myminfp * fg);
        mymax = (long)(mymaxfp * fg);
        long origin = (long)(originfp * fg16);
        depth = (long)(g_depth_fp * fg16);
        width = (long)(widthfp * fg16);
        dist = (long)(g_dist_fp * fg16);
        eyes = (long)(g_eyes_fp * fg16);
        brratio = (long)(brratiofp * fg16);
        dmx = (mxmax - mxmin) / zdots;
        dmy = (mymax - mymin) / zdots;
        longparm = &jbc;

        x_per_inch = (long)((xxmin - xxmax) / widthfp * fg);
        y_per_inch = (long)((yymax - yymin) / heightfp * fg);
        inch_per_xdot = (long)((widthfp / xdots) * fg16);
        inch_per_ydot = (long)((heightfp / ydots) * fg16);
        initz = origin - (depth / 2);
        if (juli3Dmode == 0)
        {
            RightEye.x = 0L;
        }
        else
        {
            RightEye.x = eyes / 2;
        }
        LeftEye.x = -RightEye.x;
        RightEye.y = 0L;
        LeftEye.y = RightEye.y;
        RightEye.zx = dist;
        LeftEye.zx = RightEye.zx;
        RightEye.zy = dist;
        LeftEye.zy = RightEye.zy;
        bbase = (int)(128.0 * brratiofp);
    }
#endif

    if (juli3Dmode == 3)
    {
        savedac = 0;
        mapname = g_glasses1_map.c_str();
    }
    else
    {
        mapname = GreyFile.c_str();
    }
    if (savedac != 1)
    {
        if (ValidateLuts(mapname))
        {
            return false;
        }
        spindac(0, 1);               // load it, but don't spin
        if (savedac == 2)
        {
            savedac = 1;
        }
    }
    return r >= 0;
}


int
jb_per_pixel()
{
    jx = multiply(Per->x - xpixel, initz, 16);
    jx = divide(jx, dist, 16) - xpixel;
    jx = multiply(jx << (bitshift - 16), x_per_inch, bitshift);
    jx += xoffset;
    djx = divide(depth, dist, 16);
    djx = multiply(djx, Per->x - xpixel, 16) << (bitshift - 16);
    djx = multiply(djx, x_per_inch, bitshift) / zdots;

    jy = multiply(Per->y - ypixel, initz, 16);
    jy = divide(jy, dist, 16) - ypixel;
    jy = multiply(jy << (bitshift - 16), y_per_inch, bitshift);
    jy += yoffset;
    djy = divide(depth, dist, 16);
    djy = multiply(djy, Per->y - ypixel, 16) << (bitshift - 16);
    djy = multiply(djy, y_per_inch, bitshift) / zdots;

    return (1);
}

int
jbfp_per_pixel()
{
    jxfp = ((Perfp->x - xpixelfp) * initzfp / g_dist_fp - xpixelfp) * x_per_inchfp;
    jxfp += xoffsetfp;
    djxfp = (g_depth_fp / g_dist_fp) * (Perfp->x - xpixelfp) * x_per_inchfp / zdots;

    jyfp = ((Perfp->y - ypixelfp) * initzfp / g_dist_fp - ypixelfp) * y_per_inchfp;
    jyfp += yoffsetfp;
    djyfp = g_depth_fp / g_dist_fp * (Perfp->y - ypixelfp) * y_per_inchfp / zdots;

    return (1);
}

static int zpixel, plotted;
static long n;

int
zline(long x, long y)
{
    xpixel = x;
    ypixel = y;
    mx = mxmin;
    my = mymin;
    switch (juli3Dmode)
    {
    case 0:
    case 1:
        Per = &LeftEye;
        break;
    case 2:
        Per = &RightEye;
        break;
    case 3:
        if ((row + col) & 1)
        {
            Per = &LeftEye;
        }
        else
        {
            Per = &RightEye;
        }
        break;
    }
    jb_per_pixel();
    for (zpixel = 0; zpixel < zdots; zpixel++)
    {
        lold.x = jx;
        lold.y = jy;
        jbc.x = mx;
        jbc.y = my;
        if (driver_key_pressed())
        {
            return (-1);
        }
        ltempsqrx = multiply(lold.x, lold.x, bitshift);
        ltempsqry = multiply(lold.y, lold.y, bitshift);
        for (n = 0; n < maxit; n++)
        {
            if (fractalspecific[static_cast<int>(neworbittype)].orbitcalc())
            {
                break;
            }
        }
        if (n == maxit)
        {
            if (juli3Dmode == 3)
            {
                g_color = (int)(128l * zpixel / zdots);
                if ((row + col) & 1)
                {

                    (*plot)(col, row, 127 - g_color);
                }
                else
                {
                    g_color = (int)(multiply((long) g_color << 16, brratio, 16) >> 16);
                    if (g_color < 1)
                    {
                        g_color = 1;
                    }
                    if (g_color > 127)
                    {
                        g_color = 127;
                    }
                    (*plot)(col, row, 127 + bbase - g_color);
                }
            }
            else
            {
                g_color = (int)(254l * zpixel / zdots);
                (*plot)(col, row, g_color + 1);
            }
            plotted = 1;
            break;
        }
        mx += dmx;
        my += dmy;
        jx += djx;
        jy += djy;
    }
    return (0);
}

int
zlinefp(double x, double y)
{
#ifdef XFRACT
    static int keychk = 0;
#endif
    xpixelfp = x;
    ypixelfp = y;
    mxfp = mxminfp;
    myfp = myminfp;
    switch (juli3Dmode)
    {
    case 0:
    case 1:
        Perfp = &LeftEyefp;
        break;
    case 2:
        Perfp = &RightEyefp;
        break;
    case 3:
        if ((row + col) & 1)
        {
            Perfp = &LeftEyefp;
        }
        else
        {
            Perfp = &RightEyefp;
        }
        break;
    }
    jbfp_per_pixel();
    for (zpixel = 0; zpixel < zdots; zpixel++)
    {
        // Special initialization for Mandelbrot types
        if ((neworbittype == fractal_type::QUATFP || neworbittype == fractal_type::HYPERCMPLXFP)
                && save_release > 2002)
        {
            old.x = 0.0;
            old.y = 0.0;
            jbcfp.x = 0.0;
            jbcfp.y = 0.0;
            qc = jxfp;
            qci = jyfp;
            qcj = mxfp;
            qck = myfp;
        }
        else
        {
            old.x = jxfp;
            old.y = jyfp;
            jbcfp.x = mxfp;
            jbcfp.y = myfp;
            qc = param[0];
            qci = param[1];
            qcj = param[2];
            qck = param[3];
        }
#ifdef XFRACT
        if (keychk++ > 500)
        {
            keychk = 0;
            if (driver_key_pressed())
            {
                return (-1);
            }
        }
#else
        if (driver_key_pressed())
        {
            return (-1);
        }
#endif
        tempsqrx = sqr(old.x);
        tempsqry = sqr(old.y);

        for (n = 0; n < maxit; n++)
        {
            if (fractalspecific[static_cast<int>(neworbittype)].orbitcalc())
            {
                break;
            }
        }
        if (n == maxit)
        {
            if (juli3Dmode == 3)
            {
                g_color = (int)(128l * zpixel / zdots);
                if ((row + col) & 1)
                {
                    (*plot)(col, row, 127 - g_color);
                }
                else
                {
                    g_color = (int)(g_color * brratiofp);
                    if (g_color < 1)
                    {
                        g_color = 1;
                    }
                    if (g_color > 127)
                    {
                        g_color = 127;
                    }
                    (*plot)(col, row, 127 + bbase - g_color);
                }
            }
            else
            {
                g_color = (int)(254l * zpixel / zdots);
                (*plot)(col, row, g_color + 1);
            }
            plotted = 1;
            break;
        }
        mxfp += dmxfp;
        myfp += dmyfp;
        jxfp += djxfp;
        jyfp += djyfp;
    }
    return (0);
}

int
Std4dFractal()
{
    long x;
    g_c_exponent = (int)param[2];
    if (neworbittype == fractal_type::LJULIAZPOWER)
    {
        if (g_c_exponent < 1)
        {
            g_c_exponent = 1;
        }
        if (param[3] == 0.0 && g_debug_flag != debug_flags::force_complex_power && (double)g_c_exponent == param[2])
        {
            fractalspecific[static_cast<int>(neworbittype)].orbitcalc = longZpowerFractal;
        }
        else
        {
            fractalspecific[static_cast<int>(neworbittype)].orbitcalc = longCmplxZpowerFractal;
        }
    }

    long y = 0;
    for (int ydot = (ydots >> 1) - 1; ydot >= 0; ydot--, y -= inch_per_ydot)
    {
        plotted = 0;
        x = -(width >> 1);
        for (int xdot = 0; xdot < xdots; xdot++, x += inch_per_xdot)
        {
            col = xdot;
            row = ydot;
            if (zline(x, y) < 0)
            {
                return (-1);
            }
            col = xdots - col - 1;
            row = ydots - row - 1;
            if (zline(-x, -y) < 0)
            {
                return (-1);
            }
        }
        if (plotted == 0)
        {
            if (y == 0)
            {
                plotted = -1;  // no points first pass; don't give up
            }
            else
            {
                break;
            }
        }
    }
    return (0);
}
int
Std4dfpFractal()
{
    double x;
    g_c_exponent = (int)param[2];

    if (neworbittype == fractal_type::FPJULIAZPOWER)
    {
        if (param[3] == 0.0 && g_debug_flag != debug_flags::force_complex_power && (double)g_c_exponent == param[2])
        {
            fractalspecific[static_cast<int>(neworbittype)].orbitcalc = floatZpowerFractal;
        }
        else
        {
            fractalspecific[static_cast<int>(neworbittype)].orbitcalc = floatCmplxZpowerFractal;
        }
        get_julia_attractor(param[0], param[1]);  // another attractor?
    }

    double y = 0.0;
    for (int ydot = (ydots >> 1) - 1; ydot >= 0; ydot--, y -= inch_per_ydotfp)
    {
        plotted = 0;
        x = -widthfp / 2;
        for (int xdot = 0; xdot < xdots; xdot++, x += inch_per_xdotfp)
        {
            col = xdot;
            row = ydot;
            if (zlinefp(x, y) < 0)
            {
                return (-1);
            }
            col = xdots - col - 1;
            row = ydots - row - 1;
            if (zlinefp(-x, -y) < 0)
            {
                return (-1);
            }
        }
        if (plotted == 0)
        {
            if (y == 0)
            {
                plotted = -1;  // no points first pass; don't give up
            }
            else
            {
                break;
            }
        }
    }
    return (0);
}
