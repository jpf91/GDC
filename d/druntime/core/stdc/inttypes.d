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
module core.stdc.inttypes;

public import core.stdc.stddef; // for wchar_t
public import core.stdc.stdint; // required by spec

extern (C):

struct imaxdiv_t
{
    intmax_t    quot,
                rem;
}

private alias immutable(char)* _cstr;

enum _cstr PRId8            = "d";
enum _cstr PRId16           = "d";
enum _cstr PRId32           = "d";
enum _cstr PRId64           = "lld";

enum _cstr PRIdLEAST8       = "d";
enum _cstr PRIdLEAST16      = "d";
enum _cstr PRIdLEAST32      = "d";
enum _cstr PRIdLEAST64      = "lld";

enum _cstr PRIdFAST8        = "d";
enum _cstr PRIdFAST16       = "d";
enum _cstr PRIdFAST32       = "d";
enum _cstr PRIdFAST64       = "lld";

enum _cstr PRIi8            = "i";
enum _cstr PRIi16           = "i";
enum _cstr PRIi32           = "i";
enum _cstr PRIi64           = "lli";

enum _cstr PRIiLEAST8       = "i";
enum _cstr PRIiLEAST16      = "i";
enum _cstr PRIiLEAST32      = "i";
enum _cstr PRIiLEAST64      = "lli";

enum _cstr PRIiFAST8        = "i";
enum _cstr PRIiFAST16       = "i";
enum _cstr PRIiFAST32       = "i";
enum _cstr PRIiFAST64       = "lli";

enum _cstr PRIo8            = "o";
enum _cstr PRIo16           = "o";
enum _cstr PRIo32           = "o";
enum _cstr PRIo64           = "llo";

enum _cstr PRIoLEAST8       = "o";
enum _cstr PRIoLEAST16      = "o";
enum _cstr PRIoLEAST32      = "o";
enum _cstr PRIoLEAST64      = "llo";

enum _cstr PRIoFAST8        = "o";
enum _cstr PRIoFAST16       = "o";
enum _cstr PRIoFAST32       = "o";
enum _cstr PRIoFAST64       = "llo";

enum _cstr PRIu8            = "u";
enum _cstr PRIu16           = "u";
enum _cstr PRIu32           = "u";
enum _cstr PRIu64           = "llu";

enum _cstr PRIuLEAST8       = "u";
enum _cstr PRIuLEAST16      = "u";
enum _cstr PRIuLEAST32      = "u";
enum _cstr PRIuLEAST64      = "llu";

enum _cstr PRIuFAST8        = "u";
enum _cstr PRIuFAST16       = "u";
enum _cstr PRIuFAST32       = "u";
enum _cstr PRIuFAST64       = "llu";

enum _cstr PRIx8            = "x";
enum _cstr PRIx16           = "x";
enum _cstr PRIx32           = "x";
enum _cstr PRIx64           = "llx";

enum _cstr PRIxLEAST8       = "x";
enum _cstr PRIxLEAST16      = "x";
enum _cstr PRIxLEAST32      = "x";
enum _cstr PRIxLEAST64      = "llx";

enum _cstr PRIxFAST8        = "x";
enum _cstr PRIxFAST16       = "x";
enum _cstr PRIxFAST32       = "x";
enum _cstr PRIxFAST64       = "llx";

enum _cstr PRIX8            = "X";
enum _cstr PRIX16           = "X";
enum _cstr PRIX32           = "X";
enum _cstr PRIX64           = "llX";

enum _cstr PRIXLEAST8       = "X";
enum _cstr PRIXLEAST16      = "X";
enum _cstr PRIXLEAST32      = "X";
enum _cstr PRIXLEAST64      = "llX";

enum _cstr PRIXFAST8        = "X";
enum _cstr PRIXFAST16       = "X";
enum _cstr PRIXFAST32       = "X";
enum _cstr PRIXFAST64       = "llX";

enum _cstr SCNd8            = "hhd";
enum _cstr SCNd16           = "hd";
enum _cstr SCNd32           = "d";
enum _cstr SCNd64           = "lld";

enum _cstr SCNdLEAST8       = "hhd";
enum _cstr SCNdLEAST16      = "hd";
enum _cstr SCNdLEAST32      = "d";
enum _cstr SCNdLEAST64      = "lld";

enum _cstr SCNdFAST8        = "hhd";
enum _cstr SCNdFAST16       = "hd";
enum _cstr SCNdFAST32       = "d";
enum _cstr SCNdFAST64       = "lld";

enum _cstr SCNi8            = "hhi";
enum _cstr SCNi16           = "hi";
enum _cstr SCNi32           = "i";
enum _cstr SCNi64           = "lli";

enum _cstr SCNiLEAST8       = "hhi";
enum _cstr SCNiLEAST16      = "hi";
enum _cstr SCNiLEAST32      = "i";
enum _cstr SCNiLEAST64      = "lli";

enum _cstr SCNiFAST8        = "hhi";
enum _cstr SCNiFAST16       = "hi";
enum _cstr SCNiFAST32       = "i";
enum _cstr SCNiFAST64       = "lli";

enum _cstr SCNo8            = "hho";
enum _cstr SCNo16           = "ho";
enum _cstr SCNo32           = "o";
enum _cstr SCNo64           = "llo";

enum _cstr SCNoLEAST8       = "hho";
enum _cstr SCNoLEAST16      = "ho";
enum _cstr SCNoLEAST32      = "o";
enum _cstr SCNoLEAST64      = "llo";

enum _cstr SCNoFAST8        = "hho";
enum _cstr SCNoFAST16       = "ho";
enum _cstr SCNoFAST32       = "o";
enum _cstr SCNoFAST64       = "llo";

enum _cstr SCNu8            = "hhu";
enum _cstr SCNu16           = "hu";
enum _cstr SCNu32           = "u";
enum _cstr SCNu64           = "llu";

enum _cstr SCNuLEAST8       = "hhu";
enum _cstr SCNuLEAST16      = "hu";
enum _cstr SCNuLEAST32      = "u";
enum _cstr SCNuLEAST64      = "llu";

enum _cstr SCNuFAST8        = "hhu";
enum _cstr SCNuFAST16       = "hu";
enum _cstr SCNuFAST32       = "u";
enum _cstr SCNuFAST64       = "llu";

enum _cstr SCNx8            = "hhx";
enum _cstr SCNx16           = "hx";
enum _cstr SCNx32           = "x";
enum _cstr SCNx64           = "llx";

enum _cstr SCNxLEAST8       = "hhx";
enum _cstr SCNxLEAST16      = "hx";
enum _cstr SCNxLEAST32      = "x";
enum _cstr SCNxLEAST64      = "llx";

enum _cstr SCNxFAST8        = "hhx";
enum _cstr SCNxFAST16       = "hx";
enum _cstr SCNxFAST32       = "x";
enum _cstr SCNxFAST64       = "llx";

version( D_LP64 )
{
    enum _cstr PRIdMAX      = PRId64;
    enum _cstr PRIiMAX      = PRIi64;
    enum _cstr PRIoMAX      = PRIo64;
    enum _cstr PRIuMAX      = PRIu64;
    enum _cstr PRIxMAX      = PRIx64;
    enum _cstr PRIXMAX      = PRIX64;

    enum _cstr SCNdMAX      = SCNd64;
    enum _cstr SCNiMAX      = SCNi64;
    enum _cstr SCNoMAX      = SCNo64;
    enum _cstr SCNuMAX      = SCNu64;
    enum _cstr SCNxMAX      = SCNx64;

    enum _cstr PRIdPTR      = PRId64;
    enum _cstr PRIiPTR      = PRIi64;
    enum _cstr PRIoPTR      = PRIo64;
    enum _cstr PRIuPTR      = PRIu64;
    enum _cstr PRIxPTR      = PRIx64;
    enum _cstr PRIXPTR      = PRIX64;

    enum _cstr SCNdPTR      = SCNd64;
    enum _cstr SCNiPTR      = SCNi64;
    enum _cstr SCNoPTR      = SCNo64;
    enum _cstr SCNuPTR      = SCNu64;
    enum _cstr SCNxPTR      = SCNx64;
}
else
{
    enum _cstr PRIdMAX      = PRId32;
    enum _cstr PRIiMAX      = PRIi32;
    enum _cstr PRIoMAX      = PRIo32;
    enum _cstr PRIuMAX      = PRIu32;
    enum _cstr PRIxMAX      = PRIx32;
    enum _cstr PRIXMAX      = PRIX32;

    enum _cstr SCNdMAX      = SCNd32;
    enum _cstr SCNiMAX      = SCNi32;
    enum _cstr SCNoMAX      = SCNo32;
    enum _cstr SCNuMAX      = SCNu32;
    enum _cstr SCNxMAX      = SCNx32;

    enum _cstr PRIdPTR      = PRId32;
    enum _cstr PRIiPTR      = PRIi32;
    enum _cstr PRIoPTR      = PRIo32;
    enum _cstr PRIuPTR      = PRIu32;
    enum _cstr PRIxPTR      = PRIx32;
    enum _cstr PRIXPTR      = PRIX32;

    enum _cstr SCNdPTR      = SCNd32;
    enum _cstr SCNiPTR      = SCNi32;
    enum _cstr SCNoPTR      = SCNo32;
    enum _cstr SCNuPTR      = SCNu32;
    enum _cstr SCNxPTR      = SCNx32;
}

intmax_t  imaxabs(intmax_t j);
imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom);
intmax_t  strtoimax(in char* nptr, char** endptr, int base);
uintmax_t strtoumax(in char* nptr, char** endptr, int base);
//Not supported
//intmax_t  wcstoimax(in wchar_t* nptr, wchar_t** endptr, int base);
//uintmax_t wcstoumax(in wchar_t* nptr, wchar_t** endptr, int base);
