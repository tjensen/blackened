/*
 * window.c: Handles the Main Window stuff for irc.  This includes proper
 * scrolling, saving of screen memory, refreshing, clearing, etc. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: window.c,v 1.3.2.1 2002/03/11 19:24:05 toast Exp $";
#endif

#include "irc.h"

#include "screen.h"
#include "menu.h"
#include "window.h"
#include "vars.h"
#include "server.h"
#include "list.h"
#include "term.h"
#include "names.h"
#include "ircaux.h"
#include "input.h"
#include "status.h"
#include "output.h"
#include "log.h"
#include "hook.h"
#include "dcc.h"
#include "translat.h"

/* value for underline mode, is 0 when on!  -lynx */
char	underline = 1;

/*
 * The following should synthesize MAXINT on any machine with an 8 bit
 * word.
 */
#define	MAXINT (-1&~(1<<(sizeof(int)*8-1)))

Window	*invisible_list = (Window *) 0;
						/* list of hidden windows */
char	*who_from = (char *) 0;	/* nick of person who's message
						 * is being displayed */
int	who_level = LOG_CRAP;/* Log level of message being displayed */
static	char	*save_from = (char *) 0;
static	int	save_level = LOG_CRAP;

int	in_window_command = 0;	/* set to true if we are in window().  This
				 * is used if a put_it() is called within the
				 * window() command.  We make sure all
				 * windows are fully updated before doing the
				 * put_it(). */

extern	char	*redirect_nick;

/*
 * window_display: this controls the display, 1 being ON, 0 being OFF.  The
 * DISPLAY var sets this. 
 */
unsigned int	window_display = 1;

/*
 * status_update_flag: if 1, the status is updated as normal.  If 0, then all
 * status updating is suppressed 
 */
	int	status_update_flag = 1;

/*
 * traverse_all_windows: This will do as the name implies, traverse every
 * window (visible and invisible) and return a pointer to each window on
 * subsequent calls.  If flag points to a non-zero value, then the traversal
 * in started from the beginning again, and flag is set to point to 0.  This
 * returns all visible windows first, then all invisible windows.  It returns
 * null after all windows have been returned.  It should generally be used as
 * follows: 
 *
 * flag = 1; while(tmp = traverse_all_windows(&flag)) { code here } 
 *
 * Major revamp by phone (phone@coombs.anu.edu.au), December 1992.
 */
Window	*
traverse_all_windows(flag)
	int	*flag;
{
	static	Window	*which;
	static	Screen	*screen;
	static	char	visible = 1;
	int	foo = 1;

	/* First call, return the current window basically */
	if (*flag)
	{
		*flag = 0;
		visible = 1;
		if (!screen_list)
			return (Window *) 0;
		screen = screen_list;
		which = screen->window_list;
		if (which)
			return (which);
		else
			foo = 0;
	}

	/*
	 * foo is used to indicate the the current screen has no windows.
	 * This happens when we create a new screen..  either way, we want
	 * to go on to the next screen, so if foo isn't set, then if which
	 * is already null, return it again (this should never happen, if
	 * traverse_all_windows()'s is called properly), else move on to
	 * the next window
	 */
	if (foo)
		if (!which)
			return (Window *) 0;
		else
			which = which->next;

	if (!which)
	{
		while (screen)
		{
			screen = screen->next;
			if (screen && screen->alive)
				break;
		}
		if (screen)
			which = screen->window_list;
	}

	if (which)
		return (which);
	/* 
	 * Got to the end of the visible list..  so we do the invisible list..
	 * Should also mean, that we've got to the end of all the visible
	 * screen..
	 */
	if (visible)
	{
		visible = 0;
		which = invisible_list;
		return (which);
	}
	return ((Window *) 0);
}

#if 0
/*
 * New version of traverse_all_windows that doesn't require that you don't
 * use it non-recusively.  Pass it a NULL pointer initially, and it will 
 * return 0 when it has finished, and also the address of an int, that is
 * initally 1
 */
static	int
new_window_traverse(window, visible)
	Window	**window;
	int	*visible;
{
	Screen	*screen,
		*old_screen;

	if (!*window)
	{
		if (!(screen = screen_list))
		{
			*visible = 0;
			if (!invisible_list)
				return 0;
			*window = invisible_list;
			return 1;
		}
		for (;screen && !screen->alive; screen = screen->next)
			;
		if (!screen)
		{
			*visible = 0;
			if (!invisible_list)
				return 0;
			*window = invisible_list;
			return 1;
		}
	}
	else
	{
		if ((*window)->next)
		{
			*window = (*window)->next;
			return 1;
		}
		if (!*visible)
			return 0;
		for (old_screen = screen = (*window)->screen;
				screen && !screen->alive; screen = screen->next)
			;
		if (!screen)
		{
			*visible = 0;
			if (!invisible_list)
				return 0;
			*window = invisible_list;
			return 1;
		}
		if (screen == old_screen)
			return 0;
	}
	*window = screen->current_window;
	return (*window) ? 1 : 0;
}
#endif


/*
 * set_scroll_lines: called by /SET SCROLL_LINES to check the scroll lines
 * value 
 */
void
set_scroll_lines(size)
	int	size;
{
	if (size == 0)
	{
		set_var_value(SCROLL_VAR, var_settings[0]);
		if (curr_scr_win)
			curr_scr_win->scroll = 0;
	}
	else if (size > curr_scr_win->display_size)
	{
		say("Maximum lines that may be scrolled is %d", 
		    curr_scr_win->display_size);
		set_int_var(SCROLL_LINES_VAR, curr_scr_win->display_size);
	}
}

/*
 * set_scroll: called by /SET SCROLL to make sure the SCROLL_LINES variable
 * is set correctly 
 */
void
set_scroll(value)
	int	value;
{
	int	old_value;

	if (value && (get_int_var(SCROLL_LINES_VAR) == 0))
	{
		put_it("You must set SCROLL_LINES to a positive value first!");
		if (curr_scr_win)
			curr_scr_win->scroll = 0;
	}
	else
	{
		if (curr_scr_win)
		{
			old_value = curr_scr_win->scroll;
			curr_scr_win->scroll = value;
			if (old_value && !value)
				scroll_window(curr_scr_win);
		}
	}
}

/*
 * reset_line_cnt: called by /SET HOLD_MODE to reset the line counter so we
 * always get a held screen after the proper number of lines 
 */
void
reset_line_cnt(value)
	int	value;
{
	curr_scr_win->hold_mode = value;
	curr_scr_win->hold_on_next_rite = 0;
	curr_scr_win->line_cnt = 0;
}

/*
 * set_continued_line: checks the value of CONTINUED_LINE for validity,
 * altering it if its no good 
 */
void
set_continued_line(value)
	char	*value;
{
	if (value && (strlen(value) > (CO / 2)))
		value[CO / 2] = '\0';
}

/*
 * free_hold: This frees all the data and structures associated with the hold
 * list for the given window 
 */
static	void
free_hold(window)
	Window	*window;
{
	Hold *tmp,
	    *next;

	for (tmp = window->hold_head; tmp; tmp = next)
	{
		next = tmp->next;
		new_free(&(tmp->str));
		new_free(&tmp);
	}
}

/*
 * free_lastlog: This frees all data and structures associated with the
 * lastlog of the given window 
 */
static	void
free_lastlog(window)
	Window	*window;
{
	Lastlog *tmp,
	    *next;

	for (tmp = window->lastlog_head; tmp; tmp = next)
	{
		next = tmp->next;
		new_free(&(tmp->msg));
		new_free(&tmp);
	}
}

/*
 * free_display: This frees all memory for the display list for a given
 * window.  It resets all of the structures related to the display list
 * appropriately as well 
 */
static	void
free_display(window)
	Window	*window;
{
	Display *tmp,
	    *next;
	int	i;

	if (window == (Window *) 0)
		window = curr_scr_win;
	for (tmp = window->top_of_display, i = 0; i < window->display_size; i++, tmp = next)
	{
		next = tmp->next;
		new_free(&(tmp->line));
		new_free(&tmp);
	}
	window->top_of_display = (Display *) 0;
	window->display_ip = (Display *) 0;
	window->display_size = 0;
}

static	void
free_nicks(window)
	Window	*window;
{
	NickList *tmp,
	    *next;

	for (tmp = window->nicks; tmp; tmp = next)
	{
		next = tmp->next;
		new_free(&(tmp->nick));
		new_free(&tmp);
	}
}

/*
 * erase_display: This effectively causes all members of the display list for
 * a window to be set to empty strings, thus "clearing" a window.  It sets
 * the cursor to the top of the window, and the display insertion point to
 * the top of the display. Note, this doesn't actually refresh the screen,
 * just cleans out the display list 
 */
void
erase_display(window)
	Window	*window;
{
	int	i;
	Display *tmp;

	if (dumb)
		return;
	if (window == (Window *) 0)
		window = curr_scr_win;
	for (tmp = window->top_of_display, i = 0; i < window->display_size;
			i++, tmp = tmp->next)
		new_free(&(tmp->line));
	window->cursor = 0;
	window->line_cnt = 0;
	window->hold_on_next_rite = 0;
	window->display_ip = window->top_of_display;
}

static	void
remove_from_invisible_list(window)
	Window	*window;
{
	window->visible = 1;
	window->screen = current_screen;
	window->miscflags &= ~WINDOW_NOTIFIED;
	if (window->prev)
		window->prev->next = window->next;
	else
		invisible_list = window->next;
	if (window->next)
		window->next->prev = window->prev;
}

extern	void
add_to_invisible_list(window)
	Window	*window;
{
	if ((window->next = invisible_list) != NULL)
		invisible_list->prev = window;
	invisible_list = window;
	window->prev = (Window *) 0;
	window->visible = 0;
	window->screen = (Screen *) 0;
}

/*
 * swap_window: This swaps the given window with the current window.  The
 * window passed must be invisible.  Swapping retains the positions of both
 * windows in their respective window lists, and retains the dimensions of
 * the windows as well 
 */
static	void
swap_window(v_window, window)
	Window	*v_window;
	Window	*window;
{
	Window tmp, *prev, *next;
	int	top, bottom, size;

	if (window->visible || !v_window->visible)
	{
		say("You can only SWAP a hidden window with a visible window.");
		return;
	}
	prev = v_window->prev;
	next = v_window->next;

	current_screen->last_window_refnum = v_window->refnum;
	current_screen->last_window_refnum = v_window->refnum;
	remove_from_invisible_list(window);

	tmp = *v_window;
	*v_window = *window;
	v_window->top = tmp.top;
	v_window->bottom = tmp.bottom;
	v_window->display_size = tmp.display_size + tmp.menu.lines -
#ifdef SCROLL_AFTER_DISPLAY
		v_window->menu.lines - 1;
#else
		v_window->menu.lines;
#endif /* SCROLL_AFTER_DISPLAY */
	v_window->prev = prev;
	v_window->next = next;

	top = window->top;
	bottom = window->bottom;
	size = window->display_size;
	*window = tmp;
	window->top = top;
	window->bottom = bottom;
#ifdef SCROLL_AFTER_DISPLAY
	window->display_size = size - 1;
#else
	window->display_size = size;
#endif /* SCROLL_AFTER_DISPLAY */

	add_to_invisible_list(window);

	v_window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
}

/*
 * move_window: This moves a window offset positions in the window list. This
 * means, of course, that the window will move on the screen as well 
 */
static	void
move_window(window, offset)
	Window	*window;
	int	offset;
{
	Window	*tmp,
	    *last;
	int	win_pos,
	pos;

	if (offset == 0)
		return;
	last = (Window *) 0;
	for (win_pos = 0, tmp = current_screen->window_list; tmp;
	    tmp = tmp->next, win_pos++)
	{
		if (window == tmp)
			break;
		last = tmp;
	}
	if (tmp == (Window *) 0)
		return;
	if (last == (Window *) 0)
		current_screen->window_list = tmp->next;
	else
		last->next = tmp->next;
	if (tmp->next)
		tmp->next->prev = last;
	else
		current_screen->window_list_end = last;
	if (offset < 0)
		win_pos = (current_screen->visible_windows + offset + win_pos) %
		    current_screen->visible_windows;
	else
		win_pos = (offset + win_pos) % current_screen->visible_windows;
	last = (Window *) 0;
	for (pos = 0, tmp = current_screen->window_list;
	    pos != win_pos; tmp = tmp->next, pos++)
		last = tmp;
	if (last == (Window *) 0)
		current_screen->window_list = window;
	else
		last->next = window;
	if (tmp)
		tmp->prev = window;
	else
		current_screen->window_list_end = window;
	window->prev = last;
	window->next = tmp;
	recalculate_window_positions();
}

/*
 * grow_window: This will increase or descrease the size of the given window
 * by offset lines (positive offset increases, negative decreases).
 * Obviously, with a fixed terminal size, this means that some other window
 * is going to have to change size as well.  Normally, this is the next
 * window in the window list (the window below the one being changed) unless
 * the window is the last in the window list, then the previous window is
 * changed as well 
 */
void
grow_window(window, offset)
	Window	*window;
	int	offset;
{
	Window	*other,
	    *tmp;
	int	after,
	window_size,
	other_size;

	if (window == (Window *) 0)
		window = curr_scr_win;
	if (!window->visible)
	{
		say("You cannot change the size of hidden windows!");
		return;
	}
	if (window->next)
	{
		other = window->next;
		after = 1;
	}
	else
	{
		other = (Window *) 0;
		for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
		{
			if (tmp == window)
				break;
			other = tmp;
		}
		if (other == (Window *) 0)
		{
			say("Can't change the size of this window!");
			return;
		}
		after = 0;
	}
	window_size = window->display_size + offset;
	other_size = other->display_size - offset;
	if ((window_size < 3) ||
	    (other_size < 3))
	{
		say("Not enough room to resize this window!");
		return;
	}
	if (after)
	{
		window->bottom += offset;
		other->top += offset;
	}
	else
	{
		window->top -= offset;
		other->bottom -= offset;
	}
#ifdef SCROLL_AFTER_DISPLAY
	window->display_size = window_size - 1;
	other->display_size = other_size - 1;
#else
	window->display_size = window_size;
	other->display_size = other_size;
#endif /* SCROLL_AFTER_DISPLAY */
	window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	other->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	term_flush();
}


/*
 * save_message_from: this is used to save (for later restoration) the
 * who_from variable.  This comes in handy very often when a routine might
 * call another routine that might change who_from. 
 */
void
save_message_from()
{
	malloc_strcpy(&save_from, who_from);
	save_level = who_level;
}

/* restore_message_from: restores a previously saved who_from variable */
void
restore_message_from()
{
	malloc_strcpy(&who_from, save_from);
	who_level = save_level;
}

/*
 * message_from: With this you can the who_from variable and the who_level
 * variable, used by the display routines to decide which window messages
 * should go to.  
 */
void
message_from(who, level)
	char	*who;
	int	level;
{
	malloc_strcpy(&who_from, who);
	who_level = level;
}

/*
 * message_from_level: Like set_lastlog_msg_level, except for message_from.
 * this is needed by XECHO, because we could want to output things in more
 * than one level.
 */
int
message_from_level(level)
	int	level;
{
	int	temp;

	temp = who_level;
	who_level = level;
	return temp;
}

/*
 * get_window_by_refnum: Given a reference number to a window, this returns a
 * pointer to that window if a window exists with that refnum, null is
 * returned otherwise.  The "safe" way to reference a window is throught the
 * refnum, since a window might be delete behind your back and and Window
 * pointers might become invalid.  
 */
Window	*
get_window_by_refnum(refnum)
	u_int	refnum;
{
	Window	*tmp;
	int	flag = 1;

	if (refnum)
	{
		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (tmp->refnum == refnum)
				return (tmp);
		}
	}
	else
		return (curr_scr_win);
	return ((Window *) 0);
}

/*
 * clear_window_by_refnum: just like clear_window(), but it uses a refnum. If
 * the refnum is invalid, the current window is cleared. 
 */
void
clear_window_by_refnum(refnum)
	u_int	refnum;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	clear_window(tmp);
}

/*
 * revamp_window_levels: Given a level setting for the current window, this
 * makes sure that that level setting is unused by any other window. Thus
 * only one window in the system can be set to a given level.  This only
 * revamps levels for windows with servers matching the given window 
 * it also makes sure that only one window has the level `DCC', as this is
 * not dependant on a server.
 */
void
revamp_window_levels(window)
	Window	*window;
{
	Window	*tmp;
	int	flag = 1;
	int	got_dcc;

	got_dcc = (LOG_DCC & window->window_level) ? 1 : 0;
	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (tmp == window)
			continue;
		if (LOG_DCC & tmp->window_level)
		{
			if (0 != got_dcc)
				tmp->window_level &= ~LOG_DCC;
			got_dcc = 1;
		}
		if (window->server == tmp->server)
			tmp->window_level ^= (tmp->window_level & window->window_level);
	}
}

/*
 * set_level_by_refnum: This sets the window level given a refnum.  It
 * revamps the windows levels as well using revamp_window_levels() 
 */
void
set_level_by_refnum(refnum, level)
	u_int	refnum;
	int	level;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	tmp->window_level = level;
	revamp_window_levels(tmp);
}

/*
 * set_prompt_by_refnum: changes the prompt for the given window.  A window
 * prompt will be used as the target in place of the query user or current
 * channel if it is set 
 */
void
set_prompt_by_refnum(refnum, prompt)
	u_int	refnum;
	char	*prompt;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	malloc_strcpy(&(tmp->prompt), prompt);
}

/*
 * message_to: This allows you to specify a window (by refnum) as a
 * destination for messages.  Used by EXEC routines quite nicely 
 */
void
message_to(refnum)
	u_int	refnum;
{
	if (refnum)
		to_window = get_window_by_refnum(refnum);
	else
		to_window = (Window *) NULL;
}

/*
 * get_next_window: This returns a pointer to the next *visible* window in
 * the window list.  It automatically wraps at the end of the list back to
 * the beginning of the list 
 */
static	Window	*
get_next_window()
{
	if (curr_scr_win && curr_scr_win->next)
		return (curr_scr_win->next);
	else
		return (current_screen->window_list);
}

/*
 * get_previous_window: this returns the previous *visible* window in the
 * window list.  This automatically wraps to the last window in the window
 * list 
 */
static	Window	*
get_previous_window()
{
	if (curr_scr_win && curr_scr_win->prev)
		return (curr_scr_win->prev);
	else
		return (current_screen->window_list_end);
}

/*
 * set_current_window: This sets the "current" window to window.  It also
 * keeps track of the last_current_screen->current_window by setting it to the
 * previous current window.  This assures you that the new current window is
 * visible.
 * If not, a new current window is chosen from the window list 
 */
void
set_current_window(window)
	Window	*window;
{
	Window	*tmp;
	unsigned int	refnum;

	refnum = current_screen->last_window_refnum;
	if (curr_scr_win)

	{
		curr_scr_win->update |= UPDATE_STATUS;
		current_screen->last_window_refnum = curr_scr_win->refnum;
	}
	if ((window == (Window *) 0) || (!window->visible))

	{
		if ((tmp = get_window_by_refnum(refnum)) && (tmp->visible))
			curr_scr_win = tmp;
		else
			curr_scr_win = get_next_window();
	}
	else
		curr_scr_win = window;
	curr_scr_win->update |= UPDATE_STATUS;
}

/*
 * swap_last_window:  This swaps the current window with the last window
 * that was hidden.
 */

void
swap_last_window()
{
	if (invisible_list == (Window *) 0)
	{
		/* say("There are no hidden windows"); */
		/* Not sure if we need to warn   - phone. */
		return;
	}
	swap_window(curr_scr_win, invisible_list);
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	cursor_to_input();
}

/*
 * next_window: This switches the current window to the next visible window 
 */
void
next_window()
{
	if (current_screen->visible_windows == 1)
		return;
	set_current_window(get_next_window());
	update_all_windows();
}

/*
 * swap_next_window:  This swaps the current window with the next hidden
 * window.
 */

void
swap_next_window()
{
	int	flag;
	Window	*tmp;
	int	next = MAXINT;
	int	smallest;

	if (invisible_list == (Window *) 0)
	{
		say("There are no hidden windows");
		return;
	}
	flag = 1;
	smallest = curr_scr_win->refnum;
	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (!tmp->visible)
		{
			if (tmp->refnum < smallest)
				smallest = tmp->refnum;
			if ((tmp->refnum > curr_scr_win->refnum)
			    && (next > tmp->refnum))
				next = tmp->refnum;
		}
	}
	if (next != MAXINT)
		tmp = get_window_by_refnum(next);
	else
		tmp = get_window_by_refnum(smallest);
	swap_window(curr_scr_win, tmp);
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	update_all_status();
	cursor_to_input();
}

/*
 * previous_window: This switches the current window to the previous visible
 * window 
 */
void
previous_window()
{
	if (current_screen->visible_windows == 1)
		return;
	set_current_window(get_previous_window());
	update_all_windows();
}

/*
 * swap_previous_window:  This swaps the current window with the next 
 * hidden window.
 */

void
swap_previous_window()
{
	int	flag;
	Window	*tmp;
	int	previous = 0;
	int	largest;

	if (invisible_list == (Window *) 0)
	{
		say("There are no hidden windows");
		return;
	}
	flag = 1;
	largest = curr_scr_win->refnum;
	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (!tmp->visible)
		{
			if (tmp->refnum > largest)
				largest = tmp->refnum;
			if ((tmp->refnum < curr_scr_win->refnum)
			    && (previous < tmp->refnum))
				previous = tmp->refnum;
		}
	}
	if (previous)
		tmp = get_window_by_refnum(previous);
	else
		tmp = get_window_by_refnum(largest);
	swap_window(curr_scr_win,tmp);
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	update_all_status();
	cursor_to_input();
}

/*
 * back_window:  goes to the last window that was current.  Swapping the
 * current window if the last window was hidden.
 */

void
back_window()
{
	Window	*tmp;

	tmp = get_window_by_refnum(current_screen->last_window_refnum);
	if (tmp->visible)
		set_current_window(tmp);
	else
	{
		swap_window(curr_scr_win, tmp);
		message_from((char *) 0, LOG_CRAP);
		update_all_windows();
		update_all_status();
		cursor_to_input();
	}
}

/*
 * add_to_window_list: This inserts the given window into the visible window
 * list (and thus adds it to the displayed windows on the screen).  The
 * window is added by splitting the current window.  If the current window is
 * too small, the next largest window is used.  The added window is returned
 * as the function value or null is returned if the window couldn't be added 
 */
extern	Window	*
add_to_window_list(new)
	Window	*new;
{
	Window	*biggest = (Window *) 0,
		*tmp;

	current_screen->visible_windows++;
	if (curr_scr_win == (Window *) 0)
	{
		current_screen->window_list_end =
				current_screen->window_list = new;
		if (dumb)
		{
#ifdef SCROLL_AFTER_DISPLAY
			new->display_size = 24 - 1;
#else
			new->display_size = 24;	/* what the hell */
#endif /* SCROLL_AFTER_DISPLAY */
			set_current_window(new);
			return (new);
		}
		recalculate_windows();
	}
	else
	{
		/* split current window, or find a better window to split */
		if ((curr_scr_win->display_size < 4) ||
				get_int_var(ALWAYS_SPLIT_BIGGEST_VAR))
		{
			int	size = 0;

			for (tmp = current_screen->window_list; tmp;
					tmp = tmp->next)
			{
				if (tmp->display_size > size)
				{
					size = tmp->display_size;
					biggest = tmp;
				}
			}
			if ((biggest == (Window *) 0) || (size < 4))
			{
				say("Not enough room for another window!");
				/* Probably a source of memory leaks */
				new_free(&new);
				current_screen->visible_windows--;
				return ((Window *) 0);
			}
		}
		else
			biggest = curr_scr_win;
		if ((new->prev = biggest->prev) != NULL)
			new->prev->next = new;
		else
			current_screen->window_list = new;
		new->next = biggest;
		biggest->prev = new;
		new->top = biggest->top;
		new->bottom = (biggest->top + biggest->bottom) / 2;
		biggest->top = new->bottom + 1;
#ifdef SCROLL_AFTER_DISPLAY
		new->display_size = new->bottom - new->top - 1;
		biggest->display_size = biggest->bottom - biggest->top -
			biggest->menu.lines - 1;
#else
		new->display_size = new->bottom - new->top;
		biggest->display_size = biggest->bottom - biggest->top -
			biggest->menu.lines;
#endif /* SCROLL_AFTER_DISPLAY */
		new->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
		biggest->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	}
	return (new);
}

/*
 * remove_from_window_list: this removes the given window from the list of
 * visible windows.  It closes up the hole created by the windows abnsense in
 * a nice way 
 */
void
remove_from_window_list(window)
	Window	*window;
{
	Window	*other;

	/* find adjacent visible window to close up the screen */
	for (other = window->next; other; other = other->next)
	{
		if (other->visible)
		{
			other->top = window->top;
			break;
		}
	}
	if (other == (Window *) 0)
	{
		for (other = window->prev; other; other = other->prev)
		{
			if (other->visible)
			{
				other->bottom = window->bottom;
				break;
			}
		}
	}
	/* remove window from window list */
	if (window->prev)
		window->prev->next = window->next;
	else
		current_screen->window_list = window->next;
	if (window->next)
		window->next->prev = window->prev;
	else
		current_screen->window_list_end = window->prev;
	if (window->visible)
	{
		current_screen->visible_windows--;
#ifdef SCROLL_AFTER_DISPLAY
		other->display_size = other->bottom - other->top -
			other->menu.lines - 1;
#else
		other->display_size = other->bottom - other->top -
			other->menu.lines;
#endif /* SCROLL_AFTER_DISPLAY */
		if (window == curr_scr_win)
			set_current_window((Window *) 0);
		if (window->refnum == current_screen->last_window_refnum)
			current_screen->last_window_refnum =
			    curr_scr_win->refnum;
	}
}

/*
 * window_check_servers: this checks the validity of the open servers vs the
 * current window list.  Every open server must have at least one window
 * associated with it.  If a window is associated with a server that's no
 * longer open, that window's server is set to the primary server.  If an
 * open server has no assicatiate windows, that server is closed.  If the
 * primary server is no more, a new primary server is picked from the open
 * servers 
 */
void
window_check_servers()
{
	Window	*tmp;
	int	flag, cnt, max, i, not_connected,
	prime = -1;

	connected_to_server = 0;
	max = server_list_size();
	for (i = 0; i < max; i++)
	{
		not_connected = !is_server_open(i);
		flag = 1;
		cnt = 0;
		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (tmp->server == i)
			{
				if (not_connected)
					tmp->server = primary_server;
				else
				{
					prime = tmp->server;
					cnt++;
				}
			}
		}
		if (cnt == 0)
			close_server(i, "no windows left");
		else
			connected_to_server++;
	}
	if (!is_server_open(primary_server))
	{
		flag = 1;
		while ((tmp = traverse_all_windows(&flag)) != NULL)
			if (tmp->server == primary_server)
				tmp->server = prime;
		primary_server = prime;
	}
	update_all_status();
	cursor_to_input();
}

/*
 * delete_window: This deletes the given window.  It frees all data and
 * structures associated with the window, and it adjusts the other windows so
 * they will display correctly on the screen. 
 */
void
delete_window(window)
	Window	*window;
{
	char	*tmp = (char *) 0;

	if (window == (Window *) 0)
		window = curr_scr_win;
	if (window->visible && (current_screen->visible_windows == 1))
	{
		if (invisible_list)
		{
			swap_window(window, invisible_list);
			window = invisible_list;
		}
		else
		{
			say("You can't kill the last window!");
			return;
		}
	}
	if (window->name)
		strmcpy(buffer, window->name, BIG_BUFFER_SIZE);
	else
		sprintf(buffer, "%u", window->refnum);
	malloc_strcpy(&tmp, buffer);
	new_free(&window->status_line);
	new_free(&window->query_nick);
	new_free(&window->current_channel);
	new_free(&window->waiting_channel);
	new_free(&window->bind_channel);
	new_free(&window->logfile);
	new_free(&window->name);
	free_display(window);
	free_hold(window);
	free_lastlog(window);
	free_nicks(window);
	if (window->visible)
		remove_from_window_list(window);
	else
		remove_from_invisible_list(window);
	new_free(&window);
	window_check_servers();
	do_hook(WINDOW_KILL_LIST, "%s", tmp);
	new_free(&tmp);
}

/* delete_other_windows: zaps all visible windows except the current one */
static	void
delete_other_windows()
{
	Window	*tmp,
		*cur,
		*next;

	cur = curr_scr_win;
	tmp = current_screen->window_list;
	while (tmp)
	{
		next = tmp->next;
		if (tmp != cur)
			delete_window(tmp);
		tmp = next;
	}
}

/*
 * window_kill_swap:  Swaps with the last window that was hidden, then
 * kills the window that was swapped.  Give the effect of replacing the
 * current window with the last one, and removing it at the same time.
 */

void
window_kill_swap()
{
	if (invisible_list != (Window *) 0)
	{
		swap_last_window();
		delete_window(get_window_by_refnum(current_screen->last_window_refnum));
	}
	else
		say("There are no hidden windows!");
}

/*
 * unhold_windows: This is used by the main io loop to display held
 * information at an appropriate time.  Each time this is called, each
 * windows hold list is checked.  If there is info in the hold list and the
 * window is not held, the first line is displayed and removed from the hold
 * list.  Zero is returned if no infomation is displayed 
 */
int
unhold_windows()
{
	Window	*tmp;
	char	*stuff;
	int	hold_flag = 0,
		flag = 1;
	int	logged;

	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (!hold_output(tmp) && (stuff = hold_queue(tmp)))
		{
			logged = hold_queue_logged(tmp);
			if (rite(tmp, stuff, 1, 0, 0, logged) == 0)
			{
				remove_from_hold_list(tmp);
				hold_flag = 1;
			}
		}
	}
	return (hold_flag);
}

/*
 * update_window_status: This updates the status line for the given window.
 * If the refresh flag is true, the entire status line is redrawn.  If not,
 * only some of the changed portions are redrawn 
 */
void
update_window_status(window, refresh)
	Window	*window;
	int	refresh;
{
	if (dumb || (!window->visible) || !status_update_flag || never_connected)
		return;
	if (window == (Window *) 0)
		window = curr_scr_win;
	if (refresh)
		new_free(&(window->status_line));
	make_status(window);
}

/*
 * redraw_all_status: This redraws all of the status lines for all of the
 * windows. 
 */
void
redraw_all_status()
{
	Window	*tmp;

	if (dumb)
		return;
	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		new_free(&(tmp->status_line));
		make_status(tmp);
	}
	update_input(UPDATE_JUST_CURSOR);
	term_flush();
}

/*
 * update_all_status: This updates all of the status lines for all of the
 * windows.  By updating, it only draws from changed portions of the status
 * line to the right edge of the screen 
 */
/*ARGSUSED*/
void
update_all_status()
{
	Window	*window;
	Screen	*screen;

	if (dumb || !status_update_flag || never_connected)
		return;
	for (screen = screen_list; screen; screen = screen->next)
	{
		if (!screen->alive)
			continue;
		for (window = screen->window_list;window; window = window->next)
			if (window->visible)
				make_status(window);
	}
	update_input(UPDATE_JUST_CURSOR);
	term_flush();
}

/*
 * status_update: sets the status_update_flag to whatever flag is.  This also
 * calls update_all_status(), which will update the status line if the flag
 * was true, otherwise it's just ignored 
 */
void
status_update(flag)
	int	flag;
{
	Window *old_to_window;

	old_to_window = to_window;

	status_update_flag = flag;
	update_all_status();
	cursor_to_input();

	to_window = old_to_window;
}

/*
 * is_current_channel: Returns true is channel is a current channel for any
 * window.  If the delete flag is set, the unset channel as the current
 * channel for any and all windows 
 */
int
is_current_channel(channel, delete)
	char	*channel;
	char	delete;
{
	Window	*tmp;
	int	found = 0,
		flag = 1;

	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (tmp->current_channel &&
		    !my_stricmp(channel, tmp->current_channel) &&
		    (tmp->server == from_server))
		{
			found = 1;
			if (delete)
			{
				new_free(&(tmp->current_channel));
				tmp->update |= UPDATE_STATUS;
			}
		}
	}
	return (found);
}

extern int
is_bound(channel, server)
	char	*channel;
	int	server;
{
	Window *tmp;
	int	flag = 1;

	while (NULL != (tmp = traverse_all_windows(&flag)))
		if (tmp->server == server && tmp->bind_channel
		&& !my_stricmp(tmp->bind_channel, channel))
			return 1;
	return 0;
}

extern void
unbind_channel(channel, server)
	char	*channel;
	int	server;
{
	Window *tmp;
	int	flag = 1;

	while (NULL != (tmp = traverse_all_windows(&flag)))
		if (tmp->server == server && tmp->bind_channel
		&& !my_stricmp(tmp->bind_channel, channel))
		{
			new_free(&tmp->bind_channel);
			return;
		}
}

extern char *
get_bound_channel(window)
	Window *window;
{
	Window *tmp;
	int	flag = 1;

	while (NULL != (tmp = traverse_all_windows(&flag)))
		if (tmp == window)
			return tmp->bind_channel;
	return empty_string;
}

/*
 * get_window_server: returns the server index for the window with the given
 * refnum 
 */
int
get_window_server(refnum)
	unsigned int	refnum;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	return (tmp->server);
}

/*
 * set_window_server:  This sets the server of the given window to server. 
 * If refnum is -1 then we are setting the primary server and all windows
 * that are set to the current primary server are changed to server.  The all
 * flag is ignored in this case.  If refnum is not -1, then that window is
 * set to the given server.  If the all flag is set as well, then all windows
 * with the same server as renum are set to the new server as well 
 */
void
set_window_server(refnum, server, all)
	int	refnum;
	int	server;
	int	all;
{
	Window	*tmp,
		*window;
	int	flag = 1,
		old;

	if (refnum == -1)
	{
		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (tmp->server == primary_server)
				tmp->server = server;
		}
		window_check_servers();
		primary_server = server;
	}
	else
	{
		if ((window = get_window_by_refnum(refnum)) == NULL)
			window = curr_scr_win;
		old = window->server;
		if (all)
		{
			while ((tmp = traverse_all_windows(&flag)) != NULL)
			{
				if (tmp->server == old)
					tmp->server = server;
			}
		}
		else
			window->server = server;
		flag = 1;
		window_check_servers();
	}
}

/*
 * set_channel_by_refnum: This sets the current channel for the current
 * window. It returns the current channel as it's value.  If channel is null,
 * the * current channel is not changed, but simply reported by the function
 * result.  This treats as a special case setting the current channel to
 * channel "0".  This frees the current_channel for the
 * current_screen->current_window, * setting it to null 
 */
char	*
set_channel_by_refnum(refnum, channel)
	unsigned int	refnum;
	char	*channel;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	if (channel)
		if (strcmp(channel, "0") == 0)
			channel = (char *) 0;
	malloc_strcpy(&tmp->current_channel, channel);
	new_free(&tmp->waiting_channel);
	tmp->update |= UPDATE_STATUS;
	set_channel_window(tmp, channel);
	return (channel);
}

/* get_channel_by_refnum: returns the current channel for window refnum */
char	*
get_channel_by_refnum(refnum)
	u_int	refnum;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	return (tmp->current_channel);
}

/* current_refnum: returns the reference number for the current window */
unsigned int
current_refnum()
{
	return (curr_scr_win->refnum);
}

/* query_nick: Returns the query nick for the current channel */
char	*
query_nick()
{
	return (curr_scr_win->query_nick);
}

/* get_prompt_by_refnum: returns the prompt for the given window refnum */
char	*
get_prompt_by_refnum(refnum)
	u_int	refnum;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	if (tmp->prompt)
		return (tmp->prompt);
	else
		return (empty_string);
}

/*
 * get_target_by_refnum: returns the target for the window with the given
 * refnum (or for the current window).  The target is either the query nick
 * or current channel for the window 
 */
char	*
get_target_by_refnum(refnum)
	u_int	refnum;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	if (tmp->query_nick)
		return (tmp->query_nick);
	else if (tmp->current_channel)
		return (tmp->current_channel);
	else
		return ((char *) 0);
}

/* set_query_nick: sets the query nick for the current channel to nick */
void
set_query_nick(nick)
	char	*nick;
{
	char	*ptr;
	NickList *tmp;

	if (curr_scr_win->query_nick)
	{
		char	*nick;

		nick = curr_scr_win->query_nick;
		while(nick)
		{
			if ((ptr = (char *) index(nick,',')) != NULL)
				*(ptr++) = '\0';
			if ((tmp = (NickList *)
			    remove_from_list(&(curr_scr_win->nicks), nick))
			    != NULL)
			{
				new_free(&tmp->nick);
				new_free(&tmp);
			}
			nick = ptr;
		}
		new_free(&curr_scr_win->query_nick);
	}
	if (nick)
	{
		malloc_strcpy(&(curr_scr_win->query_nick), nick);
		curr_scr_win->update |= UPDATE_STATUS;
		while (nick)
		{
			if ((ptr = (char *) index(nick,',')) != NULL)
				*(ptr++) = '\0';
			tmp = (NickList *) new_malloc(sizeof(NickList));
			tmp->nick = (char *) 0;
			malloc_strcpy(&tmp->nick, nick);
			add_to_list(&(curr_scr_win->nicks),tmp);
			nick = ptr;
		}
	}
	update_window_status(curr_scr_win,0);
}

/*
 * goto_window: This will switch the current window to the window numbered
 * "which", where which is 0 through the number of visible windows on the
 * screen.  The which has nothing to do with the windows refnum. 
 */
static	void
goto_window(which)
	int	which;
{
	Window	*tmp;
	int	i;


	if (which == 0)
		return;
	if ((which < 0) || (which > current_screen->visible_windows))
	{
		say("GOTO: Illegal value");
		return;
	}
	tmp = current_screen->window_list;
	for (i = 1; tmp && (i != which); tmp = tmp->next, i++)
		;
	set_current_window(tmp);
}

/*
 * hide_window: sets the given window to invisible and recalculates remaing
 * windows to fill the entire screen 
 */
void
hide_window(window)
	Window	*window;
{
	if (current_screen->visible_windows == 1)
	{
		say("You can't hide the last window.");
		return;
	}
	if (window->visible)
	{
		remove_from_window_list(window);
		add_to_invisible_list(window);
#ifdef SCROLL_AFTER_DISPLAY
		window->display_size = LI - 3;
#else
		window->display_size = LI - 2;
#endif /* SCROLL_AFTER_DISPLAY */
		set_current_window((Window *) 0);
	}
}

/* hide_other_windows: makes all visible windows but the current one hidden */
void
hide_other_windows()
{
	Window	*tmp,
		*cur,
		*next;

	cur = curr_scr_win;
	tmp = current_screen->window_list;
	while (tmp)
	{
		next = tmp->next;
		if (tmp != cur)
			hide_window(tmp);
		tmp = next;
	}
}

#define WIN_FORM "%%-4s %%-%u.%us %%-%u.%us  %%-%u.%us %%-9.9s %%-10.10s %%s%%s"
static	void
list_a_window(window, len)
	Window	*window;
	int	len;
{
	char	tmp[10];

	sprintf(tmp, "%-4u", window->refnum);
	sprintf(buffer, WIN_FORM, NICKNAME_LEN, NICKNAME_LEN, len,
			len, get_int_var(CHANNEL_NAME_WIDTH_VAR),
			get_int_var(CHANNEL_NAME_WIDTH_VAR));
	say(buffer, tmp, get_server_nickname(window->server),
			window->name?window->name:"<None>",
			window->current_channel ?
				window->current_channel : "<None>",
			window->query_nick ? window->query_nick : "<None>",
			get_server_itsname(window->server),
			bits_to_lastlog_level(window->window_level),
			(window->visible) ? "" : " Hidden");
}

/*
 * list_windows: This Gives a terse list of all the windows, visible or not,
 * by displaying their refnums, current channel, and current nick 
 */
void
list_windows(void)
{
	Window	*tmp;
	int	flag = 1;
	int	len = 4;

	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (tmp->name && (strlen(tmp->name) > len))
			len = strlen(tmp->name);
	}
	sprintf(buffer, WIN_FORM, NICKNAME_LEN, NICKNAME_LEN, len, len,
		get_int_var(CHANNEL_NAME_WIDTH_VAR),
		get_int_var(CHANNEL_NAME_WIDTH_VAR));
	say(buffer, "Ref", "Nick", "Name", "Channel", "Query", "Server",
		"Level", empty_string);
	flag = 1;
	while ((tmp = traverse_all_windows(&flag)) != NULL)
		list_a_window(tmp, len);
#if 0
	tmp = NULL
	while ((new_window_traverse(&tmp, &flag)) != NULL)
		list_a_window(tmp, len);
#endif
}

/* show_window: This makes the given window visible.  */
static	void
show_window(window)
	Window	*window;
{
	if (window->visible)
	{
		set_current_window(window);
		return;
	}
	remove_from_invisible_list(window);
	if (add_to_window_list(window))
		set_current_window(window);
	else
		add_to_invisible_list(window);
}

/* push_window_by_refnum:  This pushes the given refnum onto the window stack */
static	void
push_window_by_refnum(refnum)
	u_int	refnum;
{
	WindowStack *new;

	new = (WindowStack *) new_malloc(sizeof(WindowStack));
	new->refnum = refnum;
	new->next = current_screen->window_stack;
	current_screen->window_stack = new;
}

/*
 * pop_window: this pops the top entry off the window stack and sets the
 * current window to that window.  If the top of the window stack is no
 * longer a valid window, then that entry is discarded and the next entry
 * down is tried (as so on).  If the stack is empty, the current window is
 * left unchanged 
 */
static	void
pop_window()
{
	int	refnum;
	WindowStack *tmp;
	Window	*win;

	while (1)
	{
		if (current_screen->window_stack)
		{
			refnum = current_screen->window_stack->refnum;
			tmp = current_screen->window_stack->next;
			new_free(&current_screen->window_stack);
			current_screen->window_stack = tmp;
			if ((win = get_window_by_refnum(refnum)) != NULL)
			{
				if (!win->visible)
					show_window(win);
				else
					set_current_window(win);
				break;
			}
		}
		else
		{
			say("The window stack is empty!");
			break;
		}
	}
}

/*
 * show_stack: displays the current window stack.  This also purges out of
 * the stack any window refnums that are no longer valid 
 */
static	void
show_stack()
{
	WindowStack *last = (WindowStack *) 0,
	    *tmp, *crap;
	Window	*win;
	int	flag = 1;
	int	len = 4;

	while ((win = traverse_all_windows(&flag)) != NULL)
	{
		if (win->name && (strlen(win->name) > len))
			len = strlen(win->name);
	}
	say("Window stack:");
	tmp = current_screen->window_stack;
	while (tmp)
	{
		if ((win = get_window_by_refnum(tmp->refnum)) != NULL)
		{
			list_a_window(win, len);
			tmp = tmp->next;
		}
		else
		{
			crap = tmp->next;
			new_free(&tmp);
			if (last)
				last->next = crap;
			else
				current_screen->window_stack = crap;
			tmp = crap;
		}
	}
}

/*
 * is_window_name_unique: checks the given name vs the names of all the
 * windows and returns true if the given name is unique, false otherwise 
 */
static	int
is_window_name_unique(name)
	char	*name;
{
	Window	*tmp;
	int	flag = 1;

	if (name)
	{
		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (tmp->name && (my_stricmp(tmp->name, name) == 0))
				return (0);
		}
	}
	return (1);
}

/*
 * add_nicks_by_refnum:  This adds the given str to the nicklist of the
 * window refnum.  Only unique names are added to the list.  If the name is
 * preceeded by a ^ it is removed from the list.  The str may be a comma
 * separated list of nicks, channels, etc 
 */
static	void
add_nicks_by_refnum(refnum, str)
	u_int	refnum;
	char	*str;
{
	Window	*tmp;
	char	*ptr;
	NickList *new;

	if ((tmp = get_window_by_refnum(refnum)) != NULL)
	{
		while (str)
		{
			if ((ptr = (char *) index(str, ',')) != NULL)
				*(ptr++) = '\0';
			if (*str == '^')
			{
				if ((new = (NickList *)
				    remove_from_list(&(tmp->nicks),str + 1)) !=
				    NULL)
				{
					say("%s removed from window name list",
						new->nick);
					new_free(&new->nick);
					new_free(&new);
				}
				else
					say("%s is not on the list for this window!", str + 1);
			}
			else
			{
				if (!find_in_list(&(tmp->nicks), str,
						!USE_WILDCARDS))
				{
					say("%s add to window name list", str);
					new = (NickList *)
						new_malloc(sizeof(NickList));
					new->nick = (char *) 0;
					malloc_strcpy(&new->nick, str);
					add_to_list(&(tmp->nicks), new);
				}
				else
					say("%s already on window name list",
						str);
			}
			str = ptr;
		}
	}
	else
		say("No such window!");
}

/* below is stuff used for parsing of WINDOW command */

/*
 * get_window_by_name: returns a pointer to a window with a matching logical
 * name or null if no window matches 
 */
Window	*
get_window_by_name(name)
	char	*name;
{
	Window	*tmp;
	int	flag = 1;

	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (tmp->name && (my_stricmp(tmp->name, name) == 0))
			return (tmp);
	}
	return ((Window *) 0);
}

/*
 * get_window: this parses out any window (visible or not) and returns a
 * pointer to it 
 */
static	Window	*
get_window(name, args)
	char	*name;
	char	**args;
{
	char	*arg;
	Window	*tmp;

	if ((arg = next_arg(*args, args)) != NULL)
	{
		if (is_number(arg))
		{
			if ((tmp = get_window_by_refnum(atoi(arg))) != NULL)
				return (tmp);
		}
		if ((tmp = get_window_by_name(arg)) != NULL)
			return (tmp);
		say("%s: No such window: %s", name, arg);
	}
	else
		say("%s: Please specify a window refnum or name", name);
	return ((Window *) 0);
}

/*
 * get_invisible_window: parses out an invisible window by reference number.
 * Returns the pointer to the window, or null.  The args can also be "LAST"
 * indicating the top of the invisible window list (and thus the last window
 * made invisible) 
 */
static	Window	*
get_invisible_window(name, args)
	char	*name;
	char	**args;
{
	char	*arg;
	Window	*tmp;

	if ((arg = next_arg(*args, args)) != NULL)
	{
		if (my_strnicmp(arg, "LAST", strlen(arg)) == 0)
		{
			if (invisible_list == (Window *) 0)
				say("%s: There are no hidden windows", name);
			return (invisible_list);
		}
		if ((tmp = get_window(name, &arg)) != NULL)
		{
			if (!tmp->visible)
				return (tmp);
			else
			{
				if (tmp->name)
					say("%s: Window %s is not hidden!",
						name, tmp->name);
				else
					say("%s: Window %d is not hidden!",
						name, tmp->refnum);
			}
		}
	}
	else
		say("%s: Please specify a window refnum or LAST", name);
	return ((Window *) 0);
}

#ifdef THIS_NEVER_GETS_USED
/*
 * get_visible_window: parses out a window by reference number.  Returns the
 * pointer to thw window if it exists and if it is visible. 
 */
static	Window	*
get_visible_window(name, args)
	char	*name;
	char	**args;
{
	char	*arg;
	Window	*tmp;

	if (arg = next_arg(*args, args))
	{
		if (my_strnicmp(arg, "LAST", strlen(arg)) == 0)
			tmp = get_window_by_refnum(current_screen->last_window_refnum);
		else
			tmp = get_window(name, &arg);
		if (tmp)
		{
			if (tmp->visible)
				return (tmp);
			else
			{
				if (tmp->name)
					say("%s: Window %s is hidden!", name, tmp->name);
				else
					say("%s: Window %d is hidden!", name, tmp->refnum);
			}
		}
	}
	else
		say("%s: Please specify a window refnum or LAST", name);
	return ((Window *) 0);
}
#endif

/* get_number: parses out an integer number and returns it */
static	int
get_number(name, args)
	char	*name;
	char	**args;
{
	char	*arg;

	if ((arg = next_arg(*args, args)) != NULL)
		return (atoi(arg));
	else
		say("%s: You must specify the number of lines", name);
	return (0);
}

/*
 * get_boolean: parses either ON, OFF, or TOGGLE and sets the var
 * accordingly.  Returns 0 if all went well, -1 if a bogus or missing value
 * was specified 
 */
static	int
get_boolean(name, args, var)
	char	*name;
	char	**args;
	int	*var;
{
	char	*arg;

	if (((arg = next_arg(*args, args)) == (char *) 0) ||
	    do_boolean(arg, var))
	{
		say("Value for %s must be ON, OFF, or TOGGLE", name);
		return (-1);
	}
	else
	{
		say("Window %s is %s", name, var_settings[*var]);
		return (0);
	}
}

/*ARGSUSED*/
void
window(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	len;
	char	*arg;
	int	no_args = 1;
	Window	*window,
		*tmp;

	in_window_command = 1;
	message_from((char *) 0, LOG_CURRENT);
	window = curr_scr_win;
	while ((arg = next_arg(args, &args)) != NULL)
	{
		no_args = 0;
		len = strlen(arg);
		if (my_strnicmp("NEW", arg, len) == 0)
		{
			if ((tmp = new_window()) != NULL)
				window = tmp;
		}
#ifdef WINDOW_CREATE
		else if (my_strnicmp("CREATE", arg, len) == 0)
		{
			if ((tmp = create_additional_screen()) != NULL)
				window = tmp;
			else
				say("Cannot create new screen!");
		}
		else if (!my_strnicmp("DELETE", arg, len))
			kill_screen(current_screen);
#endif /* WINODW_CREATE */
		else if (my_strnicmp("REFNUM", arg, len) == 0)
		{
			if ((tmp = get_window("REFNUM", &args)) != NULL)
			{
				if (tmp->screen && tmp->screen !=window->screen)
					say("Window in another screen!");
				else if (tmp->visible)
				{ 
					set_current_window(tmp);
					window = tmp;
				}
				else
					say("Window not visible!");
			}
			else
			{
				say("No such window!");
				in_window_command = 0;
				return;
			}
		}
		else if (my_strnicmp("KILL", arg, len) == 0)
			delete_window(window);
		else if (my_strnicmp("SHRINK", arg, len) == 0)
			grow_window(window, -get_number("SHRINK", &args));
		else if (my_strnicmp("GROW", arg, len) == 0)
			grow_window(window, get_number("SHRINK", &args));
		else if (my_strnicmp("SCROLL", arg, len) == 0)
			get_boolean("SCROLL", &args, &(window->scroll));
		else if (my_strnicmp("LOG", arg, len) == 0)
		{
			if (get_boolean("LOG", &args, &(window->log)))
				break;
			else
			{
				char	*logfile;
				int	add_ext = 1;

				if ((logfile = window->logfile) != NULL)
					add_ext = 0;
				else if (!(logfile = get_string_var(LOGFILE_VAR)))
					logfile = empty_string;
				if (!add_ext)
					sprintf(buffer, "%s", logfile);
				else if (window->current_channel)
					sprintf(buffer, "%s.%s", logfile,
						window->current_channel);
				else if (window->query_nick)
					sprintf(buffer, "%s.%s", logfile,
						window->query_nick);
				else
					sprintf(buffer, "%s.Window_%d", logfile,
						window->refnum);
				window->log_fp = do_log(window->log, buffer,
					window->log_fp);
				if (window->log_fp == (FILE *) 0)
					window->log = 0;
			}
		}
		else if (my_strnicmp("HOLD_MODE", arg, len) == 0)
		{
			if (get_boolean("HOLD_MODE",&args,&(window->hold_mode)))
				break;
			else
				set_int_var(HOLD_MODE_VAR, window->hold_mode);
		}
		else if (my_strnicmp("LASTLOG_LEVEL", arg, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				window->lastlog_level = parse_lastlog_level(arg, "LASTLOG");
				say("Lastlog level is %s",
				  bits_to_lastlog_level(window->lastlog_level));
			}
			else
				say("Level required");
		}
		else if (my_strnicmp("LEVEL", arg, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				window->window_level = parse_lastlog_level(arg, "WINDOW");
				say("Window level is %s",
				   bits_to_lastlog_level(window->window_level));
				revamp_window_levels(window);
			}
			else
				say("LEVEL: Level required");
		}
		else if (my_strnicmp("BALANCE", arg, len) == 0)
			recalculate_windows();
		else if (my_strnicmp("NAME", arg, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				if (is_window_name_unique(arg))
				{
					malloc_strcpy(&(window->name), arg);
					window->update |= UPDATE_STATUS;
				}
				else
					say("%s is not unique!", arg);
			}
			else
				say("You must specify a name for the window!");
		}
		else if (my_strnicmp("PROMPT", arg, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				malloc_strcpy(&(window->prompt), arg);
				window->update |= UPDATE_STATUS;
			}
			else
			    say("You must specify a prompt for the window!");
		}
		else if (my_strnicmp("GOTO", arg, len) == 0)
		{
			goto_window(get_number("GOTO", &args));
			window = curr_scr_win;
		}
		else if (my_strnicmp("LAST", arg, len) == 0)
			set_current_window((Window *) 0);
		else if (my_strnicmp("MOVE", arg, len) == 0)
		{
			move_window(window, get_number("MOVE", &args));
			window = curr_scr_win;
		}
		else if (my_strnicmp("SWAP", arg, len) == 0)
		{
			if ((tmp = get_invisible_window("SWAP", &args)) != NULL)
				swap_window(window, tmp);
		}
		else if (my_strnicmp("HIDE", arg, len) == 0)
			hide_window(window);
		else if (my_strnicmp("PUSH", arg, len) == 0)
			push_window_by_refnum(window->refnum);
		else if (my_strnicmp("POP", arg, len) == 0)
			pop_window();
		else if (my_strnicmp("ADD", arg, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				add_nicks_by_refnum(window->refnum, arg);
			}
			else
				say("ADD: Do something!  Geez!");
		}
		else if (my_strnicmp("REMOVE", arg, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				char	*tmp = (char *) malloc(strlen(arg) + 1);

				*tmp = '^';
				strcpy(tmp + 1, arg);
				add_nicks_by_refnum(window->refnum, tmp);
				new_free(&tmp);
			}
			else
				say("REMOVE: Do something!  Geez!");
		}
		else if (my_strnicmp("STACK", arg, len) == 0)
			show_stack();
		else if (my_strnicmp("LIST", arg, len) == 0)
			list_windows();
		else if (my_strnicmp("SERVER", arg, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				int	i,
					port_num;
				char	*port,
					*password,
					*nick;

				parse_server_info(arg, &port, &password, &nick);
				if (port)
				{
					port_num = atoi(port);
					if (!port_num)
						port_num = -1;
				}
				else
					port_num = -1;
				if ((i = find_in_server_list(arg, port_num))
						!= -1)
					port_num = server_list[i].port;
				if (-1 == (i = parse_server_index(arg)))
				{
					if (nick && *nick)
					    malloc_strcpy(&connect_next_nick,
						nick);
					if (!connect_to_server(arg,port_num,-1))
					{
					    set_window_server(window->refnum,
						from_server, 0);
#ifndef PHONE
					    window->window_level = LOG_ALL;
#else
					    window->window_level = LOG_NONE;
#endif
					    if (window->current_channel)
					      new_free(&window->current_channel);
					    update_all_status();
					}
				}
				else
				{
					if (nick && *nick)
						malloc_strcpy(&(
						    server_list[i].nickname),
						    nick);
					if(!connect_to_server(get_server_name(i)
						, server_list[i].port, -1))
					{
					    set_window_server(window->refnum,
						from_server, 0);
#ifndef PHONE
					    window->window_level = LOG_ALL;
#else
					    window->window_level = LOG_NONE;
#endif
					    if (window->current_channel)
					      new_free(&window->current_channel);
					    update_all_status();
					}
				}
			}
			else
				say("SERVER: You must specify a server");
		}
		else if (my_strnicmp("SHOW", arg, len) == 0)
		{
			if ((tmp = get_window("SHOW", &args)) != NULL)
			{
				show_window(tmp);
				window = curr_scr_win;
			}
		}
		else if (my_strnicmp("HIDE_OTHERS", arg, len) == 0)
			hide_other_windows();
		else if (my_strnicmp("KILL_OTHERS", arg, len) == 0)
			delete_other_windows();
		else if (my_strnicmp("NOTIFY", arg, len) == 0)
		{
			window->miscflags ^= WINDOW_NOTIFY;
			say("Notification when hidden set to %s",
			    (window->miscflags & WINDOW_NOTIFY)? "ON" : "OFF");
		}
		else if (my_strnicmp("CHANNEL", arg, len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				if (is_bound(arg, from_server))
				{
					say("Channel %s is bound", arg);
				}
				else if (is_current_channel(arg, 1))
				{
					say("You are now talking to channel %s",
						arg);
					set_last_current_channel(0, arg);
					set_channel_by_refnum(0, arg);
				}
				else
				{
			if (is_on_channel(arg, nickname))
			{
				say("You are now talking to channel %s", arg);
				set_last_current_channel(0, arg);
				set_channel_by_refnum(0, arg);
			}
			else if (*arg == '0' && !*(args + 1))
				set_channel_by_refnum(0, NULL);
			else
			{
				int	server;

				server = from_server;
				from_server = window->server;
				if (get_server_version(window->server) == Server2_5)
					send_to_server( "CHANNEL %s", arg);
				else
					send_to_server("JOIN %s", arg);
				malloc_strcpy(&window->waiting_channel, arg);
				from_server = server;
			}
				}
			}
			else
				set_channel_by_refnum(0, "0");
		}
		else if (my_strnicmp("PREVIOUS", arg, len) == 0)
		{
			swap_previous_window();
		}
		else if (my_strnicmp("NEXT", arg, len) == 0)
		{
			swap_next_window();
		}
		else if (my_strnicmp("BACK", arg, len) == 0)
		{
			back_window();
		}
		else if (my_strnicmp("KILLSWAP", arg, len) == 0)
		{
			window_kill_swap();
		}
		else if (!my_strnicmp("LOGFILE", arg, len))
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				malloc_strcpy(&window->logfile, arg);
				say("Window LOGFILE set to %s", arg);
			}
			else
				say("No LOGFILE given");
		}
		else if (!my_strnicmp("NOTIFY_LEVEL", arg, len))
		{
			if ((arg = next_arg(args, &args)) != NULL)
			{
				window->notify_level = parse_lastlog_level(arg, "NOTIFY");
				say("Window notify level is %s",
				   bits_to_lastlog_level(window->notify_level));
			}
			else
				say("Level missing");
		}
		else if (!my_strnicmp("NUMBER", arg, len))
		{
			if ((arg = next_arg(args, &args)) != (char *) 0)
			{
				int	i;
				Window	*tmp;

				i = atoi(arg);
				if (i > 0)
				{
					/* check if window number exists */
						
					tmp = get_window_by_refnum(i);
					if (!tmp)
						window->refnum = i;
					else
					{
						tmp->refnum = window->refnum;
						window->refnum = i;
					}
					update_all_status();
				}
				else
					say("Window number must be greater than 1");
			}
			else
				say("Window number missing");
		}
		else if (!my_strnicmp("BIND", arg, len))
		{
			if ((arg = next_arg(args, &args)) != (char *) 0)
			{
				if (!is_channel(arg))
					say("BIND: %s is not a valid channel name", arg);
				else
				{
					if (is_bound(arg, from_server)
					&& is_current_channel(arg, 1))
						unbind_channel(arg, from_server);
					malloc_strcpy(&window->bind_channel, arg);
					if (is_on_channel(arg, nickname))
						set_channel_by_refnum(0, arg);
					say("Bound window to channel %s", arg);
				}
			}
			else
				say("Bound to channel %s", get_bound_channel(window));
		}
		else if (!my_strnicmp("UNBIND", arg, len))
		{
			if ((arg = next_arg(args, &args)) != (char *) 0)
			{
				if (is_bound(arg, from_server))
				{
					say("Channel %s is no longer bound", arg);
					unbind_channel(arg, from_server);
				}
				else
					say("Channel %s is not bound", arg);
			}
			else
			{
				say("Channel %s is no longer bound", window->bind_channel);
				new_free(&window->bind_channel);
			}
		}
		else
			say("Unknown WINDOW command: %s", arg);
	}
	if (no_args)
	{
		if (window->name)
			say("Window %s (%u)", window->name, window->refnum);
		else
			say("Window %u", window->refnum);
		if (window->server == -1)
			say("\tServer: <None>");
		else
			say("\tServer: %s", get_server_name(window->server));
		say("\tCurrent channel: %s", window->current_channel ?
			window->current_channel : "<None>");
		if (window->waiting_channel)
			say ("\tWaiting channel: %s", window->waiting_channel);
		if (window->bind_channel)
			say ("\tBound channel: %s", window->bind_channel);
		say("\tQuery User: %s", (window->query_nick ?
			window->query_nick : "<None>"));
		say("\tPrompt: %s", window->prompt ?
			window->prompt : "<None>");
		say("\tScrolling is %s", var_settings[window->scroll]);
		say("\tLogging is %s", var_settings[window->log]);
		if (window->logfile)
			say("\tLogfile is %s", window->logfile);
		else
			say("\tNo logfile given");
		say("\tNotification is %s",
			var_settings[window->miscflags & WINDOW_NOTIFY]);
		say("\tHold mode is %s", var_settings[window->hold_mode]);
		say("\tWindow level is %s",
			bits_to_lastlog_level(window->window_level));
		say("\tLastlog level is %s",
			bits_to_lastlog_level(window->lastlog_level));
		say("\tNotify level is %s",
			bits_to_lastlog_level(window->notify_level));
		if (window->nicks)
		{
			NickList *tmp;

			say("\tName list:");
			for (tmp = window->nicks; tmp; tmp = tmp->next)
				say("\t  %s", tmp->nick);
		}
	}
	in_window_command = 0;
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	cursor_to_input();
}

int
number_of_windows()
{
	return (current_screen->visible_windows);
}

void
unstop_all_windows()
{
	Window	*tmp;

	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
		hold_mode(tmp, OFF, 1);
}

/* this will make underline toggle between 2 and -1 and never let it get to 0 */
void
set_underline_video(value)
int	value;
{
	if (value == OFF)
		underline = -1;
	else
		underline = 1;
}

void
set_last_current_channel(refnum, channel)
	unsigned int refnum;
	char	*channel;
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	if (channel)
		if (strcmp(channel, "0") == 0)
			channel = (char *) 0;
	malloc_strcpy(&tmp->last_current_channel, channel);
}

int
is_last_current_channel(refnum, channel)
	unsigned int refnum;
	char	*channel;
{
	Window	*tmp;
	char	*last;

	if ((tmp = get_window_by_refnum(refnum)) == (Window *) 0)
		tmp = curr_scr_win;
	last = tmp->last_current_channel;

	if (last && !my_stricmp(channel, last))
		return 1;

	return 0;
}

int
get_refnum_by_channel(channel)
	char	*channel;
{
	Window	*tmp;
	int	flag = 1;

	while ((tmp = traverse_all_windows(&flag)) != NULL)
	{
		if (tmp->current_channel &&
		    !my_stricmp(channel, tmp->current_channel) &&
		    (tmp->server == from_server))
		{
			return tmp->refnum;
		}
	}
	return 0;
}
