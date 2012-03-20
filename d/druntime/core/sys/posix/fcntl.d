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
module core.sys.posix.fcntl;

private import core.sys.posix.config;
private import core.stdc.stdint;
public import core.stdc.stddef;         // for size_t
public import core.sys.posix.sys.types; // for off_t, mode_t
public import core.sys.posix.sys.stat;  // for S_IFMT, etc.

extern (C):

//
// Required
//
/*
F_DUPFD
F_GETFD
F_SETFD
F_GETFL
F_SETFL
F_GETLK
F_SETLK
F_SETLKW
F_GETOWN
F_SETOWN

FD_CLOEXEC

F_RDLCK
F_UNLCK
F_WRLCK

O_CREAT
O_EXCL
O_NOCTTY
O_TRUNC

O_APPEND
O_DSYNC
O_NONBLOCK
O_RSYNC
O_SYNC

O_ACCMODE
O_RDONLY
O_RDWR
O_WRONLY

struct flock
{
    short   l_type;
    short   l_whence;
    off_t   l_start;
    off_t   l_len;
    pid_t   l_pid;
}

int creat(in char*, mode_t);
int fcntl(int, int, ...);
int open(in char*, int, ...);
*/
version( linux )
{
    enum F_DUPFD        = 0;
    enum F_GETFD        = 1;
    enum F_SETFD        = 2;
    enum F_GETFL        = 3;
    enum F_SETFL        = 4;
    enum F_GETLK        = 5;
    enum F_SETLK        = 6;
    enum F_SETLKW       = 7;
    enum F_GETOWN       = 9;
    enum F_SETOWN       = 8;

    enum FD_CLOEXEC     = 1;

    enum F_RDLCK        = 0;
    enum F_UNLCK        = 2;
    enum F_WRLCK        = 1;

    enum O_CREAT        = 0100;
    enum O_EXCL         = 0200;
    enum O_NOCTTY       = 0400;
    enum O_TRUNC        = 01000;

    enum O_APPEND       = 02000;
    enum O_NONBLOCK     = 04000;
    enum O_SYNC         = 010000;
    //enum O_DSYNC        = O_SYNC;
    //enum O_RSYNC        = O_SYNC;

    enum O_ACCMODE      = 0003;
    enum O_RDONLY       = 00;
    enum O_WRONLY       = 01;
    enum O_RDWR         = 02;

    struct flock
    {
        short   l_type;
        short   l_whence;
        off_t   l_start;
        off_t   l_len;
        pid_t   l_pid;
    }

    int   creat(in char*, mode_t);
    int   open(in char*, int, ...);
}

int fcntl(int, int, ...);

//
// Advisory Information (ADV)
//
/*
POSIX_FADV_NORMAL
POSIX_FADV_SEQUENTIAL
POSIX_FADV_RANDOM
POSIX_FADV_WILLNEED
POSIX_FADV_DONTNEED
POSIX_FADV_NOREUSE

int posix_fadvise(int, off_t, off_t, int);
int posix_fallocate(int, off_t, off_t);
*/
