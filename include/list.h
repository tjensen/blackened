/*
 * list.h: header for list.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: list.h,v 1.1.1.1 1999/07/16 21:21:14 toast Exp $
 */

#ifndef _LIST_H_
#define _LIST_H_

typedef	struct	list_stru
{
	struct	list_stru	*next;
	char	*name;
}	List;

extern	void	add_to_list();
extern	List	*find_in_list();
extern	List	*remove_from_list();
extern	List	*list_lookup();
extern	List	*remove_from_list_ext();
extern	void	add_to_list_ext();
extern	List	*find_in_list_ext();

#define REMOVE_FROM_LIST 1
#define USE_WILDCARDS 1

#endif /* _LIST_H */
