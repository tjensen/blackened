/*
 * window.h: header file for window.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: window.h,v 1.2 2001/11/29 23:58:43 toast Exp $
 */

#ifndef _WINDOW_H_
#define _WINDOW_H_

/*
 * Define this if you want to play with the new window feature, 
 * CREATE, that allows you to start new iscreen or xterm windows
 * connected to the ircII client.
 */
#define	WINDOW_CREATE

#ifdef M_UNIX
#undef WINDOW_CREATE
#endif /* M_UNIX */

/*
 * Define this if you want ircII to scroll after printing a line,
 * like it used to (2.1.5 and back era), not before printing the
 * line.   Its a waste of a line to me, but what ever people want.
 * Thanks to Veggen for telling me what to do for this.
 */
#undef SCROLL_AFTER_DISPLAY

#include "hold.h"
#include "lastlog.h"
#include "edit.h"
#include "menu.h"

/* used by the update flag to determine what needs updating */
#define REDRAW_DISPLAY_FULL 1
#define REDRAW_DISPLAY_FAST 2
#define UPDATE_STATUS 4
#define REDRAW_STATUS 8

struct	WindowMenuTag
{
	Menu	*menu;
	int	lines;
	int	items_per_line;
	int	cursor;
};

typedef	struct	WindowMenuTag	WindowMenu;

/* NickList: structure for the list of nicknames of people on a channel */
typedef struct nick_stru
{
	struct	nick_stru	*next;	/* pointer to next nickname entry */
	char	*nick;			/* nickname of person on channel */
	char	*user;
	char	*host;
	int	chanop;			/* True if the given nick has chanop */
}	NickList;

typedef	struct	DisplayStru
{
	char	*line;
	int	linetype;
	struct	DisplayStru	*next;
}	Display;

#define	LT_UNLOGGED	0
#define	LT_LOGHEAD	1
#define	LT_LOGTAIL	2

struct	ScreenStru;	/* ooh! */

typedef	struct	WindowStru
{
	unsigned int	refnum;		/* the unique reference number,
					 * assigned by IRCII */
	char	*name;			/* window's logical name */
	int	server;			/* server index */
	int	top;			/* The top line of the window, screen
					 * coordinates */
	int	bottom;			/* The botton line of the window, screen
					 * coordinates */
	int	cursor;			/* The cursor position in the window, window
					 * relative coordinates */
	int	line_cnt;		/* counter of number of lines displayed in
					 * window */
	int	scroll;			/* true, window scrolls... false window wraps */
	int	display_size;		/* number of lines of window - menu lines */
	int	old_size;		/* if new_size != display_size,
					 * resize_display is called */
	int	visible;		/* true, window is drawn... false window is
					 * hidden */
	int	update;			/* window needs updating flag */
	unsigned miscflags;		/* Miscellaneous flags. */

	char	*prompt;		/* A prompt string, usually set by EXEC'd process */
	char	*status_line;		/* The status line string */

	Display	*top_of_display,	/* Pointer to first line of display structure */
		*display_ip;		/* Pointer to insertiong point of display
					 * structure */

	char	*current_channel;	/* Window's current channel */
	char	*waiting_channel;	/* The channel that you _want_ to be on
					 * this window .. derived from a real
					 * fix for the phone bug */
	char	*last_current_channel;	/* The previous current channel. Nice
					 * for when you're using ^X to switch
					 * channels */
	char	*bind_channel;		/* the window's bound channel */
	char	*query_nick;		/* User being QUERY'ied in this window */
	NickList	*nicks;		/* List of nicks that will go to window */
	int	window_level;		/* The LEVEL of the window, determines what
					 * messages go to it */

	/* hold stuff */
	int	hold_mode;		/* true, hold mode is on for window...
					 * false it isn't */
	int	hold_on_next_rite;	/* true, the next call to rite() will
					 * activate a hold */
	int	held;			/* true, the window is currently being
					 * held */
	int	last_held;		/* Previous value of hold flag.  Used
					 * for various updating needs */
	Hold	*hold_head,		/* Pointer to first entry in hold
					 * list */
		*hold_tail;		/* Pointer to last entry in hold list */
	int	held_lines;		/* number of lines being held */
	int	scrolled_lines;		/* number of lines scrolled back */
	int	new_scrolled_lines;	/* number of lines since scroll back
					 * keys where pressed */
	WindowMenu	menu;		/* The menu (if any) */

	/* lastlog stuff */
	Lastlog	*lastlog_head;		/* pointer to top of lastlog list */
	Lastlog	*lastlog_tail;		/* pointer to bottom of lastlog list */
	int	lastlog_level;		/* The LASTLOG_LEVEL, determines what
					 * messages go to lastlog */
	int	lastlog_size;		/* Max number of messages for the window
					 * lastlog */

	int	notify_level;		/* the notify level.. */

	char	*logfile;		/* window's logfile name */
	/* window log stuff */
	int	log;			/* true, file logging for window is on */
	FILE	*log_fp;		/* file pointer for the log file */

	struct	ScreenStru	*screen;

	struct	WindowStru	*next;	/* pointer to next entry in window list (null
					 * is end) */
	struct	WindowStru	*prev;	/* pointer to previous entry in window list
					 * (null is end) */
}	Window;

/*
 * WindowStack: The structure for the POP, PUSH, and STACK functions. A
 * simple linked list with window refnums as the data 
 */
typedef	struct	window_stack_stru
{
	unsigned int	refnum;
	struct	window_stack_stru	*next;
}	WindowStack;

extern	Window	*to_window;

extern	Window	*invisible_list;
extern	char	underline;
extern	int	who_level;
extern	char	*who_from;
extern	int	in_window_command;

typedef	struct
{
	int	top;
	int	bottom;
	int	position;
}	ShrinkInfo;

extern	void	set_scroll_lines();
extern	void	set_scroll();
extern	void	reset_line_cnt();
extern	void	set_continued_line();
extern	void	set_underline_video();
extern	int	rite();
extern	void	erase_display();
extern	ShrinkInfo	resize_display();
extern	void	redraw_all_windows();
extern	void	add_to_screen();
extern	void	init_windows();
extern	int	unhold_windows();
extern	Window	*traverse_all_windows();
extern	void	add_to_invisible_list();
extern	void	delete_window _((Window *));

/* var_settings indexes */
#define OFF 0
#define ON 1
#define TOGGLE 2

extern	Window	*add_to_window_list();
extern	void	erase_display();
extern	void	redraw_display();
extern	void	add_to_display();
extern	void	set_scroll();
extern	void	set_menu();
extern	void	set_scroll_lines();
extern	int	display_hold();
extern	void	update_all_status();
extern	void	set_query_nick();
extern	char	*query_nick();
extern	void	set_current_dcc();
extern	char	*current_dcc();
extern	void	update_window_status();
extern	void	window _((char *, char *, char *));
extern	void	redraw_window();
extern	void	redraw_all_windows();
extern	void	next_window();
extern	void	swap_last_window();
extern	void	swap_next_window();
extern	void	previous_window();
extern	void	swap_previous_window();
extern	void	back_window();
extern	void	window_kill_swap();
extern	int	is_current_channel();
extern	void	redraw_all_status();
extern	void	message_to();
extern	void	message_from();
extern	void	unstop_all_windows();
extern	void	set_display();
extern	unsigned int	window_display;

extern	int	is_main_screen();
extern	void	kill_screen();

extern	void	set_prompt_by_refnum();
extern	int	number_of_windows();
extern	void	clear_window_by_refnum();
extern	unsigned int	current_refnum();
extern	Window	*get_window_by_refnum();
extern	char	*get_target_by_refnum();
extern	char	*get_prompt_by_refnum();
extern	char	*set_channel_by_refnum();
extern	char	*get_channel_by_refnum();
extern	void	set_window_server();
extern	Window	*get_window_by_name();
extern	void	window_redirect();
extern	int	get_window_server();
extern	int	message_from_level();
extern	void	restore_message_from();
extern	void	save_message_from();
extern	void	window_check_servers();
extern	void	set_current_window();
extern	void	set_level_by_refnum();
extern	int	is_bound _((char *, int));
extern	void	unbind_channel _((char *, int));
extern	char	*get_bound_channel _((Window *));
extern	void	set_last_current_channel();
extern	int	is_last_current_channel();
extern	int	get_refnum_by_channel();
extern	void	hide_window _((Window *));
extern	void	list_windows _((void));

void	grow_window();
void	revamp_window_levels();

#define WINDOW_NOTIFY	((unsigned) 0x0001)
#define WINDOW_NOTIFIED	((unsigned) 0x0002)

#endif /* _WINDOW_H_ */
