/*
 * input.h: header for input.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: input.h,v 1.2 2001/11/29 23:58:43 toast Exp $
 */

#ifndef _INPUT_H_
#define _INPUT_H_

extern	char	input_pause();
extern	void	set_input();
extern	void	set_input_prompt();
extern	char	*get_input_prompt();
extern	char	*get_input();
extern	void	update_input();
extern	void	init_input();
extern	void	input_move_cursor();
extern	void	change_input_prompt();
extern	void	cursor_to_input();
extern	void	input_add_character(char, char *);

/* used by update_input */
#define NO_UPDATE 0
#define UPDATE_ALL 1
#define UPDATE_FROM_CURSOR 2
#define UPDATE_JUST_CURSOR 3

#endif /* _INPUT_H_ */
