/**
 * D header file for POSIX.
 *
 * Copyright: Copyright Sean Kelly 2005 - 2009.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt">Boost License 1.0</a>.
 * Authors:   Sean Kelly
 * Standards: The Open Group Base Specifications Issue 6, IEEE Std 1003.1, 2004 Edition
 */

/*          Copyright Sean Kelly 2005 - 2009.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
module core.sys.posix.setjmp;

private import core.sys.posix.config;
private import core.sys.posix.signal; // for sigset_t

extern (C):

//
// Required
//
/*
jmp_buf

int  setjmp(ref jmp_buf);
void longjmp(ref jmp_buf, int);
*/


enum _JBLEN = 64;
struct jmp_buf { c_long[_JBLEN] _jb; }

int  setjmp(ref jmp_buf);
void longjmp(ref jmp_buf, int);

//
// C Extension (CX)
//
/*
sigjmp_buf

int  sigsetjmp(sigjmp_buf, int);
void siglongjmp(sigjmp_buf, int);
*/

struct sigjmp_buf { c_long[_JBLEN + 1] _jb; }

int  sigsetjmp(ref sigjmp_buf);
void siglongjmp(ref sigjmp_buf, int);

//
// XOpen (XSI)
//
/*
int  _setjmp(jmp_buf);
void _longjmp(jmp_buf, int);
*/

int  _setjmp(ref jmp_buf);
void _longjmp(ref jmp_buf, int);
