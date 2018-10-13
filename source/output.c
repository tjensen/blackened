/*
 * output.c: handles a variety of tasks dealing with the output from the irc
 * program 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: output.c,v 1.2.2.1 2002/03/11 19:24:04 toast Exp $";
#endif

#include "irc.h"

#include <sys/ioctl.h>

#include "output.h"
#include "vars.h"
#include "input.h"
#include "term.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "hook.h"
#include "ctcp.h"
#include "log.h"
#include "toast.h"

	int	in_help = 0;

/* make this buffer *much* bigger than needed */
static	char	putbuf[BIG_BUFFER_SIZE + 1];

/*
 * refresh_screen: Whenever the REFRESH_SCREEN function is activated, this
 * swoops into effect 
 */
/*ARGSUSED*/
RETSIGTYPE
refresh_screen(key, ptr)
	char	*key;
	void	(*ptr)();
{
	term_clear_screen();
	if (term_resize())
		recalculate_windows();
	else
		redraw_all_windows();
	update_all_windows();
	update_input(UPDATE_ALL);
}

/* init_windows:  */
void
init_screen()
{
	term_init();
	term_clear_screen();
	term_resize();
	new_window();
	recalculate_windows();
	update_all_windows();
	init_input();
	term_move_cursor(0, 0);
}

/* put_file: uses put_it() to display the contents of a file to the display */
void
put_file(filename)
	char	*filename;
{
	FILE	*fp;
	char	line[256];		/* too big?  too small?  who cares? */
	int	len;

	if ((fp = fopen(filename, "r")) != (FILE *) 0)
	{
		while (fgets(line, 256, fp))
		{
			len = strlen(line);
			if (*(line + len - 1) == '\n')
				*(line + len - 1) = (char) 0;
			put_it("%s", line);
		}
		fclose(fp);
	}
}

/*
 * put_it: the irc display routine.  Use this routine to display anything to
 * the main irc window.  It handles sending text to the display or stdout as
 * needed, add stuff to the lastlog and log file, etc.  Things NOT to do:
 * Dont send any text that contains \n, very unpredictable.  Tabs will also
 * screw things up.  The calling routing is responsible for not overwriting
 * the 1K buffer allocated.  
 *
 * For Ultrix machines, you can't call put_it() with floating point arguements.
 * It just doesn't work.  - phone, jan 1993.
 */
/*VARARGS*/
void
#ifdef USE_STDARG_H
put_it(char *format, ...)
{				/* } */
	va_list vl;
#else
put_it(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
	char	*format;
	char	*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9,
		*arg10;
{
#endif
	if (window_display)
	{
#ifdef USE_STDARG_H
		va_start(vl, format);
		vsprintf(putbuf, format, vl);
		va_end(vl);
#else
		sprintf(putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif
		add_to_log(irclog_fp, putbuf);
		add_to_screen(putbuf);
		add_to_urllog(putbuf);
	}
}

/* This is an alternative form of put_it which writes three asterisks
 * before actually putting things out.
 */
void
#ifdef USE_STDARG_H
say(char *format, ...)
{				/* } */
	va_list vl;
#else
say(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
	char	*format;
	char	*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9,
		*arg10;
{
#endif
	char	*foo = get_string_var(BANNER_VAR);
	int	start = 0;
	if (window_display)
	{
		if (foo)
		{
			strcpy(putbuf, foo);
			start = strlen(foo);
		}
#ifdef USE_STDARG_H
		va_start(vl, format);
		vsprintf(&putbuf[start], format, vl);
		va_end(vl);
#else
		sprintf(&putbuf[start], format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif
		add_to_log(irclog_fp, putbuf);
		add_to_screen(putbuf);
	}
}

void
#ifdef USE_STDARG_H
yell(char *format, ...)
{				/* } */
	va_list vl;
#else
yell(format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
	char	*format;
	char	*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9,
		*arg10;
{
#endif
#ifdef USE_STDARG_H
	va_start(vl, format);
	vsprintf(putbuf, format, vl);
	va_end(vl);
#else
	sprintf(putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif
	add_to_log(irclog_fp, putbuf);
	add_to_screen(putbuf);
}


/* help_put_it: works just like put_it, but is specially used by help */
void
#ifdef USE_STDARG_H
help_put_it(char *topic, char *format, ...)
{						/* } */
	va_list vl;
#else
help_put_it(topic, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
	char	*format,
		*topic;
	char	*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9,
		*arg10;
{
#endif
#ifdef USE_STDARG_H
	va_start(vl, format);
	vsprintf(putbuf, format, vl);
	va_end(vl);
#else
	sprintf(putbuf, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
#endif
	in_help = 1;
	if (do_hook(HELP_LIST, "%s %s", topic, putbuf))
	{
		if (window_display)
		{
			add_to_log(irclog_fp, putbuf);
			add_to_screen(putbuf);
		}
	}
	in_help = 0;
}
