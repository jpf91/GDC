/**
 * D header file for C99.
 *
 * Copyright: Copyright Sean Kelly 2005 - 2009.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt">Boost License 1.0</a>.
 * Authors:   Sean Kelly
 * Standards: ISO/IEC 9899:1999 (E)
 */

/*          Copyright Sean Kelly 2005 - 2009.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.
*/
module core.stdc.math;

version(GNU) import gcc.builtins;

private import core.stdc.config;

extern (C):
nothrow:

alias float  float_t;
alias double double_t;

enum double HUGE_VAL      = double.infinity;
enum double HUGE_VALF     = float.infinity;
enum double HUGE_VALL     = real.infinity;

enum float INFINITY       = float.infinity;
enum float NAN            = float.nan;

enum int FP_ILOGB0        = int.min;
enum int FP_ILOGBNAN      = int.min;

enum int MATH_ERRNO       = 1;
enum int MATH_ERREXCEPT   = 2;
enum int math_errhandling = MATH_ERREXCEPT;

version( none )
{
    //
    // these functions are all macros in C
    //

    //int fpclassify(real-floating x);
    int fpclassify(float x);
    int fpclassify(double x);
    int fpclassify(real x);

    //int isfinite(real-floating x);
    int isfinite(float x);
    int isfinite(double x);
    int isfinite(real x);

    //int isinf(real-floating x);
    int isinf(float x);
    int isinf(double x);
    int isinf(real x);

    //int isnan(real-floating x);
    int isnan(float x);
    int isnan(double x);
    int isnan(real x);

    //int isnormal(real-floating x);
    int isnormal(float x);
    int isnormal(double x);
    int isnormal(real x);

    //int signbit(real-floating x);
    int signbit(float x);
    int signbit(double x);
    int signbit(real x);

    //int isgreater(real-floating x, real-floating y);
    int isgreater(float x, float y);
    int isgreater(double x, double y);
    int isgreater(real x, real y);

    //int isgreaterequal(real-floating x, real-floating y);
    int isgreaterequal(float x, float y);
    int isgreaterequal(double x, double y);
    int isgreaterequal(real x, real y);

    //int isless(real-floating x, real-floating y);
    int isless(float x, float y);
    int isless(double x, double y);
    int isless(real x, real y);

    //int islessequal(real-floating x, real-floating y);
    int islessequal(float x, float y);
    int islessequal(double x, double y);
    int islessequal(real x, real y);

    //int islessgreater(real-floating x, real-floating y);
    int islessgreater(float x, float y);
    int islessgreater(double x, double y);
    int islessgreater(real x, real y);

    //int isunordered(real-floating x, real-floating y);
    int isunordered(float x, float y);
    int isunordered(double x, double y);
    int isunordered(real x, real y);
}

enum
{
    FP_INFINITE  = 0x01,
    FP_NAN       = 0x02,
    FP_NORMAL    = 0x04,
    FP_SUBNORMAL = 0x08,
    FP_ZERO      = 0x10,
}

enum
{
    FP_FAST_FMA  = 0,
    FP_FAST_FMAF = 0,
    FP_FAST_FMAL = 0,
}

int __fpclassifyd(double);
int __fpclassifyf(float);
int __fpclassifyl(real);
int __isfinitef(float);
int __isfinite(double);
int __isfinitel(real);
int __isinff(float);
int __isinfl(real);
int __isnanl(real);
int __isnormalf(float);
int __isnormal(double);
int __isnormall(real);
int __signbit(double);
int __signbitf(float);
int __signbitl(real);

extern (D)
{
    //int fpclassify(real-floating x);
    int fpclassify(float x)     { return __fpclassifyf(x); }
    int fpclassify(double x)    { return __fpclassifyd(x); }
    int fpclassify(real x)      { return __fpclassifyl(x); }

    //int isfinite(real-floating x);
    int isfinite(float x)       { return __isfinitef(x); }
    int isfinite(double x)      { return __isfinite(x); }
    int isfinite(real x)        { return __isfinitel(x); }

    //int isinf(real-floating x);
    int isinf(float x)          { return __isinff(x); }
    int isinf(double x)         { return __isinfl(x); }
    int isinf(real x)           { return __isinfl(x); }

    //int isnan(real-floating x);
    int isnan(float x)          { return __isnanl(x); }
    int isnan(double x)         { return __isnanl(x); }
    int isnan(real x)           { return __isnanl(x); }

    //int isnormal(real-floating x);
    int isnormal(float x)       { return __isnormalf(x); }
    int isnormal(double x)      { return __isnormal(x); }
    int isnormal(real x)        { return __isnormall(x); }

    //int signbit(real-floating x);
    int signbit(float x)        { return __signbitf(x); }
    int signbit(double x)       { return __signbit(x); }
    int signbit(real x)         { return __signbit(x); }
}

extern (D)
{
    //int isgreater(real-floating x, real-floating y);
    int isgreater(float x, float y)        { return !(x !>  y); }
    int isgreater(double x, double y)      { return !(x !>  y); }
    int isgreater(real x, real y)          { return !(x !>  y); }

    //int isgreaterequal(real-floating x, real-floating y);
    int isgreaterequal(float x, float y)   { return !(x !>= y); }
    int isgreaterequal(double x, double y) { return !(x !>= y); }
    int isgreaterequal(real x, real y)     { return !(x !>= y); }

    //int isless(real-floating x, real-floating y);
    int isless(float x, float y)           { return !(x !<  y); }
    int isless(double x, double y)         { return !(x !<  y); }
    int isless(real x, real y)             { return !(x !<  y); }

    //int islessequal(real-floating x, real-floating y);
    int islessequal(float x, float y)      { return !(x !<= y); }
    int islessequal(double x, double y)    { return !(x !<= y); }
    int islessequal(real x, real y)        { return !(x !<= y); }

    //int islessgreater(real-floating x, real-floating y);
    int islessgreater(float x, float y)    { return !(x !<> y); }
    int islessgreater(double x, double y)  { return !(x !<> y); }
    int islessgreater(real x, real y)      { return !(x !<> y); }

    //int isunordered(real-floating x, real-floating y);
    int isunordered(float x, float y)      { return (x !<>= y); }
    int isunordered(double x, double y)    { return (x !<>= y); }
    int isunordered(real x, real y)        { return (x !<>= y); }
}

version( GNU )
{
    alias __builtin_acos  acos;
    alias __builtin_acosf acosf;
    alias __builtin_acosl acosl;

    alias __builtin_asin  asin;
    alias __builtin_asinf asinf;
    alias __builtin_asinl asinl;

    alias __builtin_atan  atan;
    alias __builtin_atanf atanf;
    alias __builtin_atanl atanl;

    alias __builtin_atan2  atan2;
    alias __builtin_atan2f atan2f;
    alias __builtin_atan2l atan2l;

    alias __builtin_cos  cos;
    alias __builtin_cosf cosf;
    alias __builtin_cosl cosl;

    alias __builtin_sin  sin;
    alias __builtin_sinf sinf;
    alias __builtin_sinl sinl;

    alias __builtin_tan  tan;
    alias __builtin_tanf tanf;
    alias __builtin_tanl tanl;

    alias __builtin_acosh  acosh;
    alias __builtin_acoshf acoshf;

    alias __builtin_asinh  asinh;
    alias __builtin_asinhf asinhf;

    alias __builtin_atanh  atanh;
    alias __builtin_atanhf atanhf;

    alias __builtin_cosh  cosh;
    alias __builtin_coshf coshf;

    alias __builtin_sinh  sinh;
    alias __builtin_sinhf sinhf;

    alias __builtin_tanh  tanh;
    alias __builtin_tanhf tanhf;

    alias __builtin_exp  exp;
    alias __builtin_expf expf;

    alias __builtin_exp2  exp2;
    alias __builtin_exp2f exp2f;

    alias __builtin_expm1  expm1;
    alias __builtin_expm1f expm1f;

    alias __builtin_frexp  frexp;
    alias __builtin_frexpf frexpf;
    alias __builtin_frexpl frexpl;

    alias __builtin_ilogb  ilogb;
    alias __builtin_ilogbf ilogbf;
    alias __builtin_ilogbl ilogbl;

    alias __builtin_ldexp  ldexp;
    alias __builtin_ldexpf ldexpf;
    alias __builtin_ldexpl ldexpl;

    alias __builtin_log  log;
    alias __builtin_logf logf;

    alias __builtin_log10  log10;
    alias __builtin_log10f log10f;

    alias __builtin_log1p  log1p;
    alias __builtin_log1pf log1pf;

    alias __builtin_log2  log2;
    alias __builtin_log2f log2f;

    alias __builtin_logb  logb;
    alias __builtin_logbf logbf;
    alias __builtin_logbl logbl;

    alias __builtin_modf  modf;
    alias __builtin_modff modff;
    alias __builtin_modfl modfl;

    alias __builtin_scalbn  scalbn;
    alias __builtin_scalbnf scalbnf;
    alias __builtin_scalbnl scalbnl;

    alias __builtin_scalbln  scalbln;
    alias __builtin_scalblnf scalblnf;
    alias __builtin_scalblnl scalblnl;

    alias __builtin_cbrt  cbrt;
    alias __builtin_cbrtf cbrtf;

    alias __builtin_fabs  fabs;
    alias __builtin_fabsf fabsf;
    alias __builtin_fabsl fabsl;

    alias __builtin_hypot  hypot;
    alias __builtin_hypotf hypotf;
    alias __builtin_hypotl hypotl;

    alias __builtin_pow  pow;
    alias __builtin_powf powf;

    alias __builtin_sqrt  sqrt;
    alias __builtin_sqrtf sqrtf;
    alias __builtin_sqrtl sqrtl;

    alias __builtin_erf  erf;
    alias __builtin_erff erff;

    alias __builtin_erfc  erfc;
    alias __builtin_erfcf erfcf;

    alias __builtin_lgamma  lgamma;
    alias __builtin_lgammaf lgammaf;

    alias __builtin_tgamma  tgamma;
    alias __builtin_tgammaf tgammaf;

    alias __builtin_ceil  ceil;
    alias __builtin_ceilf ceilf;
    alias __builtin_ceill ceill;

    alias __builtin_floor  floor;
    alias __builtin_floorf floorf;
    alias __builtin_floorl floorl;

    alias __builtin_nearbyint  nearbyint;
    alias __builtin_nearbyintf nearbyintf;

    alias __builtin_rint  rint;
    alias __builtin_rintf rintf;
    alias __builtin_rintl rintl;

    alias __builtin_lrint  lrint;
    alias __builtin_lrintf lrintf;
    alias __builtin_lrintl lrintl;

    alias __builtin_llrint  llrint;
    alias __builtin_llrintf llrintf;
    alias __builtin_llrintl llrintl;

    alias __builtin_round  round;
    alias __builtin_roundf roundf;
    alias __builtin_roundl roundl;

    alias __builtin_lround  lround;
    alias __builtin_lroundf lroundf;
    alias __builtin_lroundl lroundl;

    alias __builtin_llround  llround;
    alias __builtin_llroundf llroundf;
    alias __builtin_llroundl llroundl;

    alias __builtin_trunc  trunc;
    alias __builtin_truncf truncf;
    alias __builtin_truncl truncl;

    alias __builtin_fmod  fmod;
    alias __builtin_fmodf fmodf;
    alias __builtin_fmodl fmodl;

    alias __builtin_remainder  remainder;
    alias __builtin_remainderf remainderf;
    alias __builtin_remainderl remainderl;

    alias __builtin_remquo  remquo;
    alias __builtin_remquof remquof;
    alias __builtin_remquol remquol;

    alias __builtin_copysign  copysign;
    alias __builtin_copysignf copysignf;
    alias __builtin_copysignl copysignl;

    alias __builtin_nan  nan;
    alias __builtin_nanf nanf;

    alias __builtin_nextafter  nextafter;
    alias __builtin_nextafterf nextafterf;
    alias __builtin_nextafterl nextafterl;

    alias __builtin_nexttoward  nexttoward;
    alias __builtin_nexttowardf nexttowardf;
    alias __builtin_nexttowardl nexttowardl;

    alias __builtin_fdim  fdim;
    alias __builtin_fdimf fdimf;
    alias __builtin_fdiml fdiml;

    alias __builtin_fmax  fmax;
    alias __builtin_fmaxf fmaxf;
    alias __builtin_fmaxl fmaxl;

    alias __builtin_fmin  fmin;
    alias __builtin_fminf fminf;
    alias __builtin_fminl fminl;

    alias __builtin_fma  fma;
    alias __builtin_fmaf fmaf;
    alias __builtin_fmal fmal;

    /**
    * FreeBSD is missing a few symbols in -lm, so for the symbols that are missing,
    * we use the lower resolution double versions for the long double functions.
    * For everything else we use the GCC builtins.
    */
    @trusted:
        /* This call to a builtin that is not at all pure. */
        real   lgammal(real x) { return lgamma(x); }

        /* For everything else, the builtins are pure by default. */
        pure:
        real   acoshl(real x) { return acosh(x); }
        real   asinhl(real x) { return asinh(x); }
        real   atanhl(real x) { return atanh(x); }

        real   coshl(real x) { return cosh(x); }
        real   sinhl(real x) { return sinh(x); }
        real   tanhl(real x) { return tanh(x); }

        real   expl(real x) { return exp(x); }
        real   exp2l(real x) { return exp2(x); }
        real   expm1l(real x) { return expm1(x); }

        real   logl(real x) { return log(x); }
        real   log10l(real x) { return log10(x); }
        real   log1pl(real x) { return log1p(x); }
        real   log2l(real x) { return log10(x) / log10(2); }

        real   cbrtl(real x) { return cbrt(x); }

        real   powl(real x, real y) { return pow(x, y); }

        real   erfl(real x) { return erf(x); }
        real   erfcl(real x) { return erfc(x); }

        real   tgammal(real x) { return tgamma(x); }

        real   nearbyintl(real x) { return nearbyint(x); }

        real   nanl(char *tagp) { return real.nan; }
}
