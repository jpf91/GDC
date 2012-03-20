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
module core.sys.posix.pthread;

private import core.sys.posix.config;
public import core.sys.posix.sys.types;
public import core.sys.posix.sched;
public import core.sys.posix.time;

import core.stdc.stdint;

extern (C):

//
// Required
//
/*
PTHREAD_CANCEL_ASYNCHRONOUS
PTHREAD_CANCEL_ENABLE
PTHREAD_CANCEL_DEFERRED
PTHREAD_CANCEL_DISABLE
PTHREAD_CANCELED
PTHREAD_COND_INITIALIZER
PTHREAD_CREATE_DETACHED
PTHREAD_CREATE_JOINABLE
PTHREAD_EXPLICIT_SCHED
PTHREAD_INHERIT_SCHED
PTHREAD_MUTEX_INITIALIZER
PTHREAD_ONCE_INIT
PTHREAD_PROCESS_SHARED
PTHREAD_PROCESS_PRIVATE

int pthread_atfork(void function(), void function(), void function());
int pthread_attr_destroy(pthread_attr_t*);
int pthread_attr_getdetachstate(in pthread_attr_t*, int*);
int pthread_attr_getschedparam(in pthread_attr_t*, sched_param*);
int pthread_attr_init(pthread_attr_t*);
int pthread_attr_setdetachstate(pthread_attr_t*, int);
int pthread_attr_setschedparam(in pthread_attr_t*, sched_param*);
int pthread_cancel(pthread_t);
void pthread_cleanup_push(void function(void*), void*);
void pthread_cleanup_pop(int);
int pthread_cond_broadcast(pthread_cond_t*);
int pthread_cond_destroy(pthread_cond_t*);
int pthread_cond_init(in pthread_cond_t*, pthread_condattr_t*);
int pthread_cond_signal(pthread_cond_t*);
int pthread_cond_timedwait(pthread_cond_t*, pthread_mutex_t*, in timespec*);
int pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*);
int pthread_condattr_destroy(pthread_condattr_t*);
int pthread_condattr_init(pthread_condattr_t*);
int pthread_create(pthread_t*, in pthread_attr_t*, void* function(void*), void*);
int pthread_detach(pthread_t);
int pthread_equal(pthread_t, pthread_t);
void pthread_exit(void*);
void* pthread_getspecific(pthread_key_t);
int pthread_join(pthread_t, void**);
int pthread_key_create(pthread_key_t*, void function(void*));
int pthread_key_delete(pthread_key_t);
int pthread_mutex_destroy(pthread_mutex_t*);
int pthread_mutex_init(pthread_mutex_t*, pthread_mutexattr_t*);
int pthread_mutex_lock(pthread_mutex_t*);
int pthread_mutex_trylock(pthread_mutex_t*);
int pthread_mutex_unlock(pthread_mutex_t*);
int pthread_mutexattr_destroy(pthread_mutexattr_t*);
int pthread_mutexattr_init(pthread_mutexattr_t*);
int pthread_once(pthread_once_t*, void function());
int pthread_rwlock_destroy(pthread_rwlock_t*);
int pthread_rwlock_init(in pthread_rwlock_t*, pthread_rwlockattr_t*);
int pthread_rwlock_rdlock(pthread_rwlock_t*);
int pthread_rwlock_tryrdlock(pthread_rwlock_t*);
int pthread_rwlock_trywrlock(pthread_rwlock_t*);
int pthread_rwlock_unlock(pthread_rwlock_t*);
int pthread_rwlock_wrlock(pthread_rwlock_t*);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t*);
int pthread_rwlockattr_init(pthread_rwlockattr_t*);
pthread_t pthread_self();
int pthread_setcancelstate(int, int*);
int pthread_setcanceltype(int, int*);
int pthread_setspecific(pthread_key_t, in void*);
void pthread_testcancel();
*/
version( linux )
{
    //enum pthread_mutex_t PTHREAD_COND_INITIALIZER = { };

    enum
    {
        PTHREAD_CREATE_JOINABLE,
        PTHREAD_CREATE_DETACHED
    }

    //enum pthread_mutex_t PTHREAD_MUTEX_INITIALIZER = { };

    enum PTHREAD_ONCE_INIT = 0;

    enum
    {
        PTHREAD_PROCESS_PRIVATE,
        PTHREAD_PROCESS_SHARED
    }
    
}

int pthread_attr_destroy(pthread_attr_t*);
int pthread_attr_getdetachstate(in pthread_attr_t*, int*);
int pthread_attr_getschedparam(in pthread_attr_t*, sched_param*);
int pthread_attr_init(pthread_attr_t*);
int pthread_attr_setdetachstate(pthread_attr_t*, int);
int pthread_attr_setschedparam(in pthread_attr_t*, sched_param*);

version( linux )
{
    alias void function(void*) _pthread_cleanup_routine;

    struct _pthread_cleanup_buffer
    {
        _pthread_cleanup_buffer*    __prev;
        _pthread_cleanup_routine    __routine;
        void*                       __arg;
    }

    void __pthread_cleanup_push(_pthread_cleanup_buffer*, _pthread_cleanup_routine, void*);
    void __pthread_cleanup_pop(_pthread_cleanup_buffer*, int);

    struct pthread_cleanup
    {
        _pthread_cleanup_buffer buffer = void;

        void push()( _pthread_cleanup_routine routine, void* arg )
        {
            __pthread_cleanup_push( &buffer, routine, arg );
        }

        void pop()( int execute )
        {
            __pthread_cleanup_pop( &buffer, execute );
        }
    }
}

int pthread_cond_broadcast(pthread_cond_t*);
int pthread_cond_destroy(pthread_cond_t*);
int pthread_cond_init(in pthread_cond_t*, pthread_condattr_t*);
int pthread_cond_signal(pthread_cond_t*);
int pthread_cond_timedwait(pthread_cond_t*, pthread_mutex_t*, in timespec*);
int pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*);

int pthread_create(pthread_t*, in pthread_attr_t*, void* function(void*), void*);
int pthread_detach(pthread_t);
int pthread_equal(pthread_t, pthread_t);
void pthread_exit(void*);
void* pthread_getspecific(pthread_key_t);
int pthread_join(pthread_t, void**);
int pthread_key_create(pthread_key_t*, void function(void*));
int pthread_key_delete(pthread_key_t);
int pthread_mutex_destroy(pthread_mutex_t*);
int pthread_mutex_init(pthread_mutex_t*, pthread_mutexattr_t*);
int pthread_mutex_lock(pthread_mutex_t*);
int pthread_mutex_trylock(pthread_mutex_t*);
int pthread_mutex_unlock(pthread_mutex_t*);
int pthread_mutexattr_destroy(pthread_mutexattr_t*);
int pthread_mutexattr_init(pthread_mutexattr_t*);
int pthread_once(pthread_once_t*, void function());
pthread_t pthread_self();
int pthread_setspecific(pthread_key_t, in void*);

//
// Barrier (BAR)
//
/*
PTHREAD_BARRIER_SERIAL_THREAD

int pthread_barrier_destroy(pthread_barrier_t*);
int pthread_barrier_init(pthread_barrier_t*, in pthread_barrierattr_t*, uint);
int pthread_barrier_wait(pthread_barrier_t*);
int pthread_barrierattr_destroy(pthread_barrierattr_t*);
int pthread_barrierattr_getpshared(in pthread_barrierattr_t*, int*); (BAR|TSH)
int pthread_barrierattr_init(pthread_barrierattr_t*);
int pthread_barrierattr_setpshared(pthread_barrierattr_t*, int); (BAR|TSH)
*/

//
// Clock (CS)
//
/*
int pthread_condattr_getclock(in pthread_condattr_t*, clockid_t*);
int pthread_condattr_setclock(pthread_condattr_t*, clockid_t);
*/

//
// Spinlock (SPI)
//
/*
int pthread_spin_destroy(pthread_spinlock_t*);
int pthread_spin_init(pthread_spinlock_t*, int);
int pthread_spin_lock(pthread_spinlock_t*);
int pthread_spin_trylock(pthread_spinlock_t*);
int pthread_spin_unlock(pthread_spinlock_t*);
*/

//
// XOpen (XSI)
//
/*
PTHREAD_MUTEX_DEFAULT
PTHREAD_MUTEX_ERRORCHECK
PTHREAD_MUTEX_NORMAL
PTHREAD_MUTEX_RECURSIVE

int pthread_attr_getguardsize(in pthread_attr_t*, size_t*);
int pthread_attr_setguardsize(pthread_attr_t*, size_t);
int pthread_getconcurrency();
int pthread_mutexattr_gettype(in pthread_mutexattr_t*, int*);
int pthread_mutexattr_settype(pthread_mutexattr_t*, int);
int pthread_setconcurrency(int);
*/

enum PTHREAD_MUTEX_NORMAL       = 0;
enum PTHREAD_MUTEX_RECURSIVE    = 1;
enum PTHREAD_MUTEX_ERRORCHECK   = 2;
enum PTHREAD_MUTEX_DEFAULT      = PTHREAD_MUTEX_NORMAL;

int pthread_attr_getguardsize(in pthread_attr_t*, size_t*);
int pthread_attr_setguardsize(pthread_attr_t*, size_t);
int pthread_mutexattr_gettype(in pthread_mutexattr_t*, int*);
int pthread_mutexattr_settype(pthread_mutexattr_t*, int);

//
// CPU Time (TCT)
//
/*
int pthread_getcpuclockid(pthread_t, clockid_t*);
*/

int pthread_getcpuclockid(pthread_t, clockid_t*);

//
// Timeouts (TMO)
//
/*
int pthread_mutex_timedlock(pthread_mutex_t*, timespec*);
int pthread_rwlock_timedrdlock(pthread_rwlock_t*, in timespec*);
int pthread_rwlock_timedwrlock(pthread_rwlock_t*, in timespec*);
*/

int pthread_mutex_timedlock(pthread_mutex_t*, timespec*);

//
// Priority (TPI|TPP)
//
/*
PTHREAD_PRIO_INHERIT (TPI)
PTHREAD_PRIO_NONE (TPP|TPI)
PTHREAD_PRIO_PROTECT (TPI)

int pthread_mutex_getprioceiling(in pthread_mutex_t*, int*); (TPP)
int pthread_mutex_setprioceiling(pthread_mutex_t*, int, int*); (TPP)
int pthread_mutexattr_getprioceiling(pthread_mutexattr_t*, int*); (TPP)
int pthread_mutexattr_getprotocol(in pthread_mutexattr_t*, int*); (TPI|TPP)
int pthread_mutexattr_setprioceiling(pthread_mutexattr_t*, int); (TPP)
int pthread_mutexattr_setprotocol(pthread_mutexattr_t*, int); (TPI|TPP)
*/

//
// Scheduling (TPS)
//
/*
PTHREAD_SCOPE_PROCESS
PTHREAD_SCOPE_SYSTEM

int pthread_attr_getinheritsched(in pthread_attr_t*, int*);
int pthread_attr_getschedpolicy(in pthread_attr_t*, int*);
int pthread_attr_getscope(in pthread_attr_t*, int*);
int pthread_attr_setinheritsched(pthread_attr_t*, int);
int pthread_attr_setschedpolicy(pthread_attr_t*, int);
int pthread_attr_setscope(pthread_attr_t*, int);
int pthread_getschedparam(pthread_t, int*, sched_param*);
int pthread_setschedparam(pthread_t, int, in sched_param*);
int pthread_setschedprio(pthread_t, int);
*/

enum
{
    PTHREAD_SCOPE_SYSTEM,
    PTHREAD_SCOPE_PROCESS
}

int pthread_attr_getschedpolicy(in pthread_attr_t*, int*);
int pthread_attr_getscope(in pthread_attr_t*, int*);
int pthread_attr_setschedpolicy(pthread_attr_t*, int);
int pthread_attr_setscope(pthread_attr_t*, int);
int pthread_getschedparam(pthread_t, int*, sched_param*);
int pthread_setschedparam(pthread_t, int, in sched_param*);

//
// Stack (TSA|TSS)
//
/*
int pthread_attr_getstack(in pthread_attr_t*, void**, size_t*); (TSA|TSS)
int pthread_attr_getstackaddr(in pthread_attr_t*, void**); (TSA)
int pthread_attr_getstacksize(in pthread_attr_t*, size_t*); (TSS)
int pthread_attr_setstack(pthread_attr_t*, void*, size_t); (TSA|TSS)
int pthread_attr_setstackaddr(pthread_attr_t*, void*); (TSA)
int pthread_attr_setstacksize(pthread_attr_t*, size_t); (TSS)
*/

int pthread_attr_getstack(in pthread_attr_t*, void**, size_t*);
int pthread_attr_getstackaddr(in pthread_attr_t*, void**);
int pthread_attr_getstacksize(in pthread_attr_t*, size_t*);
int pthread_attr_setstack(pthread_attr_t*, void*, size_t);
int pthread_attr_setstackaddr(pthread_attr_t*, void*);
int pthread_attr_setstacksize(pthread_attr_t*, size_t);

//
// Shared Synchronization (TSH)
//
/*
int pthread_condattr_getpshared(in pthread_condattr_t*, int*);
int pthread_condattr_setpshared(pthread_condattr_t*, int);
int pthread_mutexattr_getpshared(in pthread_mutexattr_t*, int*);
int pthread_mutexattr_setpshared(pthread_mutexattr_t*, int);
int pthread_rwlockattr_getpshared(in pthread_rwlockattr_t*, int*);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t*, int);
*/

int pthread_mutexattr_getpshared(in pthread_mutexattr_t*, int*);
int pthread_mutexattr_setpshared(pthread_mutexattr_t*, int);
