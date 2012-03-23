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
module core.stdc.wchar_;

private import core.stdc.config;
private import core.stdc.stdarg; // for va_list
private import core.stdc.stdio;  // for FILE, not exposed per spec
public import core.stdc.stddef;  // for size_t, wchar_t
public import core.stdc.time;    // for tm
public import core.stdc.stdint;  // for WCHAR_MIN, WCHAR_MAX

extern (C):

nothrow:

alias int     mbstate_t;
alias wchar_t wint_t;

enum wchar_t WEOF = cast(wchar_t)(-1);
