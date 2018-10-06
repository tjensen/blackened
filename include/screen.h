/*
 * screen.h: header for screen.c
 *
 * written by matthew green.
 *
 * copyright (c) 1993, 1994.
 *
 * see the copyright file, or type help ircii copyright
 *
 * @(#)$Id: screen.h,v 1.1.1.1 1999/07/16 21:21:14 toast Exp $
 */

#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "window.h"

#define WAIT_PROMPT_LINE        0x01
#define WAIT_PROMPT_KEY         0x02

typedef struct PromptStru
{
	char	*prompt;
	char	*data;
	int	type;
	void	(*func)();
	struct	PromptStru	*next;
}	WaitPrompt;


typedef	struct	ScreenStru
{
	int	screennum;
	Window	*current_window;
	unsigned int	last_window_refnum;	/* reference number of the
						 * window that was last
						 * the current_window */
	Window	*window_list;			/* List of all visible
						 * windows */
	Window	*window_list_end;		/* end of visible window
						 * list */
	Window	*cursor_window;			/* Last window to have
						 * something written to it */
	int	visible_windows;		/* total number of windows */
	WindowStack	*window_stack;		/* the windows here */

	int	meta1_hit;			/* if meta1 is hit in this
						 * screen or not */
	int	meta2_hit;			/* above, for meta2 */
	int	meta3_hit;			/* above, for meta3 */
	int	meta4_hit;			/* above, for meta4 */
	int	quote_hit;			/* true if a key bound to
						 * QUOTE_CHARACTER has been
						 * hit. */
	int	digraph_hit;			/* A digraph key has been hit */
	int	inside_menu;			/* what it says. */

	unsigned char	digraph_first;

	struct	ScreenStru *prev;		/* These are the Screen list */
	struct	ScreenStru *next;		/* pointers */

	FILE	*fpin;				/* These are the file pointers */
	int	fdin;				/* and descriptions for the */
	FILE	*fpout;				/* screen's input/output */
	int	fdout;

	char	input_buffer[INPUT_BUFFER_SIZE+1];	/* the input buffer */
	int	buffer_pos;			/* and the positions for the */
	int	buffer_min_pos;			/* screen */

	char	saved_input_buffer[INPUT_BUFFER_SIZE+1];
	int	saved_buffer_pos;
	int	saved_min_buffer_pos;

	WaitPrompt	*promptlist;

	char	*redirect_name;
	char	*redirect_token;
	int	redirect_server;

	char	*tty_name;
	int	co;
	int	li;

	int	alive;
}	Screen;

/* Stuff for the screen/xterm junk */

#define ST_NOTHING      -1
#define ST_SCREEN       0
#define ST_XTERM        1

/* This is here because it happens in so many places */
#define curr_scr_win	current_screen->current_window

extern	void	scrollback_forwards();
extern	void	scrollback_end();
extern	void	clear_window();
extern	void	recalculate_window_positions();
extern	int	output_line();
extern	void	recalculate_windows();
extern	void	scrollback_backwards();
extern	Window	*create_additional_screen();
extern	void	scroll_window();
extern	Window	*new_window();
extern	void	update_all_windows();
extern	void	add_wait_prompt();
extern	void	clear_all_windows();
extern	void	cursor_in_display();
extern	int	is_cursor_in_display();
extern	void	cursor_not_in_display();
extern	void	set_current_screen();
extern	void	redraw_resized();
extern	void	close_all_screen _((void));

extern	Window	*to_window;
extern	Screen	*current_screen;
extern	Screen	*main_screen;
extern	Screen	*last_input_screen;
extern	Screen	*screen_list;

#endif /* _SCREEN_H_ */
