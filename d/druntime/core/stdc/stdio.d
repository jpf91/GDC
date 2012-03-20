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
module core.stdc.stdio;

private
{
    import core.stdc.config;
    import core.stdc.stddef; // for size_t
    import core.stdc.stdarg; // for va_list

    import core.sys.posix.sys.types;
    
    version (GNU)
    {
        import gcc.builtins;
    }
}

extern (C):
nothrow:

version( GNU )
{
    enum
    {
        BUFSIZ       = 1024,
        EOF          = -1,
        FOPEN_MAX    = 20,
        FILENAME_MAX = 1024,
        TMP_MAX      = 308915776,
        L_tmpnam     = 1024
    }
}

enum
{
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
}

struct _iobuf
{
    align (1):
    version( GNU )
    {
        // want to get rid of this...
        /* 81 == sizeof(__sFILE)*/
        byte[81] opaque;
    }
    else
    {
        static assert( false, "Unsupported platform" );
    }
}

alias shared(_iobuf) FILE;

version( linux )
{
    enum
    {
        _IOFBF = 0,
        _IOLBF = 1,
        _IONBF = 2,
    }

    extern(C)
    {
        FILE __sF[3];
    }

    auto stdin  = &__sF[0];
    auto stdout = &__sF[1];
    auto stderr = &__sF[2];
}
else
{
    static assert( false, "Unsupported platform" );
}

alias c_long fpos_t;

int remove(in char* filename);
int rename(in char* from, in char* to);

FILE* tmpfile();
char* tmpnam(char* s);

int   fclose(FILE* stream);
int   fflush(FILE* stream);
FILE* fopen(in char* filename, in char* mode);
FILE* freopen(in char* filename, in char* mode, FILE* stream);

void setbuf(FILE* stream, char* buf);
int  setvbuf(FILE* stream, char* buf, int mode, size_t size);

int fprintf(FILE* stream, in char* format, ...);
int fscanf(FILE* stream, in char* format, ...);
int sprintf(char* s, in char* format, ...);
int sscanf(in char* s, in char* format, ...);
int vfprintf(FILE* stream, in char* format, va_list arg);
int vfscanf(FILE* stream, in char* format, va_list arg);
int vsprintf(char* s, in char* format, va_list arg);
int vsscanf(in char* s, in char* format, va_list arg);
int vprintf(in char* format, va_list arg);
int vscanf(in char* format, va_list arg);
int printf(in char* format, ...);
int scanf(in char* format, ...);

int fgetc(FILE* stream);
int fputc(int c, FILE* stream);

char* fgets(char* s, int n, FILE* stream);
int   fputs(in char* s, FILE* stream);
char* gets(char* s);
int   puts(in char* s);

extern (D)
{
    int getchar()                 { return getc(stdin);     }
    int putchar(int c)            { return putc(c,stdout);  }
    int getc(FILE* stream)        { return fgetc(stream);   }
    int putc(int c, FILE* stream) { return fputc(c,stream); }
}

int ungetc(int c, FILE* stream);

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(in void* ptr, size_t size, size_t nmemb, FILE* stream);

int fgetpos(FILE* stream, fpos_t * pos);
int fsetpos(FILE* stream, in fpos_t* pos);

int    fseek(FILE* stream, c_long offset, int whence);
c_long ftell(FILE* stream);

version( GNU )
{
    void rewind(FILE*);
    void clearerr(FILE*);
    int  feof(FILE*);
    int  ferror(FILE*);
    int  fileno(FILE*);

    alias __builtin_snprintf snprintf;
    alias __builtin_vsnprintf vsnprintf;
}
else
{
    static assert( false, "Unsupported platform" );
}

void perror(in char* s);
