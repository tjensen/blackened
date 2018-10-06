/*
 * rejoin.c: Code for automatically re-joining channels
 *
 * Written By Timothy Jensen
 *
 * Copyright (c) 1999-2001
 *
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: rejoin.c,v 1.8 2001/11/30 23:05:47 toast Exp $";
#endif

#include "irc.h"

#include "rejoin.h"
#include "ircaux.h"
#include "server.h"
#include "vars.h"
#include "output.h"


/*
 * rejoin_add: Adds a channel to the rejoin list.  Duplicate list elements
 *	are not allowed, however adding a duplicate channel that is in the
 *	REJOIN_DELETE state will raise the existing element to the
 *	REJOIN_WAIT state.
 *
 *	The channel name may be missing the channel prefix character.
 */
static	void
rejoin_add(channel, key, server_num)
	char	*channel,
		*key;
	int	server_num;
{
	RejoinChan	*new,
			*tmp;
	int		key_matches;

	new = malloc(sizeof(RejoinChan));
	if (new != NULL)
	{
		new->next = NULL;
		new->status = REJOIN_NORMAL;
		new->last_attempt = time(NULL);
		new->channel = NULL;
		new->key = NULL;
		if (is_channel(channel))
		{
			malloc_strcpy(&(new->channel), channel);
		}
		else
		{
			new->channel = new_malloc(strlen(channel) + 2);
			if (new->channel)
			{
				sprintf(new->channel, "#%s", channel);
			}
		}
		if (key)
		{
			malloc_strcpy(&(new->key), key);
		}
		if (new->channel)
		{
			tmp = server_list[server_num].rejoins;
			if (tmp)
			{
				key_matches = (tmp->key && key) ?
					!my_stricmp(tmp->key, key) :
					!(tmp->key || key);
				if (!my_stricmp(new->channel,
					tmp->channel))
				{
					if (tmp->status ==
						REJOIN_DELETE)
					{
						if (key)
						{
							say("%s (key: %s) has been added to the channel rejoin list.",
							new->channel, key);
						}
						else
						{
						say("%s has been added to the channel rejoin list.",
							new->channel);
						}
						tmp->status =
							REJOIN_WAIT;
						malloc_strcpy(&(tmp->key),
							key);
					}
					else
					{
						if (!key_matches)
						{
							if (key)
							{
								say("%s rejoin key changed to: %s",
									new->channel,
									key);
								malloc_strcpy(&(tmp->key),
									key);
							}
							else
							{
								say("%s rejoin key cleared",
									new->channel);
								new_free(&tmp->key);
							}
						}
						else
						{
							say("%s is already on the channel rejoin list.",
								new->channel);
						}
					}
					new_free(&new->channel);
					if (new->key)
					{
						new_free(&new->key);
					}
					new_free(&new);
					return;
				}
				while (tmp->next);
				{
					if (!my_stricmp(new->channel,
						tmp->channel))
					{
						if (tmp->status ==
							REJOIN_DELETE)
						{
							if (key)
							{
								say("%s (key: %s) has been added to the channel rejoin list.",
									new->channel,
									key);
							}
							else
							{
								say("%s has been added to the channel rejoin list.",
									new->channel);
							}
							tmp->status =
								REJOIN_WAIT;
							malloc_strcpy(&(tmp->key),
								key);
						}
						else
						{
							if (!key_matches)
							{
								if (key)
								{
									say("%s rejoin key changed to: %s",
										tmp->channel,
										key);
									malloc_strcpy(&(tmp->key),
										key);
								}
								else
								{
									say("%s rejoin key cleared",
										tmp->channel);
									new_free(&tmp->key);
								}
							}
							else
							{
								say("%s is already on the channel rejoin list.",
									new->channel);
							}
						}
						new_free(&new->channel);
						if (new->key)
						{
							new_free(&new->key);
						}
						new_free(&new);
						return;
					}
					tmp = tmp->next;
				}
				tmp->next = new;
			}
			else
			{
				server_list[server_num].rejoins = new;
			}
			if (new->key)
			{
				say("%s (key: %s) has been added to the channel rejoin list.",
					new->channel, new->key);
			}
			else
			{
				say("%s has been added to the channel rejoin list.",
					new->channel);
			}
		}
		else
		{
			new_free(&new);
			say("Not enough memory to rejoin!");
		}
	}
	else
	{
		say("Not enough memory to rejoin!");
	}
}


/*
 * rejoin_remove: Removes a channel from the rejoin list.  Channel name
 * 	may be missing a channel prefix character.  Rejoin attempts that
 *	are waiting for a response from the server (REJOIN_WAIT) are put
 *	into the REJOIN_DELETE state for later removal.
 */
static	void
rejoin_remove(channel, server_num)
	char	*channel;
	int	server_num;
{
	RejoinChan	*tmp = server_list[server_num].rejoins,
			*last = NULL;
	int		cmp;

	while (tmp)
	{
		if (is_channel(channel))
		{
			cmp = my_stricmp(tmp->channel, channel);
		}
		else
		{
			cmp = my_stricmp(tmp->channel + 1, channel);
		}
		if (!cmp)
		{
			if (tmp->status == REJOIN_DELETE)
			{
				say("%s is not on the channel rejoin list!",
					channel);
				return;
			}
			say("%s is no longer on the channel rejoin list.",
				tmp->channel);
			if (tmp->status == REJOIN_NORMAL)
			{
				if (last)
				{
					last->next = tmp->next;
				}
				else
				{
					server_list[server_num].rejoins =
						tmp->next;
				}
				new_free(&tmp->channel);
				if (tmp->key)
				{
					new_free(&tmp->key);
				}
				new_free(&tmp);
			}
			else if (tmp->status == REJOIN_WAIT)
			{
				tmp->status = REJOIN_DELETE;
			}
			return;
		}
		last = tmp;
		tmp = tmp->next;
	}
	say("%s is not on the channel rejoin list!", channel);
}


/*
 * rejoin_get: Retrieves an element from the rejoin list.  If a matching
 *	channel name cannot be found, return NULL.
 */
static	RejoinChan	*
rejoin_get(channel, server_num)
	char	*channel;
	int	server_num;
{
	RejoinChan	*tmp = server_list[server_num].rejoins;

	while (tmp)
	{
		if (!my_stricmp(channel, tmp->channel))
		{
			return tmp;
		}
	}
	return NULL;
}


/*
 * rejoin_list: Lists all of the channels the client is trying to rejoin,
 *	along with the number of seconds until the next rejoin attempt.
 *
 *	If the client has made an attempt to rejoin a channel but is
 *	still waiting for a response from the server, the channel is in
 *	the REJOIN_WAIT state and is displayed with the text, "(wait)".
 *
 *	Further, when a channel is removed from the rejoin list while
 *	it is in the REJOIN_WAIT state, it is lowered to the
 *	REJOIN_DELETE state (shown as "(delete)"), indicating that when
 *	the client receives a response from the server, it should
 *	immediately remove the element from the list.
 *
 *	Make sense? :-)
 */
static	void
rejoin_list(server_num)
	int	server_num;
{
	RejoinChan	*tmp = server_list[server_num].rejoins;
	time_t		now = time(NULL);
	int		interval = get_int_var(REJOIN_INTERVAL_VAR);

	if (tmp)
	{
		say("Seconds  Channel (Key)");
		while (tmp)
		{
			if (tmp->status == REJOIN_WAIT)
			{
				if (tmp->key)
				{
					say("(wait)   %s (%s)", tmp->channel,
						tmp->key);
				}
				else
				{
					say("(wait)   %s", tmp->channel);
				}
			}
			else if (tmp->status == REJOIN_DELETE)
			{
				if (tmp->key)
				{
					say("(delete) %s (%s)", tmp->channel,
						tmp->key);
				}
				else
				{
					say("(delete) %s", tmp->channel);
				}
			}
			else
			{
				if (tmp->key)
				{
					say("%-8d %s (%s)",
						interval - (now - tmp->last_attempt),
						tmp->channel, tmp->key);
				}
				else
				{
					say("%-8d %s",
						interval - (now - tmp->last_attempt),
						tmp->channel);
				}
			}
			tmp = tmp->next;
		}
	}
	else
	{
		say("There are no channels on the rejoin list!");
	}
}


/*
 * rejoin: The REJOIN command.  This is basically just a springboard
 *	for branching to one of the functions listed above.
 */
void
rejoin(command, args)
	char	*command,
		*args;
{
	char	*channel,
		*key;
	int	add;

	if ((channel = next_arg(args, &args)) != NULL)
	{
		do {
			add = 1;
			if (*channel == '-')
			{
				add = 0;
				channel++;
			}
			if (*channel != 0)
			{
				if (NULL != (key = index(channel, ',')))
				{
					*(key++) = 0;
					if (0 == *key)
					{
						key = NULL;
					}
				}
				if (add)
				{
					rejoin_add(channel, key, from_server);
				}
				else
				{
					rejoin_remove(channel, from_server);
				}
			}
			else
			{
				say("Channel name required!");
			}
		} while ((channel = next_arg(args, &args)) != NULL);
	}
	else
	{
		rejoin_list(from_server);
	}
}


/*
 * rejoin_kill: Silently removes a channel from the rejoin list.  Channel
 *	name must be exact.  Channel rejoin status is NOT checked.  The
 *	channel is absolutely, positively removed from the list.  No
 *	error messages are displayed if the channel is not present in the
 *	list.
 *
 *	This function is not called by the REJOIN command.  It is intended
 *	for use when a positive rejoin response is received, or when a
 *	negative rejoin response is received and the channel has
 *	REJOIN_DELETE status.
 */
void
rejoin_kill(channel, server_num)
	char	*channel;
	int	server_num;
{
	RejoinChan	*tmp = server_list[server_num].rejoins,
			*last = NULL;

	while (tmp)
	{
		if (!my_stricmp(tmp->channel, channel))
		{
			if (last)
			{
				last->next = tmp->next;
			}
			else
			{
				server_list[server_num].rejoins =
					tmp->next;
			}
			new_free(&tmp->channel);
			if (tmp->key)
			{
				new_free(&tmp->key);
			}
			new_free(&tmp);
			return;
		}
		last = tmp;
		tmp = tmp->next;
	}
}


/*
 * is_on_rejoin_list: Returns non-zero if the given channel is on the rejoin
 *	list.
 */
int
is_on_rejoin_list(channel, server_num)
	char	*channel;
	int	server_num;
{
	return (NULL != rejoin_get(channel, server_num));
}


/*
 * rejoin_failed: This function is called by numbers.c when a server response
 *	is received, indicating a join attempt failed.
 */
int
rejoin_failed(channel, server_num, variable)
	char	*channel;
	int	server_num,
		variable;
{
	RejoinChan	*tmp;

	tmp = rejoin_get(channel, server_num);
	if (tmp)
	{
		if (tmp->status == REJOIN_NORMAL)
		{
			return 0;
		}
		else if (tmp->status == REJOIN_DELETE)
		{
			rejoin_kill(channel, server_num);
		}
		else if (tmp->status == REJOIN_WAIT)
		{
			tmp->last_attempt = time(NULL);
			tmp->status = REJOIN_NORMAL;
		}
		return 1;
	}
	else
	{
		if (get_int_var(variable))
		{
			rejoin_add(channel, NULL, server_num);
		}
	}
	return 0;
}


/*
 * RejoinTimer: Scans through all rejoin lists and attempts to rejoin
 *	channels whose rejoin intervals have elapsed.
 */
void
RejoinTimer(void)
{
	RejoinChan	*curr,
			*last,
			*next;
	time_t	now = time(NULL);
	int 	i,
		save_server,
		interval = get_int_var(REJOIN_INTERVAL_VAR);

	for (i = 0; i < number_of_servers; i++) {
		curr = server_list[i].rejoins;
		last = (RejoinChan *) 0;
		while (curr) {
			next = curr->next;
			if ((curr->status == REJOIN_NORMAL) &&
				((now - curr->last_attempt) >= interval))
			{
				save_server = from_server;
				from_server = i;
				if (curr->key)
				{
					send_to_server("JOIN %s %s",
						curr->channel, curr->key);
				}
				else
				{
					send_to_server("JOIN %s",
						curr->channel);
				}
				curr->status = REJOIN_WAIT;
				from_server = save_server;
			}
			curr = next;
		}
	}
}

