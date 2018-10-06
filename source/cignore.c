/*
 * cignore.c: handles the channel ignore commands
 *
 * Written By Timothy Jensen
 *
 * Copyright (c) 1999 
 *
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: cignore.c,v 1.3 2001/04/01 15:58:26 toast Exp $";
#endif

#include "irc.h"

#include "cignore.h"
#include "ircaux.h"
#include "list.h"
#include "vars.h"
#include "output.h"

#define CIGNORE_REMOVE 1

static	int	remove_cignore();

/*
 * CIgnore: the channel ignore list structure,  consists of the channel,
 * and the type of ignorance which is to take place 
 */
typedef struct	CIgnoreStru
{
	struct	CIgnoreStru *next;
	char	*channel;
	int	type;
}	CIgnore;

/* ignored_channels: pointer to the head of the ignore list */
static	CIgnore *ignored_channels = NULL;

/*
 * ignore_channel: adds channel to the ignore list, using type as the type of
 * ignorance to take place.  
 */
static	void
ignore_channel(channel, type, flag)
	char	*channel;
	int	type;
	int	flag;
{
	CIgnore	*new;
	char	*msg,
		*ptr;

	while (channel)
	{
		if ((ptr = index(channel, ',')) != NULL)
			*ptr = '\0';
		if (*channel)
		{
			if (!(new = (CIgnore *) list_lookup(&ignored_channels, channel,
					!USE_WILDCARDS, !REMOVE_FROM_LIST)))
			{
				if (flag == CIGNORE_REMOVE)
				{
					say("%s is not on the channel ignorance list",
						channel);
					if (ptr)
						*(ptr++) = ',';
					channel = ptr;
					continue;
				}
				else
				{
					if ((new = (CIgnore *)
						remove_from_list(&ignored_channels,
						channel)) != NULL)
					{
						new_free(&(new->channel));
						new_free(&new);
					}
					new = (CIgnore *)
						new_malloc(sizeof(CIgnore));
					new->channel = (char *) 0;
					new->type = 0;
					malloc_strcpy(&(new->channel), channel);
					upper(new->channel);
					add_to_list(&ignored_channels, new);
				}
			}
			switch (flag)
			{
			case CIGNORE_REMOVE:
				new->type &= (~type);
				msg = "Not ignoring";
				break;
			default:
				new->type |= type;
				msg = "Ignoring";
				break;
			}
			if (type == CIGNORE_ALL)
			{
				switch (flag)
				{
				case CIGNORE_REMOVE:
					say("%s removed from channel ignorance list",
							new->channel);
					remove_cignore(new->channel);
					break;
				default:
				    say("Ignoring ALL messages to %s",
					new->channel);
					break;
				}
				return;
			}
			else if (type)
			{
				strcpy(buffer, msg);
				if (type & CIGNORE_MSGS)
					strcat(buffer, " MSGS");
				if (type & CIGNORE_NOTICES)
					strcat(buffer, " NOTICES");
				if (type & CIGNORE_CTCPS)
					strcat(buffer, " CTCPS");
				if (type & CIGNORE_JOINS)
					strcat(buffer, " JOINS");
				if (type & CIGNORE_PARTS)
					strcat(buffer, " PARTS");
				if (type & CIGNORE_KICKS)
					strcat(buffer, " KICKS");
				if (type & CIGNORE_MODES)
					strcat(buffer, " MODES");
				if (type & CIGNORE_TOPICS)
					strcat(buffer, " TOPICS");
				if (type & CIGNORE_CRAP)
					strcat(buffer, " CRAP");
				say("%s to %s", buffer, new->channel);
			}
			if (new->type == 0)
				remove_cignore(new->channel);
		}
		if (ptr)
			*(ptr++) = ',';
		channel = ptr;
	}
}

/*
 * remove_cignore: removes the given channel from the channel ignore list and
 * returns 0.  If the channel wasn't in the ignore list to begin with, 1 is
 * returned. 
 */
static	int
remove_cignore(channel)
	char	*channel;
{
	CIgnore	*tmp;

	if ((tmp = (CIgnore *) list_lookup(&ignored_channels, channel, !USE_WILDCARDS,
			REMOVE_FROM_LIST)) != NULL)
	{
		new_free(&(tmp->channel));
		new_free(&tmp);
		return (0);
	}
	return (1);
}

/*
 * is_cignored: checks to see if channel is being ignored.  Checks against
 * type to see if ignorance is to take place.  If channel is marked as
 * CIGNORE_ALL or ignorace types match, 1 is returned, otherwise 0 is
 * returned.  
 */
int
is_cignored(channel, type)
	char	*channel;
	int	type;
{
	CIgnore	*tmp;

	if (ignored_channels)
	{
		if ((tmp = (CIgnore *) list_lookup(&ignored_channels, channel,
				USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
		{
			if (tmp->type & type)
				return (CIGNORED);
		}
	}
	return (0);
}

/* cignore_list: shows the entire channel ignorance list */
void
cignore_list(channel)
	char	*channel;
{
	CIgnore	*tmp;
	int	len = 0;

	if (ignored_channels)
	{
		say("Channel Ignorance List:");
		if (channel)
		{
			len = strlen(channel);
			upper(channel);
		}
		for (tmp = ignored_channels; tmp; tmp = tmp->next)
		{
			char	s[BIG_BUFFER_SIZE];

			if (channel)
			{
				if (strncmp(channel, tmp->channel, len))
					continue;
			}
			*buffer = (char) 0;
			if (tmp->type == CIGNORE_ALL)
				strmcat(buffer," ALL",BIG_BUFFER_SIZE);
			else
			{
				if (tmp->type & CIGNORE_MSGS)
					strmcat(buffer, " MSGS",
							BIG_BUFFER_SIZE);
				if (tmp->type & CIGNORE_NOTICES)
					strmcat(buffer, " NOTICES",
							BIG_BUFFER_SIZE);
				if (tmp->type & CIGNORE_CTCPS)
					strmcat(buffer, " CTCPS",
							BIG_BUFFER_SIZE);
				if (tmp->type & CIGNORE_JOINS)
					strmcat(buffer, " JOINS",
							BIG_BUFFER_SIZE);
				if (tmp->type & CIGNORE_PARTS)
					strmcat(buffer, " PARTS",
							BIG_BUFFER_SIZE);
				if (tmp->type & CIGNORE_KICKS)
					strmcat(buffer, " KICKS",
							BIG_BUFFER_SIZE);
				if (tmp->type & CIGNORE_MODES)
					strmcat(buffer, " MODES",
							BIG_BUFFER_SIZE);
				if (tmp->type & CIGNORE_TOPICS)
					strmcat(buffer, " TOPICS",
							BIG_BUFFER_SIZE);
				if (tmp->type & CIGNORE_CRAP)
					strmcat(buffer, " CRAP",
							BIG_BUFFER_SIZE);
			}
			say("\t%s:\t%s", tmp->channel, buffer);
		}
	}
	else
		say("There are no channels being ignored");
}

/*
 * cignore: does the /CCIGNORE command.  Figures out what type of ignoring
 * the user wants to do and calls the proper ignorance command to do it. 
 */
/*ARGSUSED*/
void
cignore(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*channel,
		*type;
	int	len, val;
	int	flag,
		no_flags;

	if ((channel = next_arg(args, &args)) != NULL)
	{
		no_flags = 1;
		while ((type = next_arg(args, &args)) != NULL)
		{
			no_flags = 0;
			upper(type);
			switch (*type)
			{
			case '-':
				flag = CIGNORE_REMOVE;
				type++;
				break;
			case '+':
				type++;
			default:
				flag = 0;
				break;
			}
			if ((len = strlen(type)) == 0)
			{
				say("You must specify one of the following:");
				say("\tALL MSGS NOTICES CTCPS JOINS PARTS KICKS MODES TOPICS NONE CRAP");
				return;
			}
			if (strncmp(type, "ALL", len) == 0)
				ignore_channel(channel, CIGNORE_ALL, flag);
			else if (strncmp(type, "MSGS", len) == 0)
				ignore_channel(channel, CIGNORE_MSGS, flag);
			else if (strncmp(type, "NOTICES", len) == 0)
				ignore_channel(channel, CIGNORE_NOTICES, flag);
			else if (strncmp(type, "CTCPS", len) == 0)
				ignore_channel(channel, CIGNORE_CTCPS, flag);
			else if (strncmp(type, "JOINS", len) == 0)
				ignore_channel(channel, CIGNORE_JOINS, flag);
			else if (strncmp(type, "PARTS", len) == 0)
				ignore_channel(channel, CIGNORE_PARTS, flag);
			else if (strncmp(type, "KICKS", len) == 0)
				ignore_channel(channel, CIGNORE_KICKS, flag);
			else if (strncmp(type, "MODES", len) == 0)
				ignore_channel(channel, CIGNORE_MODES, flag);
			else if (strncmp(type, "TOPICS", len) == 0)
				ignore_channel(channel, CIGNORE_TOPICS, flag);
			else if (strncmp(type, "CRAP", len) == 0)
				ignore_channel(channel, CIGNORE_CRAP, flag);
			else if (strncmp(type, "NONE", len) == 0)
			{
				char	*ptr;

				while (channel)
				{
					if ((ptr = index(channel, ',')) != NULL)
						*ptr = (char) 0;
					if (*channel)
					{
						if (remove_cignore(channel))
				say("%s is not in the channel ignorance list!", channel);
						else
				say("%s removed from channel ignorance list", channel);
					}
					if (ptr)
						*(ptr++) = ',';
					channel = ptr;
				}
			}
			else
			{
				say("You must specify one of the following:");
				say("\tALL MSGS NOTICES CTCPS JOINS PARTS KICKS MODES TOPICS CRAP NONE");
			}
		}
		if (no_flags)
			cignore_list(channel);
	} else
		cignore_list((char *) 0);
}
