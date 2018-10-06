/*
 * history.h: header for history.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: history.h,v 1.1.1.1 1999/07/16 21:21:13 toast Exp $
 */

#ifndef _HISTORY_H_
#define _HISTORY_H_

extern	void	set_history_size();
extern	void	set_history_file();
extern	void	add_to_history();
extern	char	*get_from_history();
extern	char	*do_history();
extern	void	history _((char *, char *, char *));

/* used by get_from_history */
#define NEXT 0
#define PREV 1

#endif /* _HISTORY_H_ */
