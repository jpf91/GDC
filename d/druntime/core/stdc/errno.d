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
module core.stdc.errno;

/+
/**
 * Bionic specific
 */

/* internal function that should *only* be called from system calls */
/* use errno = xxxx instead in C code                               */
extern int    __set_errno(int  error);

/* internal function returning the address of the thread-specific errno */
extern volatile int*   __errno(void);
+/

extern (C) int getErrno();      // for internal use
extern (C) int setErrno(int);   // for internal use

@property int errno() { return getErrno(); }
@property int errno(int n) { return setErrno(n); }

extern (C):
nothrow:

/*
 * TODO: this does in theory not depend on the used C library, only on OS
 * (at least for linux. Is this true for other OS?)
 */

version(linux)
{
    enum EPERM              = 1;        // Operation not permitted
    enum ENOENT             = 2;        // No such file or directory
    enum ESRCH              = 3;        // No such process
    enum EINTR              = 4;        // Interrupted system call
    enum EIO                = 5;        // I/O error
    enum ENXIO              = 6;        // No such device or address
    enum E2BIG              = 7;        // Argument list too long
    enum ENOEXEC            = 8;        // Exec format error
    enum EBADF              = 9;        // Bad file number
    enum ECHILD             = 10;       // No child processes
    enum EAGAIN             = 11;       // Try again
    enum ENOMEM             = 12;       // Out of memory
    enum EACCES             = 13;       // Permission denied
    enum EFAULT             = 14;       // Bad address
    enum ENOTBLK            = 15;       // Block device required
    enum EBUSY              = 16;       // Device or resource busy
    enum EEXIST             = 17;       // File exists
    enum EXDEV              = 18;       // Cross-device link
    enum ENODEV             = 19;       // No such device
    enum ENOTDIR            = 20;       // Not a directory
    enum EISDIR             = 21;       // Is a directory
    enum EINVAL             = 22;       // Invalid argument
    enum ENFILE             = 23;       // File table overflow
    enum EMFILE             = 24;       // Too many open files
    enum ENOTTY             = 25;       // Not a typewriter
    enum ETXTBSY            = 26;       // Text file busy
    enum EFBIG              = 27;       // File too large
    enum ENOSPC             = 28;       // No space left on device
    enum ESPIPE             = 29;       // Illegal seek
    enum EROFS              = 30;       // Read-only file system
    enum EMLINK             = 31;       // Too many links
    enum EPIPE              = 32;       // Broken pipe
    enum EDOM               = 33;       // Math argument out of domain of func
    enum ERANGE             = 34;       // Math result not representable
    enum EDEADLK            = 35;       // Resource deadlock would occur
    enum ENAMETOOLONG       = 36;       // File name too long
    enum ENOLCK             = 37;       // No record locks available
    enum ENOSYS             = 38;       // Function not implemented
    enum ENOTEMPTY          = 39;       // Directory not empty
    enum ELOOP              = 40;       // Too many symbolic links encountered
    enum EWOULDBLOCK        = EAGAIN;   // Operation would block
    enum ENOMSG             = 42;       // No message of desired type
    enum EIDRM              = 43;       // Identifier removed
    enum ECHRNG             = 44;       // Channel number out of range
    enum EL2NSYNC           = 45;       // Level 2 not synchronized
    enum EL3HLT             = 46;       // Level 3 halted
    enum EL3RST             = 47;       // Level 3 reset
    enum ELNRNG             = 48;       // Link number out of range
    enum EUNATCH            = 49;       // Protocol driver not attached
    enum ENOCSI             = 50;       // No CSI structure available
    enum EL2HLT             = 51;       // Level 2 halted
    enum EBADE              = 52;       // Invalid exchange
    enum EBADR              = 53;       // Invalid request descriptor
    enum EXFULL             = 54;       // Exchange full
    enum ENOANO             = 55;       // No anode
    enum EBADRQC            = 56;       // Invalid request code
    enum EBADSLT            = 57;       // Invalid slot
    enum EDEADLOCK          = EDEADLK;
    enum EBFONT             = 59;       // Bad font file format
    enum ENOSTR             = 60;       // Device not a stream
    enum ENODATA            = 61;       // No data available
    enum ETIME              = 62;       // Timer expired
    enum ENOSR              = 63;       // Out of streams resources
    enum ENONET             = 64;       // Machine is not on the network
    enum ENOPKG             = 65;       // Package not installed
    enum EREMOTE            = 66;       // Object is remote
    enum ENOLINK            = 67;       // Link has been severed
    enum EADV               = 68;       // Advertise error
    enum ESRMNT             = 69;       // Srmount error
    enum ECOMM              = 70;       // Communication error on send
    enum EPROTO             = 71;       // Protocol error
    enum EMULTIHOP          = 72;       // Multihop attempted
    enum EDOTDOT            = 73;       // RFS specific error
    enum EBADMSG            = 74;       // Not a data message
    enum EOVERFLOW          = 75;       // Value too large for defined data type
    enum ENOTUNIQ           = 76;       // Name not unique on network
    enum EBADFD             = 77;       // File descriptor in bad state
    enum EREMCHG            = 78;       // Remote address changed
    enum ELIBACC            = 79;       // Can not access a needed shared library
    enum ELIBBAD            = 80;       // Accessing a corrupted shared library
    enum ELIBSCN            = 81;       // .lib section in a.out corrupted
    enum ELIBMAX            = 82;       // Attempting to link in too many shared libraries
    enum ELIBEXEC           = 83;       // Cannot exec a shared library directly
    enum EILSEQ             = 84;       // Illegal byte sequence
    enum ERESTART           = 85;       // Interrupted system call should be restarted
    enum ESTRPIPE           = 86;       // Streams pipe error
    enum EUSERS             = 87;       // Too many users
    enum ENOTSOCK           = 88;       // Socket operation on non-socket
    enum EDESTADDRREQ       = 89;       // Destination address required
    enum EMSGSIZE           = 90;       // Message too long
    enum EPROTOTYPE         = 91;       // Protocol wrong type for socket
    enum ENOPROTOOPT        = 92;       // Protocol not available
    enum EPROTONOSUPPORT    = 93;       // Protocol not supported
    enum ESOCKTNOSUPPORT    = 94;       // Socket type not supported
    enum EOPNOTSUPP         = 95;       // Operation not supported on transport endpoint
    enum EPFNOSUPPORT       = 96;       // Protocol family not supported
    enum EAFNOSUPPORT       = 97;       // Address family not supported by protocol
    enum EADDRINUSE         = 98;       // Address already in use
    enum EADDRNOTAVAIL      = 99;       // Cannot assign requested address
    enum ENETDOWN           = 100;      // Network is down
    enum ENETUNREACH        = 101;      // Network is unreachable
    enum ENETRESET          = 102;      // Network dropped connection because of reset
    enum ECONNABORTED       = 103;      // Software caused connection abort
    enum ECONNRESET         = 104;      // Connection reset by peer
    enum ENOBUFS            = 105;      // No buffer space available
    enum EISCONN            = 106;      // Transport endpoint is already connected
    enum ENOTCONN           = 107;      // Transport endpoint is not connected
    enum ESHUTDOWN          = 108;      // Cannot send after transport endpoint shutdown
    enum ETOOMANYREFS       = 109;      // Too many references: cannot splice
    enum ETIMEDOUT          = 110;      // Connection timed out
    enum ECONNREFUSED       = 111;      // Connection refused
    enum EHOSTDOWN          = 112;      // Host is down
    enum EHOSTUNREACH       = 113;      // No route to host
    enum EALREADY           = 114;      // Operation already in progress
    enum EINPROGRESS        = 115;      // Operation now in progress
    enum ESTALE             = 116;      // Stale NFS file handle
    enum EUCLEAN            = 117;      // Structure needs cleaning
    enum ENOTNAM            = 118;      // Not a XENIX named type file
    enum ENAVAIL            = 119;      // No XENIX semaphores available
    enum EISNAM             = 120;      // Is a named type file
    enum EREMOTEIO          = 121;      // Remote I/O error
    enum EDQUOT             = 122;      // Quota exceeded
    enum ENOMEDIUM          = 123;      // No medium found
    enum EMEDIUMTYPE        = 124;      // Wrong medium type
    enum ECANCELED          = 125;      // Operation Canceled
    enum ENOKEY             = 126;      // Required key not available
    enum EKEYEXPIRED        = 127;      // Key has expired
    enum EKEYREVOKED        = 128;      // Key has been revoked
    enum EKEYREJECTED       = 129;      // Key was rejected by service
    enum EOWNERDEAD         = 130;      // Owner died
    enum ENOTRECOVERABLE    = 131;      // State not recoverable
}
