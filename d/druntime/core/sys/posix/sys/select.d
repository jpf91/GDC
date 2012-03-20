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
module core.sys.posix.sys.select;

private import core.sys.posix.config;
public import core.stdc.time;           // for timespec
public import core.sys.posix.sys.time;  // for timeval
public import core.sys.posix.sys.types; // for time_t
public import core.sys.posix.signal;    // for sigset_t

//debug=select;  // uncomment to turn on debugging printf's
version(unittest) import core.stdc.stdio: printf;

extern (C):

//
// Required
//
/*
NOTE: This module requires timeval from core.sys.posix.sys.time, but timeval
      is supposedly an XOpen extension.  As a result, this header will not
      compile on platforms that are not XSI-compliant.  This must be resolved
      on a per-platform basis.

fd_set

void FD_CLR(int fd, fd_set* fdset);
int FD_ISSET(int fd, const(fd_set)* fdset);
void FD_SET(int fd, fd_set* fdset);
void FD_ZERO(fd_set* fdset);

FD_SETSIZE

int  pselect(int, fd_set*, fd_set*, fd_set*, in timespec*, in sigset_t*);
int  select(int, fd_set*, fd_set*, fd_set*, timeval*);
*/

private
{
    alias c_ulong __fd_mask;
    enum __NFDBITS = __fd_mask.sizeof * 8;

    extern (D) auto __FDELT( int d )
    {
        return d / __NFDBITS;
    }

    extern (D) auto __FDMASK( int d )
    {
        return cast(__fd_mask) 1 << ( d % __NFDBITS );
    }
}

enum uint FD_SETSIZE = 1024;

struct fd_set
{
    __fd_mask[FD_SETSIZE / __NFDBITS] fds_bits;
}

extern (D) void FD_CLR( int fd, fd_set* fdset )
{
    fdset.fds_bits[__FDELT( fd )] &= ~__FDMASK( fd );
}

extern (D) bool FD_ISSET( int fd, const(fd_set)* fdset )
{
    return (fdset.fds_bits[__FDELT( fd )] & __FDMASK( fd )) != 0;
}

extern (D) void FD_SET( int fd, fd_set* fdset )
{
    fdset.fds_bits[__FDELT( fd )] |= __FDMASK( fd );
}

extern (D) void FD_ZERO( fd_set* fdset )
{
    fdset.fds_bits[0 .. $] = 0;
}

int pselect(int, fd_set*, fd_set*, fd_set*, in timespec*, in sigset_t*);
int select(int, fd_set*, fd_set*, fd_set*, timeval*);

unittest
{
    debug(select) printf("core.sys.posix.sys.select unittest\n");

    fd_set fd;

    for (auto i = 0; i < FD_SETSIZE; i++)
    {
        assert(!FD_ISSET(i, &fd));
    }

    for (auto i = 0; i < FD_SETSIZE; i++)
    {
        if ((i & -i) == i)
            FD_SET(i, &fd);
    }

    for (auto i = 0; i < FD_SETSIZE; i++)
    {
        if ((i & -i) == i)
            assert(FD_ISSET(i, &fd));
        else
            assert(!FD_ISSET(i, &fd));
    }

    for (auto i = 0; i < FD_SETSIZE; i++)
    {
        if ((i & -i) == i)
            FD_CLR(i, &fd);
        else
            FD_SET(i, &fd);
    }

    for (auto i = 0; i < FD_SETSIZE; i++)
    {
        if ((i & -i) == i)
            assert(!FD_ISSET(i, &fd));
        else
            assert(FD_ISSET(i, &fd));
    }

    FD_ZERO(&fd);

    for (auto i = 0; i < FD_SETSIZE; i++)
    {
        assert(!FD_ISSET(i, &fd));
    }
}

