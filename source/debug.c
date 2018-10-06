/*
 * debug.c - generic debug routines.  Copyright (C) 1993, Matthew Green.
 *
 * void debug(int level,char *format, ...);	* the function to call, at
 *						* most 10 arguments to it
 * int setdlevel(int level); 		* set the debug level to level.
 *					* returns old level
 * int getdlevel();			* returns the debug level..
 * int debuglevel;			* the current level of debugging
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: debug.c,v 1.1.1.1 1999/07/16 21:21:42 toast Exp $";
#endif

#include "config.h"		/* This is where DEBUG is defined or not */

#ifdef DEBUG
# include <stdio.h>
# include "debug.h"
# ifdef USE_STDARG_H
#  include <stdarg.h>
# endif

int	debuglevel = 0;

int
setdlevel(level)
	int	level;
{
	int	oldlevel = debuglevel;

	debuglevel = level;
	return oldlevel;
}

int	getdlevel()
{
	return debuglevel;
}

void
#ifdef USE_STDARG_H
debug(int level, char *format, ...)
#else
debug(level, format, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
	int	level;
	char	*format;
	char	*arg0, *arg1, *arg2, *arg3, *arg4,
		*arg5, *arg6, *arg7, *arg8, *arg9;
#endif
{
	char	buffer[2048];
#ifdef USE_STDARG_H
	va_list vlist;

	va_start(vlist, format);
#endif

	if (!debuglevel || level > debuglevel)
		return;

#ifdef USE_STDARG_H
	vfprintf(stderr, format, vlist);
#else
	fprintf(stderr, format, arg0, arg1, arg2, arg3, arg4,
				arg5, arg6, arg7, arg8, arg9);
#endif
	fputc('\n', stderr);
}
#endif /* DEBUG */
