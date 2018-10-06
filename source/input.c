/*
 * input.c: does the actual input line stuff... keeps the appropriate stuff
 * on the input line, handles insert/delete of characters/words... the whole
 * ball o wax 
 *
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: input.c,v 1.1.1.1 1999/07/16 21:21:42 toast Exp $";
#endif

#include "irc.h"

#include "input.h"
#include "term.h"
#include "alias.h"
#include "vars.h"
#include "ircaux.h"
#include "window.h"
#include "screen.h"
#include "exec.h"

#define WIDTH 10

/* input_prompt: contains the current, unexpanded input prompt */
static	char	*input_prompt = (char *) 0;

/* input_line: the actual screen line where the input goes */
static	int	input_line;

/* str_start: position in buffer of first visible character in the input line */
static	int	str_start = 0;

/*
 * upper_mark and lower_mark: marks the upper and lower positions in the
 * input buffer which will cause the input display to switch to the next or
 * previous bunch of text 
 */
static	int	lower_mark;
static	int	upper_mark;

/* zone: the amount of editable visible text on the screen */
static	int	zone;

/* cursor: position of the cursor in the input line on the screen */
static	int	cursor = 0;

/* cursor_to_input: move the cursor to the input line, if not there already */
void
cursor_to_input()
{
	Screen *old_current_screen;

	old_current_screen = current_screen;
	for (current_screen = screen_list; current_screen;
			current_screen = current_screen->next)
	{
		if (current_screen->alive && is_cursor_in_display())
		{
			term_move_cursor(cursor, input_line);
			cursor_not_in_display();
			term_flush();
		}
	}
	set_current_screen(old_current_screen);
}

/*
 * update_input: does varying amount of updating on the input line depending
 * upon the position of the cursor and the update flag.  If the cursor has
 * move toward one of the edge boundaries on the screen, update_cursor()
 * flips the input line to the next (previous) line of text. The update flag
 * may be: 
 *
 * NO_UPDATE - only do the above bounds checking. 
 *
 * UPDATE_JUST_CURSOR - do bounds checking and position cursor where is should
 * be. 
 *
 * UPDATE_FROM_CURSOR - does all of the above, and makes sure everything from
 * the cursor to the right edge of the screen is current (by redrawing it). 
 *
 * UPDATE_ALL - redraws the entire line 
 */
void
update_input(update)
	int	update;
{
	int	old_start;
	static	int	co = 0,
			li = 0;
	char	*ptr;
	int	len,
		free_it = 1,
		cnt,
		max;
	char	*prompt;

	if (dumb)
		return;
	cursor_to_input();
	if (current_screen->promptlist)
		prompt = current_screen->promptlist->prompt;
	else
		prompt = input_prompt;
	if (prompt)
	{
		if (update != NO_UPDATE)
		{
			char	*inp_ptr = (char *) 0;
			int	args_used;	/* this isn't used here but is
						 * passed to expand_alias()
						 */
			if (is_process(get_target_by_refnum(0)))
			{
				ptr = (char *)get_prompt_by_refnum(0);
				free_it = 0;
			}
			else
				ptr = expand_alias((char *) 0, prompt, empty_string, &args_used, NULL);
			if (*ptr && ((my_strnicmp(ptr, "Password:", 9) == 0) || (my_strnicmp(ptr, "Operator Password:",18) == 0) ||
					(my_strnicmp(ptr, "Server Password:", 16) == 0)))
				term_echo(0);
			else
				term_echo(1);
			len = strlen(ptr);
			if (strncmp(ptr, current_screen->input_buffer, len) || !len)
			{
				malloc_strcpy(&inp_ptr, current_screen->input_buffer + current_screen->buffer_min_pos);
				strmcpy(current_screen->input_buffer, ptr, INPUT_BUFFER_SIZE);
				current_screen->buffer_pos += (len - current_screen->buffer_min_pos);
				current_screen->buffer_min_pos = strlen(ptr);
				strmcat(current_screen->input_buffer, inp_ptr, INPUT_BUFFER_SIZE);
				new_free(&inp_ptr);
				update = UPDATE_ALL;
			}
			if (free_it)
				new_free(&ptr);
		}
	}
	else
		term_echo(1);
	if ((li != LI) || (co != CO))
	{
		/* resized?  Keep it simple and reset everything */
		input_line = LI - 1;
		zone = CO - (WIDTH * 2);
		lower_mark = WIDTH;
		upper_mark = CO - WIDTH;
		cursor = current_screen->buffer_min_pos;
		current_screen->buffer_pos = current_screen->buffer_min_pos;
		str_start = 0;
		li = LI;
		co = CO;
	}
	old_start = str_start;
	while ((current_screen->buffer_pos < lower_mark) && lower_mark > WIDTH)
	{
		upper_mark = lower_mark;
		lower_mark -= zone;
		str_start -= zone;
	}
	while (current_screen->buffer_pos >= upper_mark)
	{
		lower_mark = upper_mark;
		upper_mark += zone;
		str_start += zone;
	}
	cursor = current_screen->buffer_pos - str_start;
	if ((old_start != str_start) || (update == UPDATE_ALL))
	{
		term_move_cursor(0, input_line);
		if ((str_start == 0) && (current_screen->buffer_min_pos > 0))
		{
			int	echo;

			echo = term_echo(1);
			if (current_screen->buffer_min_pos > (CO - WIDTH))
				len = CO - WIDTH - 1;
			else
				len = current_screen->buffer_min_pos;
			cnt = term_puts(&(current_screen->input_buffer[
				str_start]), len);
			term_echo(echo);
			cnt += term_puts(&(current_screen->input_buffer[
				str_start + len]), CO - len);
		}
		else
			cnt = term_puts(&(current_screen->input_buffer[
				str_start]), CO);
		if (term_clear_to_eol())
			term_space_erase(cnt);
		term_move_cursor(cursor, input_line);
	}
	else if (update == UPDATE_FROM_CURSOR)
	{
		term_move_cursor(cursor, input_line);
		cnt = cursor;
		max = CO - (current_screen->buffer_pos - str_start);
		if ((len = strlen(&(current_screen->input_buffer[
				current_screen->buffer_pos]))) > max)
			len = max;
		cnt += term_puts(&(current_screen->input_buffer[
			current_screen->buffer_pos]), len);
		if (term_clear_to_eol())
			term_space_erase(cnt);
		term_move_cursor(cursor, input_line);
	}
	else if (update == UPDATE_JUST_CURSOR)
		term_move_cursor(cursor, input_line);
	term_flush();
}

void
refresh_inputline()
{
	update_input(UPDATE_ALL);
}

void
change_input_prompt(direction)
	int	direction;
{
	if (!current_screen->promptlist)
	{
		strcpy(current_screen->input_buffer,
				current_screen->saved_input_buffer);
		current_screen->buffer_pos =
				current_screen->saved_buffer_pos;
		current_screen->buffer_min_pos =
				current_screen->saved_min_buffer_pos;
		update_input(UPDATE_ALL);
	}
	else if (direction == -1)
	{
		update_input(UPDATE_ALL);
	}
	else if (!current_screen->promptlist->next)
	{
		strcpy(current_screen->saved_input_buffer,
				current_screen->input_buffer);
		current_screen->saved_buffer_pos =
				current_screen->buffer_pos;
		current_screen->saved_min_buffer_pos =
				current_screen->buffer_min_pos;
		*current_screen->input_buffer = '\0';
		current_screen->buffer_pos =
				current_screen->buffer_min_pos = 0;
		update_input(UPDATE_ALL);
	}
}

/* input_move_cursor: moves the cursor left or right... got it? */
void
input_move_cursor(dir)
	int	dir;
{
	cursor_to_input();
	if (dir)
	{
		if (current_screen->input_buffer[current_screen->buffer_pos])
		{
			current_screen->buffer_pos++;
			if (term_cursor_right())
				term_move_cursor(cursor + 1, input_line);
		}
	}
	else
	{
		if (current_screen->buffer_pos > current_screen->buffer_min_pos)
		{
			current_screen->buffer_pos--;
			if (term_cursor_left())
				term_move_cursor(cursor - 1, input_line);
		}
	}
	update_input(NO_UPDATE);
}

/*
 * input_forward_word: move the input cursor forward one word in the input
 * line 
 */
void
input_forward_word()
{
	cursor_to_input();
	while (
	  (isspace(current_screen->input_buffer[current_screen->buffer_pos]) ||
	  ispunct(current_screen->input_buffer[current_screen->buffer_pos])) &&
	  current_screen->input_buffer[current_screen->buffer_pos])
		current_screen->buffer_pos++;
	while
	 (!(ispunct(current_screen->input_buffer[current_screen->buffer_pos]) ||
	 isspace(current_screen->input_buffer[current_screen->buffer_pos])) &&
	 current_screen->input_buffer[current_screen->buffer_pos])
		current_screen->buffer_pos++;
	update_input(UPDATE_JUST_CURSOR);
}

/* input_backward_word: move the cursor left on word in the input line */
void
input_backward_word()
{
	cursor_to_input();
	while ((current_screen->buffer_pos > current_screen->buffer_min_pos) &&
	 (isspace(current_screen->input_buffer[current_screen->buffer_pos - 1])
	 ||
	 ispunct(current_screen->input_buffer[current_screen->buffer_pos - 1])))
		current_screen->buffer_pos--;
	while ((current_screen->buffer_pos > current_screen->buffer_min_pos) &&
	 !(ispunct(current_screen->input_buffer[current_screen->buffer_pos - 1])
	 ||
	 isspace(current_screen->input_buffer[current_screen->buffer_pos - 1])))
		current_screen->buffer_pos--;
	update_input(UPDATE_JUST_CURSOR);
}

/* input_delete_character: deletes a character from the input line */
void
input_delete_character()
{
	cursor_to_input();
	if (current_screen->input_buffer[current_screen->buffer_pos])
	{
		char	*ptr = (char *) 0;
		int	pos;

		malloc_strcpy(&ptr,
		  &(current_screen->input_buffer[current_screen->buffer_pos
		  + 1]));
		strcpy(&(current_screen->input_buffer[
			current_screen->buffer_pos]), ptr);
		new_free(&ptr);
		if (term_delete())
			update_input(UPDATE_FROM_CURSOR);
		else
		{
			pos = str_start + CO - 1;
			if (pos < strlen(current_screen->input_buffer))
			{
				term_move_cursor(CO - 1, input_line);
				term_putchar(current_screen->input_buffer[pos]);
				term_move_cursor(cursor, input_line);
			}
			update_input(NO_UPDATE);
		}
	}
}

/* input_backspace: does a backspace in the input buffer */
/*ARGSUSED*/
void
input_backspace(key, ptr)
	char	*key;
	void	(*ptr)();
{
	cursor_to_input();
	if (current_screen->buffer_pos > current_screen->buffer_min_pos)
	{
		char	*ptr = (char *) 0;
		int	pos;

		malloc_strcpy(&ptr,
		  &(current_screen->input_buffer[current_screen->buffer_pos]));
		strcpy(&(current_screen->input_buffer[current_screen->buffer_pos
			- 1]), ptr);
		new_free(&ptr);
		current_screen->buffer_pos--;
		if (term_cursor_left())
			term_move_cursor(cursor - 1, input_line);
		if (current_screen->input_buffer[current_screen->buffer_pos])
		{
			if (term_delete())
			{
				update_input(UPDATE_FROM_CURSOR);
				return;
			}
			else
			{
				pos = str_start + CO - 1;
				if (pos < strlen(current_screen->input_buffer))
				{
					term_move_cursor(CO - 1, input_line);
				term_putchar(current_screen->input_buffer[pos]);
				}
				update_input(UPDATE_JUST_CURSOR);
			}
		}
		else
		{
			term_putchar(' ');
			if (term_cursor_left())
				term_move_cursor(cursor - 1, input_line);
			update_input(NO_UPDATE);
		}
	}
}

/*
 * input_beginning_of_line: moves the input cursor to the first character in
 * the input buffer 
 */
void
input_beginning_of_line()
{
	cursor_to_input();
	current_screen->buffer_pos = current_screen->buffer_min_pos;
	update_input(UPDATE_JUST_CURSOR);
}

/*
 * input_end_of_line: moves the input cursor to the last character in the
 * input buffer 
 */
void
input_end_of_line()
{
	cursor_to_input();
	current_screen->buffer_pos = strlen(current_screen->input_buffer);
	update_input(UPDATE_JUST_CURSOR);
}

/*
 * input_delete_previous_word: deletes from the cursor backwards to the next
 * space character. 
 */
void
input_delete_previous_word()
{
	int	old_pos;
	char	c;

	cursor_to_input();
	old_pos = current_screen->buffer_pos;
	while ((current_screen->buffer_pos > current_screen->buffer_min_pos) &&
	 (isspace(current_screen->input_buffer[current_screen->buffer_pos - 1])
	 ||
	 ispunct(current_screen->input_buffer[current_screen->buffer_pos - 1])))
		current_screen->buffer_pos--;
	while ((current_screen->buffer_pos > current_screen->buffer_min_pos) &&
	 !(ispunct(current_screen->input_buffer[current_screen->buffer_pos - 1])
	 ||
	 isspace(current_screen->input_buffer[current_screen->buffer_pos - 1])))
		current_screen->buffer_pos--;
	c = current_screen->input_buffer[old_pos];
	current_screen->input_buffer[old_pos] = (char) 0;
	malloc_strcpy(&cut_buffer,
		&(current_screen->input_buffer[current_screen->buffer_pos]));
	current_screen->input_buffer[old_pos] = c;
	strcpy(&(current_screen->input_buffer[current_screen->buffer_pos]),
		&(current_screen->input_buffer[old_pos]));
	update_input(UPDATE_FROM_CURSOR);
}

/*
 * input_delete_next_word: deletes from the cursor to the end of the next
 * word 
 */
void
input_delete_next_word()
{
	int	pos;
	char	*ptr = (char *) 0,
		c;

	cursor_to_input();
	pos = current_screen->buffer_pos;
	while ((isspace(current_screen->input_buffer[pos]) ||
	    ispunct(current_screen->input_buffer[pos])) &&
	    current_screen->input_buffer[pos])
		pos++;
	while (!(ispunct(current_screen->input_buffer[pos]) ||
	    isspace(current_screen->input_buffer[pos])) &&
	    current_screen->input_buffer[pos])
		pos++;
	c = current_screen->input_buffer[pos];
	current_screen->input_buffer[pos] = (char) 0;
	malloc_strcpy(&cut_buffer,
		&(current_screen->input_buffer[current_screen->buffer_pos]));
	current_screen->input_buffer[pos] = c;
	malloc_strcpy(&ptr, &(current_screen->input_buffer[pos]));
	strcpy(&(current_screen->input_buffer[current_screen->buffer_pos]),			ptr);
	new_free(&ptr);
	update_input(UPDATE_FROM_CURSOR);
}

/*
 * input_add_character: adds the character c to the input buffer, repecting
 * the current overwrite/insert mode status, etc 
 */
/*ARGSUSED*/
void
input_add_character(c, unused)
	char	c;
	char	*unused;
{
	int	display_flag = NO_UPDATE;

	cursor_to_input();
	if (current_screen->buffer_pos < INPUT_BUFFER_SIZE)
	{
		if (get_int_var(INSERT_MODE_VAR))
		{
			if (current_screen->input_buffer[
					current_screen->buffer_pos])
			{
				char	*ptr = (char *) 0;

				malloc_strcpy(&ptr, &(
					current_screen->input_buffer[
					current_screen->buffer_pos]));
				current_screen->input_buffer[
					current_screen->buffer_pos] = c;
				current_screen->input_buffer[
					current_screen->buffer_pos + 1] =
					(char) 0;
				strmcat(current_screen->input_buffer, ptr,
					INPUT_BUFFER_SIZE);
				new_free(&ptr);
				if (term_insert(c))
				{
					term_putchar(c);
					if (current_screen->input_buffer[
						current_screen->buffer_pos + 1])
					    display_flag = UPDATE_FROM_CURSOR;
					else
					    display_flag = NO_UPDATE;
				}
			}
			else
			{
				current_screen->input_buffer[
					current_screen->buffer_pos] = c;
				current_screen->input_buffer[
					current_screen->buffer_pos + 1] =
					(char) 0;
				term_putchar(c);
			}
		}
		else
		{
			if (current_screen->input_buffer[
				    current_screen->buffer_pos] == (char) 0)
				current_screen->input_buffer[
				   current_screen->buffer_pos + 1] = (char) 0;
			current_screen->input_buffer[current_screen->buffer_pos]
				= c;
			term_putchar(c);
		}
		current_screen->buffer_pos++;
		update_input(display_flag);
	}
}

/*
 * set_input: sets the input buffer to the given string, discarding whatever
 * was in the input buffer before 
 */
void
set_input(str)
	char	*str;
{
	strmcpy(current_screen->input_buffer + current_screen->buffer_min_pos,
		str, INPUT_BUFFER_SIZE - current_screen->buffer_min_pos);
	current_screen->buffer_pos = strlen(current_screen->input_buffer);
}

/*
 * get_input: returns a pointer to the input buffer.  Changing this will
 * actually change the input buffer.  This is a bad way to change the input
 * buffer tho, cause no bounds checking won't be done 
 */
char	*
get_input()
{
	return (&(current_screen->input_buffer[current_screen->buffer_min_pos]));
}

/* input_clear_to_eol: erases from the cursor to the end of the input buffer */
void
input_clear_to_eol()
{
	cursor_to_input();
	malloc_strcpy(&cut_buffer,
		&(current_screen->input_buffer[current_screen->buffer_pos]));
	current_screen->input_buffer[current_screen->buffer_pos] = (char) 0;
	if (term_clear_to_eol())
	{
		term_space_erase(cursor);
		term_move_cursor(cursor, input_line);
	}
	update_input(NO_UPDATE);
}

/*
 * input_clear_to_bol: clears from the cursor to the beginning of the input
 * buffer 
 */
void
input_clear_to_bol()
{
	char	*ptr = (char *) 0;

	cursor_to_input();
	malloc_strcpy(&cut_buffer,
	    &(current_screen->input_buffer[current_screen->buffer_min_pos]));
	cut_buffer[current_screen->buffer_pos -
		current_screen->buffer_min_pos] = (char) 0;
	malloc_strcpy(&ptr,
		&(current_screen->input_buffer[current_screen->buffer_pos]));
	current_screen->input_buffer[current_screen->buffer_min_pos] =
		(char) 0;
	strmcat(current_screen->input_buffer, ptr, INPUT_BUFFER_SIZE);
	new_free(&ptr);
	current_screen->buffer_pos = current_screen->buffer_min_pos;
	term_move_cursor(current_screen->buffer_min_pos, input_line);
	if (term_clear_to_eol())
	{
		term_space_erase(current_screen->buffer_min_pos);
		term_move_cursor(current_screen->buffer_min_pos, input_line);
	}
	update_input(UPDATE_FROM_CURSOR);
}

/*
 * input_clear_line: clears entire input line
 */
void
input_clear_line()
{
	cursor_to_input();
	malloc_strcpy(&cut_buffer, current_screen->input_buffer +
		current_screen->buffer_min_pos);
	current_screen->input_buffer[current_screen->buffer_min_pos] =
		(char) 0;
	current_screen->buffer_pos = current_screen->buffer_min_pos;
	term_move_cursor(current_screen->buffer_min_pos, input_line);
	if (term_clear_to_eol())
	{
		term_space_erase(current_screen->buffer_min_pos);
		term_move_cursor(current_screen->buffer_min_pos, input_line);
	}
	update_input(NO_UPDATE);
}

/*
 * input_transpose_characters: swaps the positions of the two characters
 * before the cursor position 
 */
void
input_transpose_characters()
{
	cursor_to_input();
	if (current_screen->buffer_pos > current_screen->buffer_min_pos)
	{
		u_char	c1, c2;
		int	pos, end_of_line = 0;

		if (current_screen->input_buffer[current_screen->buffer_pos])
			pos = current_screen->buffer_pos;
		else if (strlen(current_screen->input_buffer) > current_screen->buffer_min_pos + 2)
		{
			pos = current_screen->buffer_pos - 1;
			end_of_line = 1;
		}
		else
			return;

		c1 = current_screen->input_buffer[pos];
		c2 = current_screen->input_buffer[pos] = current_screen->input_buffer[pos - 1];
		current_screen->input_buffer[pos - 1] = c1;
		if (term_cursor_left() || end_of_line && term_cursor_left())
			term_move_cursor(cursor - end_of_line ? 2 : 1, input_line);
		term_putchar(c1);
		term_putchar(c2);
		if (!end_of_line && term_cursor_left())
			term_move_cursor(cursor - 1, input_line);
		update_input(NO_UPDATE);
	}
}

/* init_input: initialized the input buffer by clearing it out */
void
init_input()
{
	*current_screen->input_buffer = (char) 0;
	current_screen->buffer_pos = current_screen->buffer_min_pos;
}

/*
 * input_yank_cut_buffer: takes the contents of the cut buffer and inserts it
 * into the input line 
 */
void
input_yank_cut_buffer()
{
	char	*ptr = (char *) 0;

	if (cut_buffer)
	{
		malloc_strcpy(&ptr,
		   &(current_screen->input_buffer[current_screen->buffer_pos]));
		current_screen->input_buffer[current_screen->buffer_pos] =
			(char) 0;
		strmcat(current_screen->input_buffer, cut_buffer,
			INPUT_BUFFER_SIZE);
		strmcat(current_screen->input_buffer, ptr, INPUT_BUFFER_SIZE);
		new_free(&ptr);
		update_input(UPDATE_FROM_CURSOR);
		current_screen->buffer_pos += strlen(cut_buffer);
		if (current_screen->buffer_pos > INPUT_BUFFER_SIZE)
			current_screen->buffer_pos = INPUT_BUFFER_SIZE;
		update_input(UPDATE_JUST_CURSOR);
	}
}

/* get_input_prompt: returns the current input_prompt */
char	*
get_input_prompt()
{ 
	return (input_prompt); 
}

/*
 * set_input_prompt: sets a prompt that will be displayed in the input
 * buffer.  This prompt cannot be backspaced over, etc.  It's a prompt.
 * Setting the prompt to null uses no prompt 
 */
void
set_input_prompt(prompt)
	char	*prompt;
{
	if (prompt)
	{
		if (input_prompt && !strcmp (prompt, input_prompt))
			return;
		malloc_strcpy(&input_prompt, prompt);
	}
	else
	{
		if (!input_prompt)
			return;
		malloc_strcpy(&input_prompt, empty_string);
	}
	update_input(UPDATE_ALL);
}

/*
 * input_pause: prompt the user with a message then waits for a single
 * keystroke before continuing.  The key hit is returned. 
 *
 * This function is NEVER to be called, once the inital irc_io()
 * loop has been called from main().  Perhaps it would be better
 * to just put this code at the only place it is called, and not
 * have the function.  If you want an input_pause, use add_wait_prompt()
 * with WAIT_PROMPT_KEY.
 */
char
input_pause(msg)
	char	*msg;
{
	char	*ptr = (char *) 0;
	char	c;

	if (dumb)
	{
		puts(msg);
		fflush(stdout);
		if (use_input)
			irc_io(&c, NULL, -1, 1);
	}
	else
	{
		malloc_strcpy(&ptr, get_input());
		set_input(msg);
		update_input(UPDATE_ALL);
		irc_io(&c, NULL, -1, 1);
		set_input(ptr);
		update_input(UPDATE_ALL);
		new_free(&ptr);
	}
	return (c);
}
