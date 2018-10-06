/*
 * lastlog.c: handles the lastlog features of irc. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: lastlog.c,v 1.1.1.1.2.1 2002/03/11 19:24:04 toast Exp $";
#endif

#include "irc.h"

#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "vars.h"
#include "ircaux.h"
#include "output.h"

/*
 * lastlog_level: current bitmap setting of which things should be stored in
 * the lastlog.  The LOG_MSG, LOG_NOTICE, etc., defines tell more about this 
 */
static	int	lastlog_level;
static	int	notify_level;

/*
 * msg_level: the mask for the current message level.  What?  Did he really
 * say that?  This is set in the set_lastlog_msg_level() routine as it
 * compared to the lastlog_level variable to see if what ever is being added
 * should actually be added 
 */
static	int	msg_level = LOG_CRAP;

#define NUMBER_OF_LEVELS 16
static	char	*levels[] =
{
	"CRAP",		"PUBLIC",	"MSGS",		"NOTICES",
	"WALLS",	"WALLOPS",	"NOTES",	"OPNOTES",
	"SNOTES",	"ACTIONS",	"DCC",		"CTCP",
	"USERLOG1",	"USERLOG2",	"USERLOG3",	"USERLOG4"
};

/*
 * bits_to_lastlog_level: converts the bitmap of lastlog levels into a nice
 * string format.  Note that this uses the global buffer, so watch out 
 */
char	*
bits_to_lastlog_level(level)
	int	level;
{
	static	char	buffer[81]; /* this *should* be enough for this */
	int	i,
		p;

	if (level == LOG_ALL)
		strcpy(buffer, "ALL");
	else if (level == 0)
		strcpy(buffer, "NONE");
	else
	{
		*buffer = '\0';
		for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++, p *= 2)
		{
			if (level & p)
			{
				strmcat(buffer, levels[i],80);
				strmcat(buffer, " ",80);
			}
		}
	}
	return (buffer);
}

int
parse_lastlog_level(str, logtype)
	char	*str;
	char	*logtype;
{
	char	*ptr,
		*rest;
	int	len,
		i,
		p,
		level,
		neg;

	level = 0;
	while ((str = next_arg(str, &rest)) != NULL)
	{
		while (str)
		{
			if ((ptr = index(str, ',')) != NULL)
				*ptr++ = '\0';
			if ((len = strlen(str)) != 0)
			{
				if (my_strnicmp(str, "ALL", len) == 0)
					level = LOG_ALL;
				else if (my_strnicmp(str, "NONE", len) == 0)
					level = 0;
				else
				{
					if (*str == '-')
					{
						str++;
						neg = 1;
					}
					else
						neg = 0;
					for (i = 0, p = 1; i < NUMBER_OF_LEVELS;
							i++, p *= 2)
					{
						if (!my_strnicmp(str, levels[i],
								len))
						{
							if (neg)
								level &=
								  (LOG_ALL ^ p);
							else
								level |= p;
							break;
						}
					}
					if (i == NUMBER_OF_LEVELS)
						say("Unknown %s level: %s",
							logtype, str);
				}
			}
			str = ptr;
		}
		str = rest;
	}
	return (level);
}

/*
 * set_lastlog_level: called whenever a "SET LASTLOG_LEVEL" is done.  It
 * parses the settings and sets the lastlog_level variable appropriately.  It
 * also rewrites the LASTLOG_LEVEL variable to make it look nice 
 */
void
set_lastlog_level(str)
	char	*str;
{
	lastlog_level = parse_lastlog_level(str, "LASTLOG");
	set_string_var(LASTLOG_LEVEL_VAR, bits_to_lastlog_level(lastlog_level));
	curr_scr_win->lastlog_level = lastlog_level;
}

static	void
remove_from_lastlog(window)
	Window	*window;
{
	Lastlog *tmp;

	if (window->lastlog_tail)
	{
		tmp = window->lastlog_tail->prev;
		new_free(&window->lastlog_tail->msg);
		new_free(&window->lastlog_tail);
		window->lastlog_tail = tmp;
		if (tmp)
			tmp->next = (Lastlog *) 0;
		else
			window->lastlog_head = window->lastlog_tail;
		window->lastlog_size--;
	}
	else
		window->lastlog_size = 0;
}

/*
 * set_lastlog_size: sets up a lastlog buffer of size given.  If the lastlog
 * has gotten larger than it was before, all previous lastlog entry remain.
 * If it get smaller, some are deleted from the end. 
 */
void
set_lastlog_size(size)
	int	size;
{
	int	i,
		diff;

	if (curr_scr_win->lastlog_size > size)
	{
		diff = curr_scr_win->lastlog_size - size;
		for (i = 0; i < diff; i++)
			remove_from_lastlog(curr_scr_win);
	}
}

/*
 * lastlog: the /LASTLOG command.  Displays the lastlog to the screen. If
 * args contains a valid integer, only that many lastlog entries are shown
 * (if the value is less than lastlog_size), otherwise the entire lastlog is
 * displayed 
 */
/*ARGSUSED*/
void
lastlog(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	cnt,
		from = 0,
		p,
		i,
		level = 0,
		msg_level,
		len,
		mask = 0,
		header = 1;
	Lastlog *start_pos;
	char	*match = NULL,
		*arg;

	message_from((char *) 0, LOG_CURRENT);
	cnt = curr_scr_win->lastlog_size;

	while ((arg = next_arg(args, &args)) != NULL)
	{
		if (*arg == '-')
		{
			arg++;
			if (!(len = strlen(arg)))
				header = 0;
			else if (!my_strnicmp(arg, "LITERAL", len))
			{
				if (match)
				{
					say("Second -LITERAL argument ignored");
					(void) next_arg(args, &args);
					continue;
				}
				if ((match = next_arg(args, &args)) != NULL)
					continue;
				say("Need pattern for -LITERAL");
				return;
			}
			else
			{
				for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++,
						p *= 2)
				{
					if (my_strnicmp(levels[i], arg, len) == 0)
					{
						mask |= p;
						break;
					}
				}
				if (i == NUMBER_OF_LEVELS)
				{
					say("Unknown flag: %s", arg);
					message_from((char *) 0, LOG_CRAP);
					return;
				}
			}
		}
		else
		{
			if (level == 0)
			{
				if (match || isdigit(*arg))
				{
					cnt = atoi(arg);
					level++;
				}
				else
					match = arg;
			}
			else if (level == 1)
			{
				from = atoi(arg);
				level++;
			}
		}
	}
	start_pos = curr_scr_win->lastlog_head;
	for (i = 0; (i < from) && start_pos; start_pos = start_pos->next)
		if (!mask || (mask & start_pos->level))
			i++;

	for (i = 0; (i < cnt) && start_pos; start_pos = start_pos->next)
		if (!mask || (mask & start_pos->level))
			i++;

	level = curr_scr_win->lastlog_level;
	msg_level = set_lastlog_msg_level(0);
	if (start_pos == (Lastlog *) 0)
		start_pos = curr_scr_win->lastlog_tail;
	else
		start_pos = start_pos->prev;

	/* Let's not get confused here, display a seperator.. -lynx */
	if (header)
		say("Lastlog:");
	for (i = 0; (i < cnt) && start_pos; start_pos = start_pos->prev)
	{
		if (!mask || (mask & start_pos->level))
		{
			i++;
			if (match)
			{
				if (scanstr(start_pos->msg, match))
					put_it("%s", start_pos->msg);
			}
			else
				put_it("%s", start_pos->msg);
		}
	}
	if (header)
		say("End of Lastlog");
	curr_scr_win->lastlog_level = level;
	set_lastlog_msg_level(msg_level);
	message_from((char *) 0, LOG_CRAP);
}

/* set_lastlog_msg_level: sets the message level for recording in the lastlog */
int
set_lastlog_msg_level(level)
	int	level;
{
	int	old;

	old = msg_level;
	msg_level = level;
	return (old);
}

/*
/* get_lastlog_msg_level: gets the message level for recording in the lastlog */
int
get_lastlog_msg_level(void)
{
	return (msg_level);
}

/*
 * add_to_lastlog: adds the line to the lastlog.  If the LASTLOG_CONVERSATION
 * variable is on, then only those lines that are user messages (private
 * messages, channel messages, wall's, and any outgoing messages) are
 * recorded, otherwise, everything is recorded 
 */
void
add_to_lastlog(window, line)
	Window	*window;
	char	*line;
{
	Lastlog *new;

	if (window == (Window *) 0)
		window = curr_scr_win;
	if (window->lastlog_level & msg_level)
	{
		/* no nulls or empty lines (they contain "> ") */
		if (line && (strlen(line) > 2))
		{
			new = (Lastlog *) new_malloc(sizeof(Lastlog));
			new->next = window->lastlog_head;
			new->prev = (Lastlog *) 0;
			new->level = msg_level;
			new->msg = (char *) 0;
			malloc_strcpy(&(new->msg), line);

			if (window->lastlog_head)
				window->lastlog_head->prev = new;
			window->lastlog_head = new;

			if (window->lastlog_tail == (Lastlog *) 0)
				window->lastlog_tail = window->lastlog_head;

			if (window->lastlog_size++ == get_int_var(LASTLOG_VAR))
				remove_from_lastlog(window);
		}
	}
}

int
islogged(window)
	Window	*window;
{
	return (window->lastlog_level & msg_level) ? 1 : 0;
}

int
real_notify_level()
{
	return (notify_level);
}

int
real_lastlog_level()
{
	return (lastlog_level);
}

void
set_notify_level(str)
	char	*str;
{
	notify_level = parse_lastlog_level(str, "NOTIFY");
	set_string_var(NOTIFY_LEVEL_VAR, bits_to_lastlog_level(notify_level));
	curr_scr_win->notify_level = notify_level;
}
