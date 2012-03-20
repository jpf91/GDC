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
module core.sys.posix.poll;

private import core.sys.posix.config;

extern (C):

//
// XOpen (XSI)
//
/*
struct pollfd
{
    int     fd;
    short   events;
    short   revents;
}

nfds_t

POLLIN
POLLRDNORM
POLLRDBAND
POLLPRI
POLLOUT
POLLWRNORM
POLLWRBAND
POLLERR
POLLHUP
POLLNVAL

int poll(pollfd[], nfds_t, int);
*/

struct pollfd
{
    int     fd;
    short   events;
    short   revents;
}

alias uint nfds_t;

enum
{
    POLLIN      = 0x001,
    POLLRDNORM  = 0x040,
    POLLRDBAND  = 0x080,
    POLLPRI     = 0x002,
    POLLOUT     = 0x004,
    POLLWRNORM  = 0x100,
    POLLWRBAND  = 0x200,
    POLLERR     = 0x008,
    POLLHUP     = 0x010,
    POLLNVAL    = 0x020,
}

int poll(pollfd*, nfds_t, c_long);
