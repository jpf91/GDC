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
module core.sys.posix.sys.types;

private import core.sys.posix.config;
private import core.stdc.stdint;
public import core.stdc.stddef; // for size_t
public import core.stdc.time;   // for clock_t, time_t

extern (C):

//
// Required
//
/*
blkcnt_t
blksize_t
dev_t
gid_t
ino_t
mode_t
nlink_t
off_t
pid_t
size_t
ssize_t
time_t
uid_t
*/

version( linux )
{
    alias c_long    blkcnt_t;
    alias c_ulong   ino_t;
    alias c_long    off_t;
    alias long      loff_t;
    alias c_ulong   blksize_t;
    alias uint      dev_t;
    alias uint      gid_t;
    alias ushort    mode_t;
    alias ushort    nlink_t;
    alias int       pid_t;
    //size_t (defined in core.stdc.stddef)
    alias c_long    ssize_t;
    //time_t (defined in core.stdc.time)
    alias uint      uid_t;
}

//
// XOpen (XSI)
//
/*
clock_t
fsblkcnt_t
fsfilcnt_t
id_t
key_t
suseconds_t
useconds_t
*/

version( linux )
{
    alias c_ulong   fsblkcnt_t;
    alias c_ulong   fsfilcnt_t;
    // clock_t (defined in core.stdc.time)
    alias uint      id_t;
    alias int       key_t;
    alias c_long    suseconds_t;
    alias c_long      useconds_t;
}

//
// Thread (THR)
//
/*
pthread_attr_t
pthread_cond_t
pthread_condattr_t
pthread_key_t
pthread_mutex_t
pthread_mutexattr_t
pthread_once_t
pthread_rwlock_t
pthread_rwlockattr_t
pthread_t
*/

version( linux )
{
    union pthread_attr_t
    {
        byte __size[24];
    }

    union pthread_cond_t
    {
        byte __size[4];
    }

    alias c_long pthread_condattr_t;

    alias int pthread_key_t;

    alias int pthread_mutex_t;

    alias c_long pthread_mutexattr_t;

    alias int pthread_once_t;

    alias c_long pthread_t;
}

//
// Barrier (BAR)
//
/*
pthread_barrier_t
pthread_barrierattr_t
*/

//
// Spin (SPN)
//
/*
pthread_spinlock_t
*/

//
// Timer (TMR)
//
/*
clockid_t
timer_t
*/

//
// Trace (TRC)
//
/*
trace_attr_t
trace_event_id_t
trace_event_set_t
trace_id_t
*/
