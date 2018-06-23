/* d-diagnostics.cc -- D frontend interface to gcc diagnostics.
   Copyright (C) 2017-2018 Free Software Foundation, Inc.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"

#include "dfrontend/globals.h"
#include "dfrontend/errors.h"

#include "tree.h"
#include "options.h"
#include "diagnostic.h"

#include "d-tree.h"


/* Rewrite the format string FORMAT to deal with any format extensions not
   supported by pp_format().  The result should be freed by the caller.  */

static char *
expand_format (const char *format)
{
  OutBuffer buf;
  bool inbacktick = false;

  for (const char *p = format; *p; )
    {
      while (*p != '\0' && *p != '%' && *p != '`')
	{
	  buf.writeByte (*p);
	  p++;
	}

      if (*p == '\0')
	break;

      if (*p == '`')
	{
	  /* Text enclosed by `...` are translated as a quoted string.  */
	  if (inbacktick)
	    {
	      buf.writestring ("%>");
	      inbacktick = false;
	    }
	  else
	    {
	      buf.writestring ("%<");
	      inbacktick = true;
	    }
	  p++;
	  continue;
	}

      /* Check the conversion specification for unhandled flags.  */
      buf.writeByte (*p);
      p++;

    Lagain:
      switch (*p)
	{
	case '\0':
	  /* Malformed format string.  */
	  gcc_unreachable ();

	case '0':
	  /* Remove zero padding from format string.  */
	  while (ISDIGIT (*p))
	    p++;
	  goto Lagain;

	case 'X':
	  /* Hex format only supports lower-case.  */
	  buf.writeByte ('x');
	  p++;
	  break;

	default:
	  break;
	}
    }

  gcc_assert (!inbacktick);
  return buf.extractString ();
}

/* Helper routine for all error routines.  Reports a diagnostic specified by
   KIND at the explicit location LOC, where the message FORMAT has not yet
   been translated by the gcc diagnostic routines.  */

static bool ATTRIBUTE_GCC_DIAG(3,0)
d_diagnostic_report_diagnostic (const Loc& loc, int opt, const char *format,
				va_list ap, diagnostic_t kind)
{
  va_list argp;
  va_copy (argp, ap);

  rich_location rich_loc (line_table, get_linemap (loc));
  diagnostic_info diagnostic;
  char *xformat = expand_format (format);

  diagnostic_set_info (&diagnostic, xformat, &argp, &rich_loc, kind);
  if (opt != 0)
    diagnostic.option_index = opt;

  bool ret = diagnostic_report_diagnostic (global_dc, &diagnostic);
  free (xformat);
  va_end (argp);

  return ret;
}

/* Print a hard error message with explicit location LOC, increasing the
   global or gagged error count.  */

void ATTRIBUTE_GCC_DIAG(2,0)
verror (const Loc& loc, const char *format, va_list ap,
	const char *p1, const char *p2, const char *)
{
  if (!global.gag || global.params.showGaggedErrors)
    {
      char *xformat;

      /* Build string and emit.  */
      if (p2 != NULL)
	xformat = xasprintf ("%s %s %s", p1, p2, format);
      else if (p1 != NULL)
	xformat = xasprintf ("%s %s", p1, format);
      else
	xformat = xasprintf ("%s", format);

      d_diagnostic_report_diagnostic (loc, 0, xformat, ap,
				      global.gag ? DK_ANACHRONISM : DK_ERROR);
      free (xformat);
    }

  if (global.gag)
    global.gaggedErrors++;

  global.errors++;
}

/* Print supplementary message about the last error with explicit location LOC.
   This doesn't increase the global error count.  */

void ATTRIBUTE_GCC_DIAG(2,0)
verrorSupplemental (const Loc& loc, const char *format, va_list ap)
{
  if (global.gag && !global.params.showGaggedErrors)
    return;

  d_diagnostic_report_diagnostic (loc, 0, format, ap, DK_NOTE);
}

/* Print a warning message with explicit location LOC, increasing the
   global warning count.  */

void ATTRIBUTE_GCC_DIAG(2,0)
vwarning (const Loc& loc, const char *format, va_list ap)
{
  if (global.params.warnings && !global.gag)
    {
      /* Warnings don't count if gagged.  */
      if (global.params.warnings == 1)
	global.warnings++;

      d_diagnostic_report_diagnostic (loc, 0, format, ap, DK_WARNING);
    }
}

/* Print supplementary message about the last warning with explicit location
   LOC.  This doesn't increase the global warning count.  */

void ATTRIBUTE_GCC_DIAG(2,0)
vwarningSupplemental (const Loc& loc, const char *format, va_list ap)
{
  if (!global.params.warnings || global.gag)
    return;

  d_diagnostic_report_diagnostic (loc, 0, format, ap, DK_NOTE);
}

/* Print a deprecation message with explicit location LOC, increasing the
   global warning or error count depending on how deprecations are treated.  */

void ATTRIBUTE_GCC_DIAG(2,0)
vdeprecation (const Loc& loc, const char *format, va_list ap,
	      const char *p1, const char *p2)
{
  if (global.params.useDeprecated == 0)
    verror (loc, format, ap, p1, p2);
  else if (global.params.useDeprecated == 2 && !global.gag)
    {
      char *xformat;

      /* Build string and emit.  */
      if (p2 != NULL)
	xformat = xasprintf ("%s %s %s", p1, p2, format);
      else if (p1 != NULL)
	xformat = xasprintf ("%s %s", p1, format);
      else
	xformat = xasprintf ("%s", format);

      d_diagnostic_report_diagnostic (loc, OPT_Wdeprecated, xformat, ap,
				      DK_WARNING);
      free (xformat);
    }
}

/* Print supplementary message about the last deprecation with explicit
   location LOC.  This does not increase the global error count.  */

void ATTRIBUTE_GCC_DIAG(2,0)
vdeprecationSupplemental (const Loc& loc, const char *format, va_list ap)
{
  if (global.params.useDeprecated == 0)
    verrorSupplemental (loc, format, ap);
  else if (global.params.useDeprecated == 2 && !global.gag)
    d_diagnostic_report_diagnostic (loc, 0, format, ap, DK_NOTE);
}

/* Call this after printing out fatal error messages to clean up and
   exit the compiler.  */

void
fatal (void)
{
  exit (FATAL_EXIT_CODE);
}
