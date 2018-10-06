/*
 * output.h: header for output.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: output.h,v 1.1.1.1 1999/07/16 21:21:14 toast Exp $
 */

#ifndef _OUTPUT_H_
#define _OUTPUT_H_

# ifdef USE_STDARG_H
extern	void	put_it(char *, ...);
extern	void	send_to_server(char *, ...);
extern	void	say(char *, ...);
extern	void	yell(char *, ...);
extern	void	help_put_it(char *, char *, ...);
# else
extern	void	put_it();
extern	void	send_to_server();
extern	void	say();
extern	void	yell();
extern	void	help_put_it();
# endif

extern	RETSIGTYPE	refresh_screen();
extern	void	init_screen();
extern	void	set_continued_line();
extern	FILE	*irclog_fp;
extern	void	put_file();

#endif /* _OUTPUT_H_ */
