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
module core.sys.posix.dirent;

private import core.sys.posix.config;
public import core.sys.posix.sys.types; // for ino_t

extern (C):

//
// Required
//
/*
DIR

struct dirent
{
    char[] d_name;
}

int     closedir(DIR*);
DIR*    opendir(in char*);
dirent* readdir(DIR*);
void    rewinddir(DIR*);
*/

version( linux )
{
    // NOTE: The following constants are non-standard Linux definitions
    //       for dirent.d_type.
    enum
    {
        DT_UNKNOWN  = 0,
        DT_FIFO     = 1,
        DT_CHR      = 2,
        DT_DIR      = 4,
        DT_BLK      = 6,
        DT_REG      = 8,
        DT_LNK      = 10,
        DT_SOCK     = 12,
        DT_WHT      = 14
    }

    struct dirent
    {
        ulong       d_ino;
        long        d_off;
        ushort      d_reclen;
        ubyte       d_type;
        char[256]   d_name;
    }

    struct DIR
    {
        // Managed by OS
    }

    dirent* readdir(DIR*);
}

int     closedir(DIR*);
DIR*    opendir(in char*);
void    rewinddir(DIR*);

//
// Thread-Safe Functions (TSF)
//
/*
int readdir_r(DIR*, dirent*, dirent**);
*/

version( linux )
{
    int readdir_r(DIR*, dirent*, dirent**);
}
