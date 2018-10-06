/*
 * if.h: header for if.c
 *
 * copyright(c) 1994 matthew green
 *
 * See the copyright file, or do a help ircii copyright 
 *
 * @(#)$Id: if.h,v 1.2 2001/11/30 00:00:48 toast Exp $
 */

#ifndef __if_h
# define __if_h

extern	char	*next_expr _((char **, char));
extern	void	ifcmd _((char *, char *, char *));
extern	void	whilecmd _((char *, char *, char *));
extern	void	foreach_handler _((char *, char *, char *));
extern	void	foreach _((char *, char *, char *));
extern	void	fe _((char *, char *, char *));
extern	void	forcmd _((char *, char *, char *));
extern	void	fec _((char *, char *, char *));
extern	void	repeatcmd _((char *, char *, char *));
extern	void	switchcmd _((char *, char *, char *));

#endif /* __if_h */
