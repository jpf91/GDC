/**
 * D header file for C99.
 *
 * Copyright: Copyright Digital Mars 2000 - 2009.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt">Boost License 1.0</a>.
 * Authors:   Walter Bright, Hauke Duden
 * Standards: ISO/IEC 9899:1999 (E)
 */

/*          Copyright Digital Mars 2000 - 2009.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

/* NOTE: This file has been patched from the original DMD distribution to
   work with the GDC compiler.
 */
module core.stdc.stdarg;

@system:

version( GNU )
{
    private import gcc.builtins;
    alias __builtin_va_list va_list;
    alias __builtin_va_list __va_list; // C ABI va_list
    alias __builtin_va_end va_end;
    alias __builtin_va_copy va_copy;

    version( X86_64 )
    {   // TODO: implement this?
        alias __builtin_va_list __va_argsave_t;
    }

    // The va_start and va_arg template functions are magically
    // handled by the compiler.
    void va_start(T)(out va_list ap, ref T parmn);
    T va_arg(T)(ref va_list ap);
}
else
{
    static assert(false, "Unsupported platform");
}
