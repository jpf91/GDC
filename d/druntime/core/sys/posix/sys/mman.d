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
module core.sys.posix.sys.mman;

private import core.sys.posix.config;
public import core.stdc.stddef;          // for size_t
public import core.sys.posix.sys.types; // for off_t, mode_t

extern (C):

//
// Advisory Information (ADV)
//
/*
int posix_madvise(void*, size_t, int);
*/

//
// Advisory Information and either Memory Mapped Files or Shared Memory Objects (MC1)
//
/*
POSIX_MADV_NORMAL
POSIX_MADV_SEQUENTIAL
POSIX_MADV_RANDOM
POSIX_MADV_WILLNEED
POSIX_MADV_DONTNEED
*/

//
// Memory Mapped Files, Shared Memory Objects, or Memory Protection (MC2)
//
/*
PROT_READ
PROT_WRITE
PROT_EXEC
PROT_NONE
*/

version( linux )
{
    enum PROT_NONE      = 0x0;
    enum PROT_READ      = 0x1;
    enum PROT_WRITE     = 0x2;
    enum PROT_EXEC      = 0x4;
}

//
// Memory Mapped Files, Shared Memory Objects, or Typed Memory Objects (MC3)
//
/*
void* mmap(void*, size_t, int, int, int, off_t);
int munmap(void*, size_t);
*/


void* mmap(void*, size_t, int, int, int, off_t);
int   munmap(void*, size_t);

//
// Memory Mapped Files (MF)
//
/*
MAP_SHARED (MF|SHM)
MAP_PRIVATE (MF|SHM)
MAP_FIXED  (MF|SHM)
MAP_FAILED (MF|SHM)

MS_ASYNC (MF|SIO)
MS_SYNC (MF|SIO)
MS_INVALIDATE (MF|SIO)

int msync(void*, size_t, int); (MF|SIO)
*/

enum MAP_SHARED     = 0x01;
enum MAP_PRIVATE    = 0x02;
enum MAP_FIXED      = 0x10;
enum MAP_ANON       = 0x20; // non-standard

enum MAP_FAILED     = cast(void*) -1;

enum
{
    MS_ASYNC         = 1,
    MS_SYNC          = 4,
    MS_INVALIDATE    = 2
}

int msync(void*, size_t, int);

//
// Process Memory Locking (ML)
//
/*
MCL_CURRENT
MCL_FUTURE

int mlockall(int);
int munlockall();
*/

enum MCL_CURRENT    = 1;
enum MCL_FUTURE     = 2;

int mlockall(int);
int munlockall();


//
// Range Memory Locking (MLR)
//
/*
int mlock(in void*, size_t);
int munlock(in void*, size_t);
*/

int mlock(in void*, size_t);
int munlock(in void*, size_t);

//
// Memory Protection (MPR)
//
/*
int mprotect(void*, size_t, int);
*/

int mprotect(void*, size_t, int);

//
// Shared Memory Objects (SHM)
//
/*
int shm_open(in char*, int, mode_t);
int shm_unlink(in char*);
*/
//
// Typed Memory Objects (TYM)
//
/*
POSIX_TYPED_MEM_ALLOCATE
POSIX_TYPED_MEM_ALLOCATE_CONTIG
POSIX_TYPED_MEM_MAP_ALLOCATABLE

struct posix_typed_mem_info
{
    size_t posix_tmi_length;
}

int posix_mem_offset(in void*, size_t, off_t *, size_t *, int *);
int posix_typed_mem_get_info(int, struct posix_typed_mem_info *);
int posix_typed_mem_open(in char*, int, int);
*/
