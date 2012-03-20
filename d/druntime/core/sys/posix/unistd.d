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
module core.sys.posix.unistd;

private import core.sys.posix.config;
private import core.stdc.stddef;
public import core.sys.posix.inttypes;  // for intptr_t
public import core.sys.posix.sys.types; // for size_t, ssize_t, uid_t, gid_t, off_t, pid_t, useconds_t

extern (C):

enum STDIN_FILENO  = 0;
enum STDOUT_FILENO = 1;
enum STDERR_FILENO = 2;

char*   optarg;
int     optind;
int     opterr;
int     optopt;

int     access(in char*, int);
uint    alarm(uint);
int     chdir(in char*);
int     chown(in char*, uid_t, gid_t);
int     close(int);
int     dup(int);
int     dup2(int, int);
int     execl(in char*, in char*, ...);
int     execle(in char*, in char*, ...);
int     execlp(in char*, in char*, ...);
int     execv(in char*, in char**);
int     execve(in char*, in char**, in char**);
int     execvp(in char*, in char**);
void    _exit(int);
int     fchown(int, uid_t, gid_t);
pid_t   fork();
//int     ftruncate(int, off_t);
char*   getcwd(char*, size_t);
gid_t   getegid();
uid_t   geteuid();
gid_t   getgid();
int     getgroups(int, gid_t *);
int     gethostname(char*, size_t);
char*   getlogin();
int     getopt(int, in char**, in char*);
pid_t   getpgrp();
pid_t   getpid();
pid_t   getppid();
uid_t   getuid();
int     isatty(int);
int     link(in char*, in char*);
//off_t   lseek(int, off_t, int);
c_long  pathconf(in char*, int);
int     pause();
int     pipe(ref int[2]);
ssize_t read(int, void*, size_t);
ssize_t readlink(in char*, char*, size_t);
int     rmdir(in char*);
int     setegid(gid_t);
int     seteuid(uid_t);
int     setgid(gid_t);
int     setpgid(pid_t, pid_t);
pid_t   setsid();
int     setuid(uid_t);
uint    sleep(uint);
int     symlink(in char*, in char*);
c_long  sysconf(int);
pid_t   tcgetpgrp(int);
int     tcsetpgrp(int, pid_t);
char*   ttyname(int);
int     unlink(in char*);
ssize_t write(int, in void*, size_t);

loff_t lseek64(int, loff_t, int);
off_t lseek(int, off_t, int);
int   ftruncate(int, off_t);


enum F_OK       = 0;
enum R_OK       = 4;
enum W_OK       = 2;
enum X_OK       = 1;

//
// File Synchronization (FSC)
//
/*
int fsync(int);
*/

int fsync(int);

//
// Synchronized I/O (SIO)
//
/*
int fdatasync(int);
*/

//
// XOpen (XSI)
//
/*
char*      crypt(in char*, in char*);
char*      ctermid(char*);
void       encrypt(ref char[64], int);
int        fchdir(int);
c_long     gethostid();
pid_t      getpgid(pid_t);
pid_t      getsid(pid_t);
char*      getwd(char*); // LEGACY
int        lchown(in char*, uid_t, gid_t);
int        lockf(int, int, off_t);
int        nice(int);
ssize_t    pread(int, void*, size_t, off_t);
ssize_t    pwrite(int, in void*, size_t, off_t);
pid_t      setpgrp();
int        setregid(gid_t, gid_t);
int        setreuid(uid_t, uid_t);
void       swab(in void*, void*, ssize_t);
void       sync();
int        truncate(in char*, off_t);
useconds_t ualarm(useconds_t, useconds_t);
int        usleep(useconds_t);
pid_t      vfork();
*/


//char*      ctermid(char*);
int        fchdir(int);
int        getpgid(pid_t);
int        lchown(in char*, uid_t, gid_t);
int        nice(int);
ssize_t    pread(int, void*, size_t, off_t);
ssize_t    pwrite(int, in void*, size_t, off_t);
int        setpgrp(pid_t, pid_t);
int        setregid(gid_t, gid_t);
int        setreuid(uid_t, uid_t);
void       sync();
int        truncate(in char*, off_t);
int        usleep(useconds_t);
pid_t      vfork();


//TODO: Have to readd some defines
int _SC_NPROCESSORS_ONLN = 0x0061;
