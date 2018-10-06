/*
 * hold.h: header for hold.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: hold.h,v 1.1.1.1 1999/07/16 21:21:13 toast Exp $
 */

#ifndef _HOLD_H_
#define _HOLD_H_

/* Hold: your general doubly-linked list type structure */

typedef struct HoldStru
{
	char	*str;
	struct	HoldStru	*next;
	struct	HoldStru	*prev;
	int	logged;
}	Hold;

extern	void	remove_from_hold_list();
extern	void	add_to_hold_list();
extern	void	hold_mode();
extern	int	hold_output();
extern	char	*hold_queue();
extern	void	reset_hold();
extern	int	hold_queue_logged();

#endif /* _HOLD_H_ */
