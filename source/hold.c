/*
 * hold.c: handles buffering of display output.
 *
 * Written By Michael Sandrof 
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: hold.c,v 1.1.1.1 1999/07/16 21:21:42 toast Exp $";
#endif

#include "irc.h"

#include "ircaux.h"
#include "window.h"
#include "screen.h"
#include "vars.h"
#include "input.h"

/* reset_hold: Make hold_mode behave like VM CHAT, hold only
 * when there is no user interaction, this should be called
 * whenever the user does something in a window.  -lynx
 */
void
reset_hold(win)
	Window	*win;
{
	if (!win)
		win = curr_scr_win;
	if (!win->scrolled_lines)
		win->line_cnt = 0;
}

/* add_to_hold_list: adds str to the hold list queue */
void
add_to_hold_list(window, str, logged)
	Window	*window;
	char	*str;
	int	logged;
{
	Hold	*new;
	unsigned int	max;

	new = (Hold *) new_malloc(sizeof(Hold));
	new->str = (char *) 0;
	malloc_strcpy(&(new->str), str);
	new->logged = logged;
	window->held_lines++;
	if ((max = get_int_var(HOLD_MODE_MAX_VAR)) != 0)
	{
		if (window->held_lines > max)
			hold_mode(window, OFF, 1);
	}
	new->next = window->hold_head;
	new->prev = (Hold *) 0;
	if (window->hold_tail == (Hold *) 0)
		window->hold_tail = new;
	if (window->hold_head)
		window->hold_head->prev = new;
	window->hold_head = new;
	update_all_status();
}

/* remove_from_hold_list: pops the next element off the hold list queue. */
void
remove_from_hold_list(window)
	Window	*window;
{
	Hold	*crap;

	if (window->hold_tail)
	{
		window->held_lines--;
		new_free(&window->hold_tail->str);
		crap = window->hold_tail;
		window->hold_tail = window->hold_tail->prev;
		if (window->hold_tail == (Hold *) 0)
			window->hold_head = window->hold_tail;
		else
			window->hold_tail->next = (Hold *) 0;
		new_free(&crap);
		update_all_status();
	}
}

/*
 * hold_mode: sets the "hold mode".  Really.  If the update flag is true,
 * this will also update the status line, if needed, to display the hold mode
 * state.  If update is false, only the internal flag is set.  
 */
void
hold_mode(window, flag, update)
	Window	*window;
	int	flag,
		update;
{
	if (window == (Window *) 0)
		window = curr_scr_win;
	if (flag != ON && window->scrolled_lines)
		return;
	if (flag == TOGGLE)
	{
		if (window->held == OFF)
			window->held = ON;
		else
			window->held = OFF;
	}
	else
		window->held = flag;
	if (update)
	{
		if (window->held != window->last_held)
		{
			window->last_held = window->held;
					/* This shouldn't be done
					 * this way */
			update_window_status(window, 0);
			if (window->update | UPDATE_STATUS)
				window->update -= UPDATE_STATUS;
			cursor_in_display();
			update_input(NO_UPDATE);
		}
	}
	else
		window->last_held = -1;
}

/*
 * hold_output: returns the state of the window->held, which is set in the
 * hold_mode() routine. 
 */
int
hold_output(window)
	Window	*window;
{
	if (!window)
		window = curr_scr_win;
	return (window->held == ON) || (window->scrolled_lines != 0);
}

/*
 * hold_queue: returns the string value of the next element on the hold
 * quere.  This does not change the hold queue 
 */
char	*
hold_queue(window)
	Window	*window;
{
	if (window->hold_tail)
		return (window->hold_tail->str);
	else
		return ((char *) 0);
}

int
hold_queue_logged(window)
	Window	*window;
{
	if (window->hold_tail)
		return window->hold_tail->logged;
	else
		return 0;
}

/* toggle_stop_screen: the BIND function TOGGLE_STOP_SCREEN */
void
toggle_stop_screen()
{
	hold_mode((Window *) 0, TOGGLE, 1);
	update_all_windows();
}
