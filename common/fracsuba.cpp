#include <float.h>
#include <stdlib.h>

#include "port.h"
#include "prototyp.h"

int asmlMODbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    g_l_magnitude = ltempsqrx + ltempsqry;
    if (g_l_magnitude >= g_l_limit || g_l_magnitude < 0 || labs(g_l_new.x) > g_l_limit2
            || labs(g_l_new.y) > g_l_limit2 || overflow)
    {
        overflow = false;
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asmlREALbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    if (ltempsqrx >= g_l_limit || overflow)
    {
        overflow = false;
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asmlIMAGbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    if (ltempsqry >= g_l_limit || overflow)
    {
        overflow = false;
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asmlORbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    if (ltempsqrx >= g_l_limit || ltempsqry >= g_l_limit || overflow)
    {
        overflow = false;
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asmlANDbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    if ((ltempsqrx >= g_l_limit && ltempsqry >= g_l_limit) || overflow)
    {
        overflow = false;
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asmlMANHbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    magnitude = fabs(g_new.x) + fabs(g_new.y);
    if (magnitude*magnitude >= rqlim)
    {
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asmlMANRbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    magnitude = fabs(g_new.x + g_new.y);
    if (magnitude*magnitude >= rqlim)
    {
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asm386lMODbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    g_l_magnitude = ltempsqrx + ltempsqry;
    if (g_l_magnitude >= g_l_limit || g_l_magnitude < 0 || labs(g_l_new.x) > g_l_limit2
            || labs(g_l_new.y) > g_l_limit2 || overflow)
    {
        overflow = false;
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asm386lREALbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    if (ltempsqrx >= g_l_limit || overflow)
    {
        overflow = false;
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asm386lIMAGbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    if (ltempsqry >= g_l_limit || overflow)
    {
        overflow = false;
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asm386lORbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    if (ltempsqrx >= g_l_limit || overflow)
    {
        overflow = false;
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asm386lANDbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    if ((ltempsqrx >= g_l_limit && ltempsqry >= g_l_limit) || overflow)
    {
        overflow = false;
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asm386lMANHbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    magnitude = fabs(g_new.x) + fabs(g_new.y);
    if (magnitude*magnitude >= rqlim)
    {
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

int asm386lMANRbailout()
{
    ltempsqrx = lsqr(g_l_new.x);
    ltempsqry = lsqr(g_l_new.y);
    magnitude = fabs(g_new.x + g_new.y);
    if (magnitude*magnitude >= rqlim)
    {
        return 1;
    }
    g_l_old = g_l_new;
    return 0;
}

/*
asmfpMODbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ; ny2
        fst     tempsqry
        fld     qword ptr new           ; nx ny2
        fmul    st,st                   ; nx2 ny2
        fst     tempsqrx
        fadd
        fst     magnitude
        fcomp   rqlim                   ; stack is empty
        fstsw   ax                      ; 287 and up only
        sahf
        jae     bailout
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpMODbailout endp
*/
int asmfpMODbailout()
{
    // TODO: verify this code is correct
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = tempsqrx + tempsqry;
    if (magnitude > rqlim || magnitude < 0.0 || fabs(g_new.x) > rqlim2 ||
            fabs(g_new.y) > rqlim2 || overflow)
    {
        overflow = false;
        return 1;
    }
    old = g_new;
    return 0;
}

/*
asmfpREALbailout proc near uses si di
        fld     qword ptr new
        fmul    st,st                   ; nx2
        fst     tempsqrx
        fld     qword ptr new+8 ; ny nx2
        fmul    st,st                   ; ny2 nx2
        fst     tempsqry                ; ny2 nx2
        fadd    st,st(1)                ; ny2+nx2 nx2
        fstp    magnitude               ; nx2
        fcomp   rqlim                   ; ** stack is empty
        fstsw   ax                      ; ** 287 and up only
        sahf
        jae     bailout
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpREALbailout endp
*/
int asmfpREALbailout()
{
    // TODO: verify this code is correct
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    if (tempsqrx >= rqlim || overflow)
    {
        overflow = false;
        return 1;
    }
    old = g_new;
    return 0;
}

/*
asmfpIMAGbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ; ny2
        fst     tempsqry
        fld     qword ptr new   ; nx ny2
        fmul    st,st                   ; nx2 ny2
        fst     tempsqrx                ; nx2 ny2
        fadd    st,st(1)                ; nx2+ny2 ny2
        fstp    magnitude               ; ny2
        fcomp   rqlim                   ; ** stack is empty
        fstsw   ax                      ; ** 287 and up only
        sahf
        jae     bailout
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpIMAGbailout endp
*/
int asmfpIMAGbailout()
{
    // TODO: verify this code is correct
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    if (tempsqry >= rqlim || overflow)
    {
        overflow = false;
        return 1;
    }
    old = g_new;
    return 0;
}

/*
asmfpORbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ; ny2
        fst     tempsqry
        fld     qword ptr new   ; nx ny2
        fmul    st,st                   ; nx2 ny2
        fst     tempsqrx
        fld     st(1)                   ; ny2 nx2 ny2
        fadd    st,st(1)                ; ny2+nx2 nx2 ny2
        fstp    magnitude               ; nx2 ny2
        fcomp   rqlim                   ; ny2
        fstsw   ax                      ; ** 287 and up only
        sahf
        jae     bailoutp
        fcomp   rqlim                   ; ** stack is empty
        fstsw   ax                      ; ** 287 and up only
        sahf
        jae     bailout
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailoutp:
        finit           ; cleans up stack
bailout:
        mov     ax,1
        ret
asmfpORbailout endp
*/
int asmfpORbailout()
{
    // TODO: verify this code is correct
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    if (tempsqrx >= rqlim || tempsqry >= rqlim || overflow)
    {
        overflow = false;
        return 1;
    }
    old = g_new;
    return 0;
}

/*
asmfpANDbailout proc near uses si di
        fld     qword ptr new+8
        fmul    st,st                   ; ny2
        fst     tempsqry
        fld     qword ptr new   ; nx ny2
        fmul    st,st                   ; nx2 ny2
        fst     tempsqrx
        fld     st(1)                   ; ny2 nx2 ny2
        fadd    st,st(1)                ; ny2+nx2 nx2 ny2
        fstp    magnitude               ; nx2 ny2
        fcomp   rqlim                   ; ny2
        fstsw   ax                      ; ** 287 and up only
        sahf
        jb      nobailoutp
        fcomp   rqlim                   ; ** stack is empty
        fstsw   ax                      ; ** 287 and up only
        sahf
        jae     bailout
        jmp     short nobailout
nobailoutp:
        finit           ; cleans up stack
nobailout:
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpANDbailout endp
*/
int asmfpANDbailout()
{
    // TODO: verify this code is correct
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    if ((tempsqrx >= rqlim && tempsqry >= rqlim) || overflow)
    {
        overflow = false;
        return 1;
    }
    old = g_new;
    return 0;
}

/*
asmfpMANHbailout proc near uses si di
        fld     qword ptr new+8
        fld     st
        fmul    st,st                   ; ny2 ny
        fst     tempsqry
        fld     qword ptr new   ; nx ny2 ny
        fld     st
        fmul    st,st                   ; nx2 nx ny2 ny
        fst     tempsqrx
        faddp   st(2),st                ; nx nx2+ny2 ny
        fxch    st(1)                   ; nx2+ny2 nx ny
        fstp    magnitude               ; nx ny
        fabs
        fxch
        fabs
        fadd                            ; |nx|+|ny|
        fmul    st,st                   ; (|nx|+|ny|)2
        fcomp   rqlim                   ; ** stack is empty
        fstsw   ax                      ; ** 287 and up only
        sahf
        jae     bailout
        jmp     short nobailout
nobailoutp:
        finit           ; cleans up stack
nobailout:
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpMANHbailout endp
*/
int asmfpMANHbailout()
{
    // TODO: verify this code is correct
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = fabs(g_new.x) + fabs(g_new.y);
    if (magnitude*magnitude >= rqlim)
    {
        return 1;
    }
    old = g_new;
    return 0;
}

/*
asmfpMANRbailout proc near uses si di
        fld     qword ptr new+8
        fld     st
        fmul    st,st                   ; ny2 ny
        fst     tempsqry
        fld     qword ptr new           ; nx ny2 ny
        fld     st
        fmul    st,st                   ; nx2 nx ny2 ny
        fst     tempsqrx
        faddp   st(2),st                ; nx nx2+ny2 ny
        fxch    st(1)                   ; nx2+ny2 nx ny
        fstp    magnitude               ; nx ny
        fadd                            ; nx+ny
        fmul    st,st                   ; square, don't need abs
        fcomp   rqlim                   ; ** stack is empty
        fstsw   ax                      ; ** 287 and up only
        sahf
        jae     bailout
        jmp     short nobailout
nobailoutp:
        finit           ; cleans up stack
nobailout:
        mov     si,offset new
        mov     di,offset old
        mov     ax,ds
        mov     es,ax
        mov     cx,8
        rep     movsw
        xor     ax,ax
        ret
bailout:
        mov     ax,1
        ret
asmfpMANRbailout endp
*/
int asmfpMANRbailout()
{
    // TODO: verify this code is correct
    tempsqrx = sqr(g_new.x);
    tempsqry = sqr(g_new.y);
    magnitude = fabs(g_new.x + g_new.y);
    if (magnitude*magnitude >= rqlim)
    {
        return 1;
    }
    old = g_new;
    return 0;
}
