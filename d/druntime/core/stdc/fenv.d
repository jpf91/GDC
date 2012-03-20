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
module core.stdc.fenv;

extern (C):

alias uint fenv_t;
alias uint fexcept_t;

enum
{
    FE_INVALID      = 0x0001,
    FE_DIVBYZERO    = 0x0002,
    FE_OVERFLOW     = 0x0004,
    FE_UNDERFLOW    = 0x0008,
    FE_INEXACT      = 0x0010,
    FE_ALL_EXCEPT   = (FE_DIVBYZERO | FE_INEXACT | FE_INVALID
                      | FE_OVERFLOW | FE_UNDERFLOW),
    FE_TONEAREST    = 0x0000,
    FE_TOWARDZERO   = 0x0001,
    FE_UPWARD       = 0x0002,
    FE_DOWNWARD     = 0x0003,
    _ROUND_MASK     = (FE_TONEAREST | FE_DOWNWARD | FE_UPWARD | FE_TOWARDZERO)
}
pragma(msg, "FE_DENORMAL not available on this platform!");

private extern(C) const fenv_t __fe_dfl_env;
const fenv_t* FE_DFL_ENV = &__fe_dfl_env;

void feraiseexcept(int excepts);
void feclearexcept(int excepts);

int fetestexcept(int excepts);
int feholdexcept(fenv_t* envp);

void fegetexceptflag(fexcept_t* flagp, int excepts);
void fesetexceptflag(in fexcept_t* flagp, int excepts);

/*
 * Apparently, the rounding mode is specified as part of the
 * instruction format on ARM, so the dynamic rounding mode is
 * indeterminate.  Some FPUs may differ.
 */
int fegetround();
int fesetround(int round);

int fegetenv(fenv_t* envp);
int fesetenv(in fenv_t* envp);
int feupdateenv(in fenv_t* envp);
