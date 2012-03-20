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
module core.stdc.wctype;

public  import core.stdc.wchar_; // for wint_t, WEOF

extern (C):
nothrow:

alias wchar_t wctrans_t;
alias wchar_t wctype_t;

pragma(msg, "Android/Bionic doesn't have an implementation for multibyte strings");
