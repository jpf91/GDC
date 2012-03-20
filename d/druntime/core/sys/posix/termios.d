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
module core.sys.posix.termios;

private import core.sys.posix.config;
public import core.sys.posix.sys.types; // for pid_t

extern (C):

//
// Required
//
/*
cc_t
speed_t
tcflag_t

NCCS

struct termios
{
    tcflag_t   c_iflag;
    tcflag_t   c_oflag;
    tcflag_t   c_cflag;
    tcflag_t   c_lflag;
    cc_t[NCCS] c_cc;
}

VEOF
VEOL
VERASE
VINTR
VKILL
VMIN
VQUIT
VSTART
VSTOP
VSUSP
VTIME

BRKINT
ICRNL
IGNBRK
IGNCR
IGNPAR
INLCR
INPCK
ISTRIP
IXOFF
IXON
PARMRK

OPOST

B0
B50
B75
B110
B134
B150
B200
B300
B600
B1200
B1800
B2400
B4800
B9600
B19200
B38400

CSIZE
    CS5
    CS6
    CS7
    CS8
CSTOPB
CREAD
PARENB
PARODD
HUPCL
CLOCAL

ECHO
ECHOE
ECHOK
ECHONL
ICANON
IEXTEN
ISIG
NOFLSH
TOSTOP

TCSANOW
TCSADRAIN
TCSAFLUSH

TCIFLUSH
TCIOFLUSH
TCOFLUSH

TCIOFF
TCION
TCOOFF
TCOON

speed_t cfgetispeed(in termios*);
speed_t cfgetospeed(in termios*);
int     cfsetispeed(termios*, speed_t);
int     cfsetospeed(termios*, speed_t);
int     tcdrain(int);
int     tcflow(int, int);
int     tcflush(int, int);
int     tcgetattr(int, termios*);
int     tcsendbreak(int, int);
int     tcsetattr(int, int, in termios*);
*/


alias ubyte cc_t;
alias uint  speed_t;
alias uint  tcflag_t;

enum NCCS   = 19;

struct termios
{
    tcflag_t   c_iflag;
    tcflag_t   c_oflag;
    tcflag_t   c_cflag;
    tcflag_t   c_lflag;
    cc_t       c_line;
    cc_t[NCCS] c_cc;
}

enum VEOF       = 4;
enum VEOL       = 11;
enum VERASE     = 2;
enum VINTR      = 0;
enum VKILL      = 3;
enum VMIN       = 6;
enum VQUIT      = 1;
enum VSTART     = 8;
enum VSTOP      = 9;
enum VSUSP      = 10;
enum VTIME      = 5;

enum BRKINT     = 0000002;
enum ICRNL      = 0000400;
enum IGNBRK     = 0000001;
enum IGNCR      = 0000200;
enum IGNPAR     = 0000004;
enum INLCR      = 0000100;
enum INPCK      = 0000020;
enum ISTRIP     = 0000040;
enum IXOFF      = 0010000;
enum IXON       = 0002000;
enum PARMRK     = 0000010;

enum OPOST      = 0000001;

enum B0         = 0000000;
enum B50        = 0000001;
enum B75        = 0000002;
enum B110       = 0000003;
enum B134       = 0000004;
enum B150       = 0000005;
enum B200       = 0000006;
enum B300       = 0000007;
enum B600       = 0000010;
enum B1200      = 0000011;
enum B1800      = 0000012;
enum B2400      = 0000013;
enum B4800      = 0000014;
enum B9600      = 0000015;
enum B19200     = 0000016;
enum B38400     = 0000017;

enum CSIZE      = 0000060;
enum   CS5      = 0000000;
enum   CS6      = 0000020;
enum   CS7      = 0000040;
enum   CS8      = 0000060;
enum CSTOPB     = 0000100;
enum CREAD      = 0000200;
enum PARENB     = 0000400;
enum PARODD     = 0001000;
enum HUPCL      = 0002000;
enum CLOCAL     = 0004000;

enum ECHO       = 0000010;
enum ECHOE      = 0000020;
enum ECHOK      = 0000040;
enum ECHONL     = 0000100;
enum ICANON     = 0000002;
enum IEXTEN     = 0100000;
enum ISIG       = 0000001;
enum NOFLSH     = 0000200;
enum TOSTOP     = 0000400;

enum TCSANOW    = 0;
enum TCSADRAIN  = 1;
enum TCSAFLUSH  = 2;

enum TCIFLUSH   = 0;
enum TCOFLUSH   = 1;
enum TCIOFLUSH  = 2;

enum TCIOFF     = 2;
enum TCION      = 3;
enum TCOOFF     = 0;
enum TCOON      = 1;

speed_t cfgetispeed(in termios*);
speed_t cfgetospeed(in termios*);
int     cfsetispeed(termios*, speed_t);
int     cfsetospeed(termios*, speed_t);
int     tcflow(int, int);
int     tcflush(int, int);
int     tcgetattr(int, termios*);
int     tcsendbreak(int, int);
int     tcsetattr(int, int, in termios*);


//
// XOpen (XSI)
//
/*
IXANY

ONLCR
OCRNL
ONOCR
ONLRET
OFILL
NLDLY
    NL0
    NL1
CRDLY
    CR0
    CR1
    CR2
    CR3
TABDLY
    TAB0
    TAB1
    TAB2
    TAB3
BSDLY
    BS0
    BS1
VTDLY
    VT0
    VT1
FFDLY
    FF0
    FF1

pid_t   tcgetsid(int);
*/

enum IXANY      = 0004000;

enum ONLCR      = 0000004;
enum OCRNL      = 0000010;
enum ONOCR      = 0000020;
enum ONLRET     = 0000040;
enum OFILL      = 0000100;
enum NLDLY      = 0000400;
enum   NL0      = 0000000;
enum   NL1      = 0000400;
enum CRDLY      = 0003000;
enum   CR0      = 0000000;
enum   CR1      = 0001000;
enum   CR2      = 0002000;
enum   CR3      = 0003000;
enum TABDLY     = 0014000;
enum   TAB0     = 0000000;
enum   TAB1     = 0004000;
enum   TAB2     = 0010000;
enum   TAB3     = 0014000;
enum BSDLY      = 0020000;
enum   BS0      = 0000000;
enum   BS1      = 0020000;
enum VTDLY      = 0040000;
enum   VT0      = 0000000;
enum   VT1      = 0040000;
enum FFDLY      = 0100000;
enum   FF0      = 0000000;
enum   FF1      = 0100000;

pid_t   tcgetsid(int);
