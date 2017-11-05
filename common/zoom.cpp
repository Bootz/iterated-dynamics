/*
    zoom.c - routines for zoombox manipulation and for panning

*/
#include <cassert>
#include <vector>

#include <float.h>
#include <string.h>

#include "port.h"
#include "prototyp.h"
#include "drivers.h"

#define PIXELROUND 0.00001

int g_box_x[NUM_BOX_POINTS] = { 0 };
int g_box_y[NUM_BOX_POINTS] = { 0 };
int g_box_values[NUM_BOX_POINTS] = { 0 };
bool g_video_scroll = false;

static void zmo_calc(double, double, double *, double *, double);
static void zmo_calcbf(bf_t, bf_t, bf_t, bf_t, bf_t, bf_t, bf_t, bf_t, bf_t);
static int  check_pan();
static void fix_worklist();
static void move_row(int fromrow, int torow, int col);

// big number declarations
void calc_corner(bf_t target, bf_t p1, double p2, bf_t p3, double p4, bf_t p5)
{
    bf_t btmp1, btmp2 , btmp3;
    int saved;
    saved = save_stack();
    btmp1 = alloc_stack(rbflength+2);
    btmp2 = alloc_stack(rbflength+2);
    btmp3 = alloc_stack(rbflength+2);

    // use target as temporary variable
    floattobf(btmp3, p2);
    mult_bf(btmp1, btmp3, p3);
    mult_bf(btmp2, floattobf(target, p4), p5);
    add_bf(target, btmp1, btmp2);
    add_a_bf(target, p1);
    restore_stack(saved);
}

// Zoom-Box color
int g_box_color = 0;

#ifndef XFRACT
void dispbox()
{
    int boxc = (g_colors-1)&g_box_color;
    int rgb[3];
    for (int i = 0; i < g_box_count; i++)
    {
        if (g_is_true_color && g_true_mode != true_color_mode::default_color)
        {
            driver_get_truecolor(g_box_x[i]-sxoffs, g_box_y[i]-syoffs, &rgb[0], &rgb[1], &rgb[2], nullptr);
            driver_put_truecolor(g_box_x[i]-sxoffs, g_box_y[i]-syoffs,
                                 rgb[0]^255, rgb[1]^255, rgb[2]^255, 255);
        }
        else
        {
            g_box_values[i] = getcolor(g_box_x[i]-sxoffs, g_box_y[i]-syoffs);
        }
    }
    // There is an interaction between getcolor and putcolor, so separate them
    if (!(g_is_true_color && g_true_mode != true_color_mode::default_color)) // don't need this for truecolor with truemode set
        for (int i = 0; i < g_box_count; i++)
        {
            if (g_colors == 2)
            {
                g_put_color(g_box_x[i]-sxoffs, g_box_y[i]-syoffs, (1 - g_box_values[i]));
            }
            else
            {
                g_put_color(g_box_x[i]-sxoffs, g_box_y[i]-syoffs, boxc);
            }
        }
}

void clearbox()
{
    if (g_is_true_color && g_true_mode != true_color_mode::default_color)
    {
        dispbox();
    }
    else
    {
        for (int i = 0; i < g_box_count; i++)
        {
            g_put_color(g_box_x[i]-sxoffs, g_box_y[i]-syoffs, g_box_values[i]);
        }
    }
}
#endif

void drawbox(bool drawit)
{
    coords tl, bl, tr, br; // dot addr of topleft, botleft, etc
    double tmpx, tmpy, dx, dy, rotcos, rotsin, ftemp1, ftemp2;
    double fxwidth, fxskew, fydepth, fyskew, fxadj;
    bf_t bffxwidth, bffxskew, bffydepth, bffyskew, bffxadj;
    int saved = 0;
    if (zoom_box_width == 0)
    {
        // no box to draw
        if (g_box_count != 0)
        {
            // remove the old box from display
            clearbox();
            g_box_count = 0;
        }
        reset_zoom_corners();
        return;
    }
    if (bf_math != bf_math_type::NONE)
    {
        saved = save_stack();
        bffxwidth = alloc_stack(rbflength+2);
        bffxskew  = alloc_stack(rbflength+2);
        bffydepth = alloc_stack(rbflength+2);
        bffyskew  = alloc_stack(rbflength+2);
        bffxadj   = alloc_stack(rbflength+2);
    }
    ftemp1 = PI*zoom_box_rotation/72; // convert to radians
    rotcos = cos(ftemp1);   // sin & cos of rotation
    rotsin = sin(ftemp1);

    // do some calcs just once here to reduce fp work a bit
    fxwidth = g_save_x_max-g_save_x_3rd;
    fxskew  = g_save_x_3rd-g_save_x_min;
    fydepth = g_save_y_3rd-g_save_y_max;
    fyskew  = g_save_y_min-g_save_y_3rd;
    fxadj   = zoom_box_width*zoom_box_skew;

    if (bf_math != bf_math_type::NONE)
    {
        // do some calcs just once here to reduce fp work a bit
        sub_bf(bffxwidth, bfsxmax, bfsx3rd);
        sub_bf(bffxskew, bfsx3rd, bfsxmin);
        sub_bf(bffydepth, bfsy3rd, bfsymax);
        sub_bf(bffyskew, bfsymin, bfsy3rd);
        floattobf(bffxadj, fxadj);
    }

    // calc co-ords of topleft & botright corners of box
    tmpx = zoom_box_width/-2+fxadj; // from zoombox center as origin, on xdots scale
    tmpy = zoom_box_height*g_final_aspect_ratio/2;
    dx = (rotcos*tmpx - rotsin*tmpy) - tmpx; // delta x to rotate topleft
    dy = tmpy - (rotsin*tmpx + rotcos*tmpy); // delta y to rotate topleft

    // calc co-ords of topleft
    ftemp1 = zbx + dx + fxadj;
    ftemp2 = zby + dy/g_final_aspect_ratio;

    tl.x   = (int)(ftemp1*(g_x_size_dots+PIXELROUND)); // screen co-ords
    tl.y   = (int)(ftemp2*(g_y_size_dots+PIXELROUND));
    xxmin  = g_save_x_min + ftemp1*fxwidth + ftemp2*fxskew; // real co-ords
    yymax  = g_save_y_max + ftemp2*fydepth + ftemp1*fyskew;
    if (bf_math != bf_math_type::NONE)
    {
        calc_corner(bfxmin, bfsxmin, ftemp1, bffxwidth, ftemp2, bffxskew);
        calc_corner(bfymax, bfsymax, ftemp2, bffydepth, ftemp1, bffyskew);
    }

    // calc co-ords of bottom right
    ftemp1 = zbx + zoom_box_width - dx - fxadj;
    ftemp2 = zby - dy/g_final_aspect_ratio + zoom_box_height;
    br.x   = (int)(ftemp1*(g_x_size_dots+PIXELROUND));
    br.y   = (int)(ftemp2*(g_y_size_dots+PIXELROUND));
    xxmax  = g_save_x_min + ftemp1*fxwidth + ftemp2*fxskew;
    yymin  = g_save_y_max + ftemp2*fydepth + ftemp1*fyskew;
    if (bf_math != bf_math_type::NONE)
    {
        calc_corner(bfxmax, bfsxmin, ftemp1, bffxwidth, ftemp2, bffxskew);
        calc_corner(bfymin, bfsymax, ftemp2, bffydepth, ftemp1, bffyskew);
    }
    // do the same for botleft & topright
    tmpx = zoom_box_width/-2 - fxadj;
    tmpy = 0.0-tmpy;
    dx = (rotcos*tmpx - rotsin*tmpy) - tmpx;
    dy = tmpy - (rotsin*tmpx + rotcos*tmpy);
    ftemp1 = zbx + dx - fxadj;
    ftemp2 = zby + dy/g_final_aspect_ratio + zoom_box_height;
    bl.x   = (int)(ftemp1*(g_x_size_dots+PIXELROUND));
    bl.y   = (int)(ftemp2*(g_y_size_dots+PIXELROUND));
    xx3rd  = g_save_x_min + ftemp1*fxwidth + ftemp2*fxskew;
    yy3rd  = g_save_y_max + ftemp2*fydepth + ftemp1*fyskew;
    if (bf_math != bf_math_type::NONE)
    {
        calc_corner(bfx3rd, bfsxmin, ftemp1, bffxwidth, ftemp2, bffxskew);
        calc_corner(bfy3rd, bfsymax, ftemp2, bffydepth, ftemp1, bffyskew);
        restore_stack(saved);
    }
    ftemp1 = zbx + zoom_box_width - dx + fxadj;
    ftemp2 = zby - dy/g_final_aspect_ratio;
    tr.x   = (int)(ftemp1*(g_x_size_dots+PIXELROUND));
    tr.y   = (int)(ftemp2*(g_y_size_dots+PIXELROUND));

    if (g_box_count != 0)
    {
        // remove the old box from display
        clearbox();
        g_box_count = 0;
    }

    if (drawit)
    {
        // caller wants box drawn as well as co-ords calc'd
#ifndef XFRACT
        // build the list of zoom box pixels
        addbox(tl);
        addbox(tr);               // corner pixels
        addbox(bl);
        addbox(br);
        drawlines(tl, tr, bl.x-tl.x, bl.y-tl.y); // top & bottom lines
        drawlines(tl, bl, tr.x-tl.x, tr.y-tl.y); // left & right lines
#else
        g_box_x[0] = tl.x + sxoffs;
        g_box_y[0] = tl.y + syoffs;
        g_box_x[1] = tr.x + sxoffs;
        g_box_y[1] = tr.y + syoffs;
        g_box_x[2] = br.x + sxoffs;
        g_box_y[2] = br.y + syoffs;
        g_box_x[3] = bl.x + sxoffs;
        g_box_y[3] = bl.y + syoffs;
        g_box_count = 4;
#endif
        dispbox();
    }
}

void drawlines(coords fr, coords to, int dx, int dy)
{
    if (abs(to.x-fr.x) > abs(to.y-fr.y))
    {
        // delta.x > delta.y
        if (fr.x > to.x)
        {
            // swap so from.x is < to.x
            coords const tmpp = fr;
            fr = to;
            to = tmpp;
        }
        int const xincr = (to.x-fr.x)*4/g_screen_x_dots+1; // do every 1st, 2nd, 3rd, or 4th dot
        int ctr = (to.x-fr.x-1)/xincr;
        int const altdec = abs(to.y-fr.y)*xincr;
        int const altinc = to.x-fr.x;
        int altctr = altinc/2;
        int const yincr = (to.y > fr.y)?1:-1;
        coords line1;
        line1.y = fr.y;
        line1.x = fr.x;
        coords line2;
        line2.x = line1.x + dx;
        line2.y = line1.y + dy;
        while (--ctr >= 0)
        {
            line1.x += xincr;
            line2.x += xincr;
            altctr -= altdec;
            while (altctr < 0)
            {
                altctr  += altinc;
                line1.y += yincr;
                line2.y += yincr;
            }
            addbox(line1);
            addbox(line2);
        }
    }
    else
    {
        // delta.y > delta.x
        if (fr.y > to.y)
        {
            // swap so from.y is < to.y
            coords const tmpp = fr;
            fr = to;
            to = tmpp;
        }
        int const yincr = (to.y-fr.y)*4/g_screen_y_dots+1; // do every 1st, 2nd, 3rd, or 4th dot
        int ctr = (to.y-fr.y-1)/yincr;
        int const altdec = abs(to.x-fr.x)*yincr;
        int const altinc = to.y-fr.y;
        int altctr = altinc/2;
        int const xincr = (to.x > fr.x) ? 1 : -1;
        coords line1;
        line1.x = fr.x;
        line1.y = fr.y;
        coords line2;
        line2.x = line1.x + dx;
        line2.y = line1.y + dy;
        while (--ctr >= 0)
        {
            line1.y += yincr;
            line2.y += yincr;
            altctr  -= altdec;
            while (altctr < 0)
            {
                altctr  += altinc;
                line1.x += xincr;
                line2.x += xincr;
            }
            addbox(line1);
            addbox(line2);
        }
    }
}

void addbox(coords point)
{
    assert(g_box_count < NUM_BOX_POINTS);
    point.x += sxoffs;
    point.y += syoffs;
    if (point.x >= 0 && point.x < g_screen_x_dots &&
            point.y >= 0 && point.y < g_screen_y_dots)
    {
        g_box_x[g_box_count] = point.x;
        g_box_y[g_box_count] = point.y;
        ++g_box_count;
    }
}

void moveboxf(double dx, double dy)
{
    int align;
    align = check_pan();
    if (dx != 0.0)
    {
        if ((zbx += dx) + zoom_box_width/2 < 0)    // center must stay onscreen
        {
            zbx = zoom_box_width/-2;
        }
        if (zbx + zoom_box_width/2 > 1)
        {
            zbx = 1.0 - zoom_box_width/2;
        }
        int col;
        if (align != 0
                && ((col = (int)(zbx*(g_x_size_dots+PIXELROUND))) & (align-1)) != 0)
        {
            if (dx > 0)
            {
                col += align;
            }
            col -= col & (align-1); // adjust col to pass alignment
            zbx = (double)col/g_x_size_dots;
        }
    }
    if (dy != 0.0)
    {
        if ((zby += dy) + zoom_box_height/2 < 0)
        {
            zby = zoom_box_height/-2;
        }
        if (zby + zoom_box_height/2 > 1)
        {
            zby = 1.0 - zoom_box_height/2;
        }
        int row;
        if (align != 0
                && ((row = (int)(zby*(g_y_size_dots+PIXELROUND))) & (align-1)) != 0)
        {
            if (dy > 0)
            {
                row += align;
            }
            row -= row & (align-1);
            zby = (double)row/g_y_size_dots;
        }
    }
#ifndef XFRACT
    if (g_video_scroll)                 // scroll screen center to the box center
    {
        int col = (int)((zbx + zoom_box_width/2)*(g_x_size_dots + PIXELROUND)) + sxoffs;
        int row = (int)((zby + zoom_box_height/2)*(g_y_size_dots + PIXELROUND)) + syoffs;
        if (!zscroll)
        {
            // fixed - screen center fixed to the zoombox center
            scroll_center(col, row);
        }
        else
        {
            // relaxed - as the zoombox center leaves the screen
            if ((col -= g_video_start_x) > 0 && (col -= g_vesa_x_res - 1) < 0)
            {
                col = 0;
            }
            if ((row -= g_video_start_y) > 0 && (row -= g_vesa_y_res - 1) < 0)
            {
                row = 0;
            }
            if (col != 0 || row != 0)
            {
                scroll_relative(col, row);
            }
        }
    }
#endif
}

static void chgboxf(double dwidth, double ddepth)
{
    if (zoom_box_width+dwidth > 1)
    {
        dwidth = 1.0-zoom_box_width;
    }
    if (zoom_box_width+dwidth < 0.05)
    {
        dwidth = 0.05-zoom_box_width;
    }
    zoom_box_width += dwidth;
    if (zoom_box_height+ddepth > 1)
    {
        ddepth = 1.0-zoom_box_height;
    }
    if (zoom_box_height+ddepth < 0.05)
    {
        ddepth = 0.05-zoom_box_height;
    }
    zoom_box_height += ddepth;
    moveboxf(dwidth/-2, ddepth/-2); // keep it centered & check limits
}

void resizebox(int steps)
{
    double deltax, deltay;
    if (zoom_box_height*g_screen_aspect > zoom_box_width)
    {
        // box larger on y axis
        deltay = steps * 0.036 / g_screen_aspect;
        deltax = zoom_box_width * deltay / zoom_box_height;
    }
    else
    {
        // box larger on x axis
        deltax = steps * 0.036;
        deltay = zoom_box_height * deltax / zoom_box_width;
    }
    chgboxf(deltax, deltay);
}

void chgboxi(int dw, int dd)
{
    // change size by pixels
    chgboxf((double)dw/g_x_size_dots, (double)dd/g_y_size_dots);
}

extern void show_three_bf();

static void zmo_calcbf(bf_t bfdx, bf_t bfdy,
                                 bf_t bfnewx, bf_t bfnewy, bf_t bfplotmx1, bf_t bfplotmx2, bf_t bfplotmy1,
                                 bf_t bfplotmy2, bf_t bfftemp)
{
    bf_t btmp1, btmp2, btmp3, btmp4, btempx, btempy ;
    bf_t btmp2a, btmp4a;
    int saved;
    saved = save_stack();

    btmp1  = alloc_stack(rbflength+2);
    btmp2  = alloc_stack(rbflength+2);
    btmp3  = alloc_stack(rbflength+2);
    btmp4  = alloc_stack(rbflength+2);
    btmp2a = alloc_stack(rbflength+2);
    btmp4a = alloc_stack(rbflength+2);
    btempx = alloc_stack(rbflength+2);
    btempy = alloc_stack(rbflength+2);

    /* calc cur screen corner relative to zoombox, when zoombox co-ords
       are taken as (0,0) topleft thru (1,1) bottom right */

    // tempx = dy * plotmx1 - dx * plotmx2;
    mult_bf(btmp1, bfdy, bfplotmx1);
    mult_bf(btmp2, bfdx, bfplotmx2);
    sub_bf(btempx, btmp1, btmp2);

    // tempy = dx * plotmy1 - dy * plotmy2;
    mult_bf(btmp1, bfdx, bfplotmy1);
    mult_bf(btmp2, bfdy, bfplotmy2);
    sub_bf(btempy, btmp1, btmp2);

    // calc new corner by extending from current screen corners
    // *newx = sxmin + tempx*(sxmax-sx3rd)/ftemp + tempy*(sx3rd-sxmin)/ftemp;
    sub_bf(btmp1, bfsxmax, bfsx3rd);
    mult_bf(btmp2, btempx, btmp1);
    // show_three_bf("fact1",btempx,"fact2",btmp1,"prod ",btmp2,70);
    div_bf(btmp2a, btmp2, bfftemp);
    // show_three_bf("num  ",btmp2,"denom",bfftemp,"quot ",btmp2a,70);
    sub_bf(btmp3, bfsx3rd, bfsxmin);
    mult_bf(btmp4, btempy, btmp3);
    div_bf(btmp4a, btmp4, bfftemp);
    add_bf(bfnewx, bfsxmin, btmp2a);
    add_a_bf(bfnewx, btmp4a);

    // *newy = symax + tempy*(sy3rd-symax)/ftemp + tempx*(symin-sy3rd)/ftemp;
    sub_bf(btmp1, bfsy3rd, bfsymax);
    mult_bf(btmp2, btempy, btmp1);
    div_bf(btmp2a, btmp2, bfftemp);
    sub_bf(btmp3, bfsymin, bfsy3rd);
    mult_bf(btmp4, btempx, btmp3);
    div_bf(btmp4a, btmp4, bfftemp);
    add_bf(bfnewy, bfsymax, btmp2a);
    add_a_bf(bfnewy, btmp4a);
    restore_stack(saved);
}

static void zmo_calc(double dx, double dy, double *newx, double *newy, double ftemp)
{
    double tempx, tempy;
    /* calc cur screen corner relative to zoombox, when zoombox co-ords
       are taken as (0,0) topleft thru (1,1) bottom right */
    tempx = dy * g_plot_mx1 - dx * g_plot_mx2;
    tempy = dx * g_plot_my1 - dy * g_plot_my2;

    // calc new corner by extending from current screen corners
    *newx = g_save_x_min + tempx*(g_save_x_max-g_save_x_3rd)/ftemp + tempy*(g_save_x_3rd-g_save_x_min)/ftemp;
    *newy = g_save_y_max + tempy*(g_save_y_3rd-g_save_y_max)/ftemp + tempx*(g_save_y_min-g_save_y_3rd)/ftemp;
}

void zoomoutbf() // for ctl-enter, calc corners for zooming out
{
    /* (xxmin,yymax), etc, are already set to zoombox corners;
       (sxmin,symax), etc, are still the screen's corners;
       use the same logic as plot_orbit stuff to first calculate current screen
       corners relative to the zoombox, as if the zoombox were a square with
       upper left (0,0) and width/depth 1; ie calc the current screen corners
       as if plotting them from the zoombox;
       then extend these co-ords from current real screen corners to get
       new actual corners
       */
    bf_t savbfxmin, savbfymax, bfftemp;
    bf_t tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, bfplotmx1, bfplotmx2, bfplotmy1, bfplotmy2;
    int saved;
    saved = save_stack();
    savbfxmin = alloc_stack(rbflength+2);
    savbfymax = alloc_stack(rbflength+2);
    bfftemp   = alloc_stack(rbflength+2);
    tmp1      = alloc_stack(rbflength+2);
    tmp2      = alloc_stack(rbflength+2);
    tmp3      = alloc_stack(rbflength+2);
    tmp4      = alloc_stack(rbflength+2);
    tmp5      = alloc_stack(rbflength+2);
    tmp6      = alloc_stack(rbflength+2);
    bfplotmx1 = alloc_stack(rbflength+2);
    bfplotmx2 = alloc_stack(rbflength+2);
    bfplotmy1 = alloc_stack(rbflength+2);
    bfplotmy2 = alloc_stack(rbflength+2);
    // ftemp = (yymin-yy3rd)*(xx3rd-xxmin) - (xxmax-xx3rd)*(yy3rd-yymax);
    sub_bf(tmp1, bfymin, bfy3rd);
    sub_bf(tmp2, bfx3rd, bfxmin);
    sub_bf(tmp3, bfxmax, bfx3rd);
    sub_bf(tmp4, bfy3rd, bfymax);
    mult_bf(tmp5, tmp1, tmp2);
    mult_bf(tmp6, tmp3, tmp4);
    sub_bf(bfftemp, tmp5, tmp6);
    // plotmx1 = (xx3rd-xxmin); */ ; /* reuse the plotxxx vars is safe
    copy_bf(bfplotmx1, tmp2);
    // plotmx2 = (yy3rd-yymax);
    copy_bf(bfplotmx2, tmp4);
    // plotmy1 = (yymin-yy3rd);
    copy_bf(bfplotmy1, tmp1);
    /* plotmy2 = (xxmax-xx3rd); */;
    copy_bf(bfplotmy2, tmp3);

    // savxxmin = xxmin; savyymax = yymax;
    copy_bf(savbfxmin, bfxmin);
    copy_bf(savbfymax, bfymax);

    sub_bf(tmp1, bfsxmin, savbfxmin);
    sub_bf(tmp2, bfsymax, savbfymax);
    zmo_calcbf(tmp1, tmp2, bfxmin, bfymax, bfplotmx1, bfplotmx2, bfplotmy1,
               bfplotmy2, bfftemp);
    sub_bf(tmp1, bfsxmax, savbfxmin);
    sub_bf(tmp2, bfsymin, savbfymax);
    zmo_calcbf(tmp1, tmp2, bfxmax, bfymin, bfplotmx1, bfplotmx2, bfplotmy1,
               bfplotmy2, bfftemp);
    sub_bf(tmp1, bfsx3rd, savbfxmin);
    sub_bf(tmp2, bfsy3rd, savbfymax);
    zmo_calcbf(tmp1, tmp2, bfx3rd, bfy3rd, bfplotmx1, bfplotmx2, bfplotmy1,
               bfplotmy2, bfftemp);
    restore_stack(saved);
}

void zoomoutdbl() // for ctl-enter, calc corners for zooming out
{
    /* (xxmin,yymax), etc, are already set to zoombox corners;
       (sxmin,symax), etc, are still the screen's corners;
       use the same logic as plot_orbit stuff to first calculate current screen
       corners relative to the zoombox, as if the zoombox were a square with
       upper left (0,0) and width/depth 1; ie calc the current screen corners
       as if plotting them from the zoombox;
       then extend these co-ords from current real screen corners to get
       new actual corners
       */
    double savxxmin, savyymax, ftemp;
    ftemp = (yymin-yy3rd)*(xx3rd-xxmin) - (xxmax-xx3rd)*(yy3rd-yymax);
    g_plot_mx1 = (xx3rd-xxmin); // reuse the plotxxx vars is safe
    g_plot_mx2 = (yy3rd-yymax);
    g_plot_my1 = (yymin-yy3rd);
    g_plot_my2 = (xxmax-xx3rd);
    savxxmin = xxmin;
    savyymax = yymax;
    zmo_calc(g_save_x_min-savxxmin, g_save_y_max-savyymax, &xxmin, &yymax, ftemp);
    zmo_calc(g_save_x_max-savxxmin, g_save_y_min-savyymax, &xxmax, &yymin, ftemp);
    zmo_calc(g_save_x_3rd-savxxmin, g_save_y_3rd-savyymax, &xx3rd, &yy3rd, ftemp);
}

void zoomout() // for ctl-enter, calc corners for zooming out
{
    if (bf_math != bf_math_type::NONE)
    {
        zoomoutbf();
    }
    else
    {
        zoomoutdbl();
    }
}

void aspectratio_crop(float oldaspect, float newaspect)
{
    double ftemp, xmargin, ymargin;
    if (newaspect > oldaspect)
    {
        // new ratio is taller, crop x
        ftemp = (1.0 - oldaspect / newaspect) / 2;
        xmargin = (xxmax - xx3rd) * ftemp;
        ymargin = (yymin - yy3rd) * ftemp;
        xx3rd += xmargin;
        yy3rd += ymargin;
    }
    else
    {
        // new ratio is wider, crop y
        ftemp = (1.0 - newaspect / oldaspect) / 2;
        xmargin = (xx3rd - xxmin) * ftemp;
        ymargin = (yy3rd - yymax) * ftemp;
        xx3rd -= xmargin;
        yy3rd -= ymargin;
    }
    xxmin += xmargin;
    yymax += ymargin;
    xxmax -= xmargin;
    yymin -= ymargin;
}

static int check_pan() // return 0 if can't, alignment requirement if can
{
    if ((g_calc_status != calc_status_value::RESUMABLE && g_calc_status != calc_status_value::COMPLETED) || g_evolving)
    {
        return (0); // not resumable, not complete
    }
    if (curfractalspecific->calctype != standard_fractal
            && curfractalspecific->calctype != calcmand
            && curfractalspecific->calctype != calcmandfp
            && curfractalspecific->calctype != lyapunov
            && curfractalspecific->calctype != calcfroth)
    {
        return (0); // not a worklist-driven type
    }
    if (zoom_box_width != 1.0 || zoom_box_height != 1.0
            || zoom_box_skew != 0.0 || zoom_box_rotation != 0.0)
    {
        return (0); // not a full size unrotated unskewed zoombox
    }
    if (g_std_calc_mode == 't')
    {
        return (0); // tesselate, can't do it
    }
    if (g_std_calc_mode == 'd')
    {
        return (0); // diffusion scan: can't do it either
    }
    if (g_std_calc_mode == 'o')
    {
        return (0); // orbits, can't do it
    }

    // can pan if we get this far

    if (g_calc_status == calc_status_value::COMPLETED)
    {
        return (1); // image completed, align on any pixel
    }
    if (g_potential_flag && g_potential_16bit)
    {
        return (1); // 1 pass forced so align on any pixel
    }
    if (g_std_calc_mode == 'b')
    {
        return (1); // btm, align on any pixel
    }
    if (g_std_calc_mode != 'g' || (curfractalspecific->flags&NOGUESS))
    {
        if (g_std_calc_mode == '2' || g_std_calc_mode == '3')   // align on even pixel for 2pass
        {
            return (2);
        }
        return (1); // assume 1pass
    }
    // solid guessing
    start_resume();
    get_resume(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    // don't do end_resume! we're just looking
    int i = 9;
    for (int j = 0; j < g_num_work_list; ++j)   // find lowest pass in any pending window
    {
        if (g_work_list[j].pass < i)
        {
            i = g_work_list[j].pass;
        }
    }
    int j = ssg_blocksize(); // worst-case alignment requirement
    while (--i >= 0)
    {
        j = j >> 1; // reduce requirement
    }
    return (j);
}

// move a row on the screen
static void move_row(int fromrow, int torow, int col)
{
    std::vector<BYTE> temp(xdots, 0);
    if (fromrow >= 0 && fromrow < ydots)
    {
        int startcol = 0;
        int tocol = 0;
        int endcol = xdots-1;
        if (col < 0)
        {
            tocol -= col;
            endcol += col;
        }
        if (col > 0)
        {
            startcol += col;
        }
        get_line(fromrow, startcol, endcol, &temp[tocol]);
    }
    put_line(torow, 0, xdots-1, &temp[0]);
}

int init_pan_or_recalc(int do_zoomout) // decide to recalc, or to chg worklist & pan
{
    int row;
    int col;
    int alignmask;
    int listfull;
    if (zoom_box_width == 0.0)
    {
        return (0); // no zoombox, leave g_calc_status as is
    }
    // got a zoombox
    alignmask = check_pan()-1;
    if (alignmask < 0 || g_evolving)
    {
        g_calc_status = calc_status_value::PARAMS_CHANGED; // can't pan, trigger recalc
        return (0);
    }
    if (zbx == 0.0 && zby == 0.0)
    {
        clearbox();
        return (0);
    } // box is full screen, leave g_calc_status as is
    col = (int)(zbx*(g_x_size_dots+PIXELROUND)); // calc dest col,row of topleft pixel
    row = (int)(zby*(g_y_size_dots+PIXELROUND));
    if (do_zoomout)
    {
        // invert row and col
        row = 0-row;
        col = 0-col;
    }
    if ((row&alignmask) != 0 || (col&alignmask) != 0)
    {
        g_calc_status = calc_status_value::PARAMS_CHANGED; // not on useable pixel alignment, trigger recalc
        return (0);
    }
    // pan
    g_num_work_list = 0;
    if (g_calc_status == calc_status_value::RESUMABLE)
    {
        start_resume();
        get_resume(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    } // don't do end_resume! we might still change our mind
    // adjust existing worklist entries
    for (int i = 0; i < g_num_work_list; ++i)
    {
        g_work_list[i].yystart -= row;
        g_work_list[i].yystop  -= row;
        g_work_list[i].yybegin -= row;
        g_work_list[i].xxstart -= col;
        g_work_list[i].xxstop  -= col;
        g_work_list[i].xxbegin -= col;
    }
    // add worklist entries for the new edges
    listfull = 0;
    int i = 0;
    int j = ydots-1;
    if (row < 0)
    {
        listfull |= add_worklist(0, xdots-1, 0, 0, 0-row-1, 0, 0, 0);
        i = 0 - row;
    }
    if (row > 0)
    {
        listfull |= add_worklist(0, xdots-1, 0, ydots-row, ydots-1, ydots-row, 0, 0);
        j = ydots - row - 1;
    }
    if (col < 0)
    {
        listfull |= add_worklist(0, 0-col-1, 0, i, j, i, 0, 0);
    }
    if (col > 0)
    {
        listfull |= add_worklist(xdots-col, xdots-1, xdots-col, i, j, i, 0, 0);
    }
    if (listfull != 0)
    {
        if (stopmsg(STOPMSG_CANCEL,
                    "Tables full, can't pan current image.\n"
                    "Cancel resumes old image, continue pans and calculates a new one."))
        {
            zoom_box_width = 0; // cancel the zoombox
            drawbox(true);
        }
        else
        {
            g_calc_status = calc_status_value::PARAMS_CHANGED; // trigger recalc
        }
        return (0);
    }
    // now we're committed
    g_calc_status = calc_status_value::RESUMABLE;
    clearbox();
    if (row > 0)   // move image up
    {
        for (int y = 0; y < ydots; ++y)
        {
            move_row(y+row, y, col);
        }
    }
    else             // move image down
    {
        for (int y = ydots; --y >=0;)
        {
            move_row(y+row, y, col);
        }
    }
    fix_worklist(); // fixup any out of bounds worklist entries
    alloc_resume(sizeof(g_work_list)+20, 2); // post the new worklist
    put_resume(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    return (0);
}

static void restart_window(int wknum)
// force a worklist entry to restart
{
    int yfrom = g_work_list[wknum].yystart;
    if (yfrom < 0)
    {
        yfrom = 0;
    }
    int xfrom = g_work_list[wknum].xxstart;
    if (xfrom < 0)
    {
        xfrom = 0;
    }
    int yto = g_work_list[wknum].yystop;
    if (yto >= ydots)
    {
        yto = ydots - 1;
    }
    int xto = g_work_list[wknum].xxstop;
    if (xto >= xdots)
    {
        xto = xdots - 1;
    }
    std::vector<BYTE> temp(xdots, 0);
    while (yfrom <= yto)
    {
        put_line(yfrom++, xfrom, xto, &temp[0]);
    }
    g_work_list[wknum].pass = 0;
    g_work_list[wknum].sym = g_work_list[wknum].pass;
    g_work_list[wknum].yybegin = g_work_list[wknum].yystart;
    g_work_list[wknum].xxbegin = g_work_list[wknum].xxstart;
}

static void fix_worklist() // fix out of bounds and symmetry related stuff
{
    for (int i = 0; i < g_num_work_list; ++i)
    {
        WORKLIST *wk = &g_work_list[i];
        if (wk->yystart >= ydots || wk->yystop < 0
                || wk->xxstart >= xdots || wk->xxstop < 0)
        {
            // offscreen, delete
            for (int j = i+1; j < g_num_work_list; ++j)
            {
                g_work_list[j-1] = g_work_list[j];
            }
            --g_num_work_list;
            --i;
            continue;
        }
        if (wk->yystart < 0)
        {
            // partly off top edge
            if ((wk->sym&1) == 0)
            {
                // no sym, easy
                wk->yystart = 0;
                wk->xxbegin = 0;
            }
            else
            {
                // xaxis symmetry
                int j = wk->yystop + wk->yystart;
                if (j > 0
                        && g_num_work_list < MAXCALCWORK)
                {
                    // split the sym part
                    g_work_list[g_num_work_list] = g_work_list[i];
                    g_work_list[g_num_work_list].yystart = 0;
                    g_work_list[g_num_work_list++].yystop = j;
                    wk->yystart = j+1;
                }
                else
                {
                    wk->yystart = 0;
                }
                restart_window(i); // restart the no-longer sym part
            }
        }
        if (wk->yystop >= ydots)
        {
            // partly off bottom edge
            int j = ydots-1;
            if ((wk->sym&1) != 0)
            {
                // uses xaxis symmetry
                int k = wk->yystart + (wk->yystop - j);
                if (k < j)
                {
                    if (g_num_work_list >= MAXCALCWORK)   // no room to split
                    {
                        restart_window(i);
                    }
                    else
                    {
                        // split it
                        g_work_list[g_num_work_list] = g_work_list[i];
                        g_work_list[g_num_work_list].yystart = k;
                        g_work_list[g_num_work_list++].yystop = j;
                        j = k-1;
                    }
                }
                wk->sym &= -1 - 1;
            }
            wk->yystop = j;
        }
        if (wk->xxstart < 0)
        {
            // partly off left edge
            if ((wk->sym&2) == 0)   // no sym, easy
            {
                wk->xxstart = 0;
            }
            else
            {
                // yaxis symmetry
                int j = wk->xxstop + wk->xxstart;
                if (j > 0
                        && g_num_work_list < MAXCALCWORK)
                {
                    // split the sym part
                    g_work_list[g_num_work_list] = g_work_list[i];
                    g_work_list[g_num_work_list].xxstart = 0;
                    g_work_list[g_num_work_list++].xxstop = j;
                    wk->xxstart = j+1;
                }
                else
                {
                    wk->xxstart = 0;
                }
                restart_window(i); // restart the no-longer sym part
            }
        }
        if (wk->xxstop >= xdots)
        {
            // partly off right edge
            int j = xdots-1;
            if ((wk->sym&2) != 0)
            {
                // uses xaxis symmetry
                int k = wk->xxstart + (wk->xxstop - j);
                if (k < j)
                {
                    if (g_num_work_list >= MAXCALCWORK)   // no room to split
                    {
                        restart_window(i);
                    }
                    else
                    {
                        // split it
                        g_work_list[g_num_work_list] = g_work_list[i];
                        g_work_list[g_num_work_list].xxstart = k;
                        g_work_list[g_num_work_list++].xxstop = j;
                        j = k-1;
                    }
                }
                wk->sym &= -1 - 2;
            }
            wk->xxstop = j;
        }
        if (wk->yybegin < wk->yystart)
        {
            wk->yybegin = wk->yystart;
        }
        if (wk->yybegin > wk->yystop)
        {
            wk->yybegin = wk->yystop;
        }
        if (wk->xxbegin < wk->xxstart)
        {
            wk->xxbegin = wk->xxstart;
        }
        if (wk->xxbegin > wk->xxstop)
        {
            wk->xxbegin = wk->xxstop;
        }
    }
    tidy_worklist(); // combine where possible, re-sort
}
