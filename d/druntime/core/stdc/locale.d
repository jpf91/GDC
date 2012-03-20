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
module core.stdc.locale;

extern (C):

nothrow:

enum LC_CTYPE          = 0;
enum LC_NUMERIC        = 1;
enum LC_TIME           = 2;
enum LC_COLLATE        = 3;
enum LC_MONETARY       = 4;
enum LC_ALL            = 6;
enum LC_PAPER          = 7;  // non-standard
enum LC_NAME           = 8;  // non-standard
enum LC_ADDRESS        = 9;  // non-standard
enum LC_TELEPHONE      = 10; // non-standard
enum LC_MEASUREMENT    = 11; // non-standard
enum LC_IDENTIFICATION = 12; // non-standard

char*  setlocale(int category, in char* locale);
