/*
 * ignore.c: handles the ingore command for irc 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: ignore.c,v 1.3 2001/11/08 19:56:36 toast Exp $";
#endif

#include "irc.h"

#include "ignore.h"
#include "ircaux.h"
#include "list.h"
#include "vars.h"
#include "output.h"

#define NUMBER_OF_IGNORE_LEVELS 9

#define IGNORE_REMOVE 1
#define IGNORE_DONT 2
#define IGNORE_HIGH -1

int	ignore_usernames = 0;
char	highlight_char = '\0';
static	int	ignore_usernames_sums[NUMBER_OF_IGNORE_LEVELS] =
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static	int	remove_ignore();

/*
 * Ignore: the ignore list structure,  consists of the nickname, and the type
 * of ignorance which is to take place 
 */
typedef struct	IgnoreStru
{
	struct	IgnoreStru *next;
	char	*nick;
	int	type;
	int	dont;
	int	high;
	int	warned; /* has user been warned of ignore? -Toast */
	time_t	start;
	int	timeout;
}	Ignore;

/* ignored_nicks: pointer to the head of the ignore list */
static	Ignore *ignored_nicks = NULL;

static	int
ignore_usernames_mask(mask, thing)
	int	mask;
	int	thing;
{
	int	i;
	int	p;

	for (i = 0, p = 1; i < NUMBER_OF_IGNORE_LEVELS; i++, p *= 2)
		if (mask & p)
			ignore_usernames_sums[i] += thing;

	mask = 0;
	for (i = 0, p = 1; i < NUMBER_OF_IGNORE_LEVELS; i++, p *= 2)
		if (ignore_usernames_sums[i])
			mask += p;

	return (mask);
}

/*
 * ignore_nickname: adds nick to the ignore list, using type as the type of
 * ignorance to take place.  
 */
static	void
ignore_nickname(nick, type, flag)
	char	*nick;
	int	type;
	int	flag;
{
	Ignore	*new;
	char	*msg,
		*ptr;

	while (nick)
	{
		if ((ptr = index(nick, ',')) != NULL)
			*ptr = '\0';
		if (index(nick, '@'))
			ignore_usernames = ignore_usernames_mask(type, 1);
		if (*nick)
		{
			if (!(new = (Ignore *) list_lookup(&ignored_nicks, nick,
					!USE_WILDCARDS, !REMOVE_FROM_LIST)))
			{
				if (flag == IGNORE_REMOVE)
				{
					say("%s is not on the ignorance list",
							nick);
					if (ptr)
						*(ptr++) = ',';
					nick = ptr;
					continue;
				}
				else
				{
					if ((new = (Ignore *)
						remove_from_list(&ignored_nicks,
						nick)) != NULL)
					{
						new_free(&(new->nick));
						new_free(&new);
					}
					new = (Ignore *)
						new_malloc(sizeof(Ignore));
					new->nick = (char *) 0;
					new->type = 0;
					new->dont = 0;
					new->high = 0;
					new->warned = 0;
					new->start = (time_t) 0;
					new->timeout = 0;
					malloc_strcpy(&(new->nick), nick);
					upper(new->nick);
					add_to_list(&ignored_nicks, new);
				}
			}
			if (type == IGNORE_TIME) {
				if (flag) {
					new->start = time(NULL);
					new->timeout = flag;
					say("Ignore on %s will timeout in %d minute%s",
						new->nick, flag, (flag==1)?"":"s");
				} else {
					new->start = 0;
					new->timeout = 0;
					say("Ignore on %s will not timeout", new->nick);
				}
				return;
			}
			switch (flag)
			{
			case IGNORE_REMOVE:
				new->type &= (~type);
				new->high &= (~type);
				new->dont &= (~type);
				msg = "Not ignoring";
				break;
			case IGNORE_DONT:
				new->dont |= type;
				new->type &= (~type);
				new->high &= (~type);
				msg = "Never ignoring";
				break;
			case IGNORE_HIGH:
				new->high |= type;
				new->type &= (~type);
				new->dont &= (~type);
				msg = "Highlighting";
				break;
			default:
				new->type |= type;
				new->high &= (~type);
				new->dont &= (~type);
				msg = "Ignoring";
				break;
			}
			if (type == IGNORE_ALL)
			{
				switch (flag)
				{
				case IGNORE_REMOVE:
					say("%s removed from ignorance list",
							new->nick);
					remove_ignore(new->nick);
					break;
				case IGNORE_HIGH:
				    say("Highlighting ALL messages from %s",
					new->nick);
					break;
				case IGNORE_DONT:
				    say("Never ignoring messages from %s",
					new->nick);
					break;
				default:
				    say("Ignoring ALL messages from %s",
					new->nick);
					break;
				}
				return;
			}
			else if (type)
			{
				strcpy(buffer, msg);
				if (type & IGNORE_MSGS)
					strcat(buffer, " MSGS");
				if (type & IGNORE_PUBLIC)
					strcat(buffer, " PUBLIC");
				if (type & IGNORE_WALLS)
					strcat(buffer, " WALLS");
				if (type & IGNORE_WALLOPS)
					strcat(buffer, " WALLOPS");
				if (type & IGNORE_INVITES)
					strcat(buffer, " INVITES");
				if (type & IGNORE_NOTICES)
					strcat(buffer, " NOTICES");
				if (type & IGNORE_NOTES)
					strcat(buffer, " NOTES");
				if (type & IGNORE_CTCPS)
					strcat(buffer, " CTCPS");
				if (type & IGNORE_CRAP)
					strcat(buffer, " CRAP");
				say("%s from %s", buffer, new->nick);
			}
			if ((new->type == 0) && (new->high == 0))
				remove_ignore(new->nick);
		}
		if (ptr)
			*(ptr++) = ',';
		nick = ptr;
	}
}

/*
 * remove_ignore: removes the given nick from the ignore list and returns 0.
 * If the nick wasn't in the ignore list to begin with, 1 is returned. 
 */
static	int
remove_ignore(nick)
	char	*nick;
{
	Ignore	*tmp;

	if ((tmp = (Ignore *) list_lookup(&ignored_nicks, nick, !USE_WILDCARDS,
			REMOVE_FROM_LIST)) != NULL)
	{
		if (index(nick, '@'))
			ignore_usernames = ignore_usernames_mask(tmp->type, -1);
		new_free(&(tmp->nick));
		new_free(&tmp);
		return (0);
	}
	return (1);
}

/*
 * is_ignored: checks to see if nick is being ignored (poor nick).  Checks
 * against type to see if ignorance is to take place.  If nick is marked as
 * IGNORE_ALL or ignorace types match, 1 is returned, otherwise 0 is
 * returned.  
 */
int
is_ignored(nick, type)
	char	*nick;
	int	type;
{
	Ignore	*tmp;

	if (ignored_nicks)
	{
		if ((tmp = (Ignore *) list_lookup(&ignored_nicks, nick,
				USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
		{
			if (tmp->dont & type)
				return(DONT_IGNORE);
			if (tmp->type & type)
				return (IGNORED);
			if (tmp->high & type)
				return (HIGHLIGHTED);
		}
	}
	return (0);
}

/* ignore_list: shows the entired ignorance list */
void
ignore_list(nick)
	char	*nick;
{
	Ignore	*tmp;
	int	len = 0;

	if (ignored_nicks)
	{
		say("Ignorance List:");
		if (nick)
		{
			len = strlen(nick);
			upper(nick);
		}
		for (tmp = ignored_nicks; tmp; tmp = tmp->next)
		{
			char	s[BIG_BUFFER_SIZE];

			if (nick)
			{
				if (strncmp(nick, tmp->nick, len))
					continue;
			}
			*buffer = (char) 0;
			if (tmp->start) {
				time_t now = time(NULL);
				int diff = tmp->timeout-((now-tmp->start)/60);
				sprintf(s, " %dm", diff);
				strmcat(buffer, s, BIG_BUFFER_SIZE);
			}
			if (tmp->type == IGNORE_ALL)
				strmcat(buffer," ALL",BIG_BUFFER_SIZE);
			else if (tmp->high == IGNORE_ALL)
			{
				sprintf(s, " %cALL%c", highlight_char, 
					highlight_char);
				strmcat(buffer, s, BIG_BUFFER_SIZE);
			}
			else if (tmp->dont == IGNORE_ALL)
				strmcat(buffer," DONT-ALL", BIG_BUFFER_SIZE);
			else
			{
				if (tmp->type & IGNORE_PUBLIC)
					strmcat(buffer, " PUBLIC",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_PUBLIC)
				{
					sprintf(s, " %cPUBLIC%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_PUBLIC)
					strmcat(buffer, " DONT-PUBLIC",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_MSGS)
					strmcat(buffer, " MSGS",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_MSGS)
				{
					sprintf(s, " %cMSGS%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_MSGS)
					strmcat(buffer, " DONT-MSGS",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_WALLS)
					strmcat(buffer, " WALLS",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_WALLS)
				{
					sprintf(s, " %cWALLS%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_WALLS)
					strmcat(buffer, " DONT-WALLS",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_WALLOPS)
					strmcat(buffer, " WALLOPS",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_WALLOPS)
				{
					sprintf(s, " %cWALLOPS%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_WALLOPS)
					strmcat(buffer, " DONT-WALLOPS",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_INVITES)
					strmcat(buffer, " INVITES",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_INVITES)
				{
					sprintf(s, " %cINVITES%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_INVITES)
					strmcat(buffer, " DONT-INVITES",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_NOTICES)
					strmcat(buffer, " NOTICES",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_NOTICES)
				{
					sprintf(s, " %cNOTICES%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_NOTICES)
					strmcat(buffer, " DONT-NOTICES",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_NOTES)
					strmcat(buffer, " NOTES",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_NOTES)
				{
					sprintf(s, " %cNOTES%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_NOTES)
					strmcat(buffer, " DONT-NOTES",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_CTCPS)
					strmcat(buffer, " CTCPS",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_CTCPS)
				{
					sprintf(s, " %cCTCPS%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_CTCPS)
					strmcat(buffer, " DONT-CTCPS",
							BIG_BUFFER_SIZE);
				if (tmp->type & IGNORE_CRAP)
					strmcat(buffer, " CRAP",
							BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_CRAP)
				{
					sprintf(s, " %cCRAP%c",
						highlight_char, highlight_char);
					strmcat(buffer, s, BIG_BUFFER_SIZE);
				}
				else if (tmp->dont & IGNORE_CRAP)
					strmcat(buffer, " DONT-CRAP",
							BIG_BUFFER_SIZE);
			}
			say("%c\t%s:\t%s", tmp->warned?'+':'-', tmp->nick, buffer);
		}
	}
	else
		say("There are no nicknames being ignored");
}

/*
 * ignore: does the /IGNORE command.  Figures out what type of ignoring the
 * user wants to do and calls the proper ignorance command to do it. 
 */
/*ARGSUSED*/
void
ignore(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*nick,
		*type;
	int	len, val;
	int	flag,
		no_flags;

	if ((nick = next_arg(args, &args)) != NULL)
	{
		no_flags = 1;
		while ((type = next_arg(args, &args)) != NULL)
		{
			no_flags = 0;
			upper(type);
			switch (*type)
			{
			case '^':
				flag = IGNORE_DONT;
				type++;
				break;
			case '-':
				flag = IGNORE_REMOVE;
				type++;
				break;
			case '+':
				flag = IGNORE_HIGH;
				type++;
				break;
			default:
				flag = 0;
				break;
			}
			if ((len = strlen(type)) == 0)
			{
				say("You must specify one of the following:");
				say("\tALL MSGS PUBLIC WALLS WALLOPS INVITES NOTICES NOTES NONE");
				say("or an ignore time (in minutes)");
				return;
			}
			if (is_number(type)) 
				ignore_nickname(nick, IGNORE_TIME, atoi(type));
			else if (strncmp(type, "ALL", len) == 0)
				ignore_nickname(nick, IGNORE_ALL, flag);
			else if (strncmp(type, "MSGS", len) == 0)
				ignore_nickname(nick, IGNORE_MSGS, flag);
			else if (strncmp(type, "PUBLIC", len) == 0)
				ignore_nickname(nick, IGNORE_PUBLIC, flag);
			else if (strncmp(type, "WALLS", len) == 0)
				ignore_nickname(nick, IGNORE_WALLS, flag);
			else if (strncmp(type, "WALLOPS", len) == 0)
				ignore_nickname(nick, IGNORE_WALLOPS, flag);
			else if (strncmp(type, "INVITES", len) == 0)
				ignore_nickname(nick, IGNORE_INVITES, flag);
			else if (strncmp(type, "NOTICES", len) == 0)
				ignore_nickname(nick, IGNORE_NOTICES, flag);
			else if (strncmp(type, "NOTES", len) == 0)
				ignore_nickname(nick, IGNORE_NOTES, flag);
			else if (strncmp(type, "CTCPS", len) == 0)
				ignore_nickname(nick, IGNORE_CTCPS, flag);
			else if (strncmp(type, "CRAP", len) == 0)
				ignore_nickname(nick, IGNORE_CRAP, flag);
			else if (strncmp(type, "NONE", len) == 0)
			{
				char	*ptr;

				while (nick)
				{
					if ((ptr = index(nick, ',')) != NULL)
						*ptr = (char) 0;
					if (*nick)
					{
						if (remove_ignore(nick))
				say("%s is not in the ignorance list!", nick);
						else
				say("%s removed from ignorance list", nick);
					}
					if (ptr)
						*(ptr++) = ',';
					nick = ptr;
				}
			}
			else
			{
				say("You must specify one of the following:");
				say("\tALL MSGS PUBLIC WALLS WALLOPS INVITES NOTICES NOTES CTCPS CRAP NONE");
				say("or an ignore time (in minutes)");
			}
		}
		if (no_flags)
			ignore_list(nick);
	} else
		ignore_list((char *) 0);
}

/*
 * set_highlight_char: what the name says..  the character to use
 * for highlighting..  either BOLD, INVERSE, BLINK, or UNDERLINE..
 */
void
set_highlight_char(s)
	char	*s;
{
	int	len;

	len = strlen(s);

	if (!my_strnicmp(s, "BOLD", len))
	{
		set_string_var(HIGHLIGHT_CHAR_VAR, "BOLD");
		highlight_char = BOLD_TOG;
	}
	else if (!my_strnicmp(s, "BLINK", len))
	{
		set_string_var(HIGHLIGHT_CHAR_VAR, "BLINK");
		highlight_char = BLINK_TOG;
	}
	else if (!my_strnicmp(s, "INVERSE", len))
	{
		set_string_var(HIGHLIGHT_CHAR_VAR, "INVERSE");
		highlight_char = REV_TOG;
	}
	else if (!my_strnicmp(s, "UNDERLINE", len))
	{
		set_string_var(HIGHLIGHT_CHAR_VAR, "UNDERLINE");
		highlight_char = UND_TOG;
	}
	else
		say("HIGHLIGHT_CHAR must be one of BOLD, INVERSE, BLINK or UNDERLINE");
}

int
ignore_combo(flag1, flag2)
	int	flag1;
	int	flag2;
{
        if (flag1 == DONT_IGNORE || flag2 == DONT_IGNORE)
                return DONT_IGNORE;
        if (flag1 == IGNORED || flag2 == IGNORED)
                return IGNORED;
        if (flag1 == HIGHLIGHTED || flag2 == HIGHLIGHTED)
                return HIGHLIGHTED;
        return 0;
}

/*
 * double_ignore - makes live simpiler when using doing ignore code
 * added, april 1993, phone.
 */
int
double_ignore(nick, userhost, type)
	char	*nick,
		*userhost;
	int	type;
{
	if (userhost)
		return (ignore_combo(is_ignored(nick, type),
			is_ignored(userhost, type)));
	else
		return (is_ignored(nick, type));
}

/*
 * Determines whether a user needs to be warned about being ignored or
 * not. 9 April 1998 -Toast
 */
int
do_warn(nick, userhost)
char	*nick,
	*userhost;
{
	int foo;
	Ignore *tmp;
	if (!get_int_var(SEND_IGNORE_MSG_VAR))
		return 0;
	tmp = (Ignore *) list_lookup(&ignored_nicks, nick, USE_WILDCARDS,
		!REMOVE_FROM_LIST);
	if (tmp == NULL)
		tmp = (Ignore *) list_lookup(&ignored_nicks, userhost,
			USE_WILDCARDS, !REMOVE_FROM_LIST);
	if (tmp != NULL)
	{
	  foo = tmp->warned;
	  tmp->warned = -1;
	  return !foo;
	} else
	  /* No reason to warn users who aren't being ignored! */
	  return 0;
}

void
IgnoreTimer()
{
	Ignore	*tmp, *this;
	time_t	now;
	char	*nick = (char *) 0;

	now = time(NULL);

	tmp = ignored_nicks;
	while (tmp) {
		this = tmp;
		tmp = tmp->next;
		if (this->start) {
			int diff = this->timeout - ((now - this->start)/60);
			if (diff <= 0) {
				say("%s removed from ignorance list", this->nick);
				remove_ignore(this->nick);
			}
		}
	}
}
