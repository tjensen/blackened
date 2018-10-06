/*
 * funny.h: header for funny.c
 *
 * written by michael sandrof
 *
 * copyright(c) 1990 
 *
 * see the copyright file, or do a help ircii copyright 
 *
 * @(#)$Id: funny.h,v 1.1.1.1 1999/07/16 21:21:13 toast Exp $
 */

#ifndef _FUNNY_H_
#define _FUNNY_H_

#define FUNNY_PUBLIC 1
#define FUNNY_PRIVATE 2
#define FUNNY_TOPIC  4
#define FUNNY_WIDE   8
#define FUNNY_USERS  16
#define FUNNY_NAME   32

extern	void	set_funny_flags();
extern	void	funny_match();
extern	void	reinstate_user_modes();
extern	void	funny_set_ignore_channel();
extern	void	funny_print_widelist();
extern	void	funny_list();
extern	void	funny_mode();
extern	void	funny_namreply();
extern	int	funny_is_ignore_channel();
extern	void	update_user_mode();
extern	void	funny_set_ignore_mode();

#endif /* _FUNNY_H_ */
