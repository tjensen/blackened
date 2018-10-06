/*
 * names.c: This here is used to maintain a list of all the people currently
 * on your channel.  Seems to work 
 *
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: names.c,v 1.4 2001/12/09 05:01:45 toast Exp $";
#endif

#include "irc.h"

#include "ircaux.h"
#include "names.h"
#include "window.h"
#include "screen.h"
#include "server.h"
#include "lastlog.h"
#include "list.h"
#include "output.h"
extern int in_on_who;

static	char	mode_str[] = "iklmnpst";

static	void	add_to_mode_list _((char *, char *));
static	void	check_mode_list_join _((char *));

/* NickList is declared in window.h */

/* ChannelList - moved to names.h */

struct modelist
{
	char	*chan;
	char	*mode;
	struct modelist *next;
}	*mode_list = (struct modelist *) 0;

/* channel_list: list of all the channels you are currently on */
static	ChannelList *channel_list = (ChannelList *) 0;

/* clear_channel: erases all entries in a nick list for the given channel */
static	void
clear_channel(chan)
	ChannelList *chan;
{
	NickList *tmp,
		*next;

	for (tmp = chan->nicks; tmp; tmp = next)
	{
		next = tmp->next;
		new_free(&(tmp->nick));
		new_free(&tmp);
	}
	chan->nicks = NULL;
}

extern	ChannelList *
lookup_channel(channel, server, unlink)
	char	*channel;
	int	server;
	int	unlink;
{
	ChannelList	*chan = channel_list,
			*last = NULL;

	if (server == -1)
		server = primary_server;
	while (chan)
	{
		if (chan->server == server && !my_stricmp(chan->channel, channel))
		{
			if (unlink)
			{
				if (last)
					last->next = chan->next;
				else
					channel_list = chan->next;
			}
			break;
		}
		last = chan;
		chan = chan->next;
	}
	return(chan);
}

/*
 * add_channel: adds the named channel to the channel list.  If the channel
 * is already in the list, nothing happens.   The added channel becomes the
 * current channel as well 
 */
void
add_channel(channel, server, rejoin)
	char	*channel;
	int	server,
		rejoin;
{
	ChannelList *new;

	if ((new = lookup_channel(channel, server, 0)) != NULL)
	{
		new->mode = 0;
		new->s_mode = (char *) 0;
		new->limit = 0;
		new->chop = 0;
		new->voice = 0;
		new->server = server;
		new->window = curr_scr_win;
		new->key = NULL;
		malloc_strcpy(&(new->channel), channel);
		clear_channel(new);
	}
	else
	{
		new = (ChannelList *) new_malloc(sizeof(ChannelList));
		new->channel = (char *) 0;
		new->mode = 0;
		new->s_mode = (char *) 0;
		new->limit = 0;
		new->chop = 0;
		new->voice = 0;
		new->server = server;
		new->window = curr_scr_win;
		new->key = NULL;
		malloc_strcpy(&(new->channel), channel);
		new->nicks = NULL;
		add_to_list(&channel_list, new);
	}
	if (!rejoin && !is_current_channel(channel, 0))
	{
		int	flag = 1;
		Window	*tmp;

		while ((tmp = traverse_all_windows(&flag)) != NULL)
		{
			if (!tmp->waiting_channel && !tmp->bind_channel)
				continue;
			if (tmp->server != from_server)
				continue;
			if (tmp->bind_channel && !my_stricmp(tmp->bind_channel, channel))
			{	
				set_channel_by_refnum(tmp->refnum, channel);
				new->window = tmp;
				update_all_windows();
				return;
			}
			if (tmp->waiting_channel && !my_stricmp(tmp->waiting_channel, channel))
			{	
				set_channel_by_refnum(tmp->refnum, channel);
				new->window = tmp;
				new_free(&tmp->waiting_channel);
				update_all_windows();
				return;
			}
		}
		set_channel_by_refnum(0, channel);
		new->window = curr_scr_win;
	}
	update_all_windows();
}

/*
 * add_to_channel: adds the given nickname to the given channel.  If the
 * nickname is already on the channel, nothing happens.  If the channel is
 * not on the channel list, nothing happens (although perhaps the channel
 * should be addded to the list?  but this should never happen) 
 */
void
add_to_channel(channel, nick, server, oper, voice, userhost)
	char	*channel;
	char	*nick;
	int	server;
	int	oper;
	int	voice;
	char	*userhost;
{
	NickList *new;
	ChannelList *chan;
	int	ischop = oper;
	char    temp[255];
	char    *user;
	char    *host;

	user = (char *) 0;
	host = (char *) 0;
	if (userhost)
	{
		strcpy(temp, userhost);
		if (host = strchr(temp, '@'))
		{
			user = temp;
			*host++ = '\0';
		}
	}

	if ((chan = lookup_channel(channel, server, 0)) != NULL)
	{
		if (*nick == '@')
		{
			nick++;
			if (my_stricmp(nick, get_server_nickname(server)) == 0)
			{
                                if (!list_lookup(&(chan->nicks), nick,
                                        !USE_WILDCARDS, !REMOVE_FROM_LIST))
				check_mode_list_join(channel);
				chan->chop = 1;
			}
			ischop = 1;
		}
		else if (*nick == '+')
		{
			nick++;
			if (my_stricmp(nick, get_server_nickname(server)) == 0)
			{
                                if (!list_lookup(&(chan->nicks), nick,
                                        !USE_WILDCARDS, !REMOVE_FROM_LIST))
				check_mode_list_join(channel);
				chan->voice = 1;
			}

			if (*nick == '@')
			{
				nick++;
				if (my_stricmp(nick, get_server_nickname(server)) == 0)
				{
				if (!list_lookup(&(chan->nicks), nick,
					!USE_WILDCARDS, !REMOVE_FROM_LIST))
					check_mode_list_join(channel);
					chan->chop = 1;
				}
				ischop = 1;
			}
		}

		if ((new = (NickList *) remove_from_list(&(chan->nicks), nick))
				!= NULL)
		{
			new_free(&(new->nick));
			if (new->user)
				new_free(&(new->user));
			if (new->host)
				new_free(&(new->host));
			new_free(&new);
		}
		new = (NickList *) new_malloc(sizeof(NickList));
		new->nick = (char *) 0;
		new->user = (char *) 0;
		new->host = (char *) 0;
		new->chanop = ischop;
		malloc_strcpy(&(new->nick), nick);
		if (user)
		{
			malloc_strcpy(&(new->user), user);
			malloc_strcpy(&(new->host), host);
		}
		add_to_list(&(chan->nicks), new);
	}
}


/*
 * recreate_mode: converts the bitmap representation of a channels mode into
 * a string 
 *
 * This malloces what it returns, but nothing that calls this function
 * is expecting to have to free anything.  Therefore, this function
 * should not malloc what it returns.  (hop)
 *
 * but this leads to horrible race conditions, so we add a bit to
 * the channel structure, and cache the string value of mode, and
 * the u_long value of the cached string, so that each channel only
 * has one copy of the string.  -mrg, june '94.
 */
static	char	*
recreate_mode(chan)
	ChannelList *chan;
{
	int	mode_pos = 0,
		str_pos = 0,
		mode;
	static	char	str[20];

	/* first check if cached string value is ok */

	if (chan->mode == chan->i_mode && chan->limit == chan->i_limit)
		return (chan->s_mode);

	chan->i_mode = chan->mode;
	chan->i_limit = chan->limit;
	buffer[0] = '\0';
	mode = chan->mode;
	while (mode)
	{
		if (mode % 2)
			buffer[str_pos++] = mode_str[mode_pos];
		mode /= 2;
		mode_pos++;
	}
	buffer[str_pos] = '\0';
	if (chan->key)
	{
		strcat(buffer, " ");
		strcat(buffer, chan->key);
	}
	if (chan->limit)
	{
		sprintf(str, " %d", chan->limit);
		strcat(buffer, str);
	}
	malloc_strcpy(&chan->s_mode, buffer);
	return (chan->s_mode);
}

/*
 * decifer_mode: This will figure out the mode string as returned by mode
 * commands and convert that mode string into a one byte bit map of modes 
 */
static	int
decifer_mode(mode_str, mode, chop, voice, nicks, key, channel)
	char	*mode_str;
	u_long	*mode;
	char	*chop;
	char	*voice;
	NickList **nicks;
	char	**key;
	char	*channel;
{
	char	*limit = 0;
	char	*person;
	char	*mask;
	int	add = 0;
	int	limit_set = 0;
	int	limit_reset = 0;
	char	*rest,
		*the_key;
	NickList *ThisNick;
	unsigned char	value = 0;

	if (!(mode_str = next_arg(mode_str, &rest)))
		return -1;
	for (; *mode_str; mode_str++)
	{
		switch (*mode_str)
		{
		case '+':
			add = 1;
			value = 0;
			break;
		case '-':
			add = 0;
			value = 0;
			break;
		case 'p':
			value = MODE_PRIVATE;
			break;
		case 'l':
			value = MODE_LIMIT;
			if (add)
			{
				limit_set = 1;
				if (!(limit = next_arg(rest, &rest)))
					limit = empty_string;
				else if (0 == strncmp(limit, "0", 1))
					limit_reset = 1, limit_set = 0, add = 0;
			}
			else
				limit_reset = 1;
			break;
		case 't':
			value = MODE_TOPIC;
			break;
		case 'i':
			value = MODE_INVITE;
			break;
		case 'n':
			value = MODE_MSGS;
			break;
		case 's':
			value = MODE_SECRET;
			break;
		case 'm':
			value = MODE_MODERATED;
			break;
		case 'o':
			if ((person = next_arg(rest, &rest)) &&
			    !my_stricmp(person, get_server_nickname(from_server)))
				*chop = add;
			ThisNick = (NickList *) list_lookup(nicks, person,
					!USE_WILDCARDS, !REMOVE_FROM_LIST);
			if (ThisNick)
				ThisNick->chanop=add;
			break;
		case 'k':
			value = MODE_KEY;
			the_key = next_arg(rest, &rest);
			if (add)
				malloc_strcpy(key, the_key);
			else
				new_free(key);
			break;	
		case 'v':
			if ((person = next_arg(rest, &rest)) &&
			    !my_stricmp(person, get_server_nickname(from_server)))
				*voice = add;
			break;
		case 'b':
			mask = next_arg(rest, &rest);
			if (!add && mask && channel)
				remove_ban(channel, mask);
			break;
		}
		if (add)
			*mode |= value;
		else
			*mode &= ~value;
	}
	if (limit_set)
		return (atoi(limit));
	else if (limit_reset)
		return(0);
	else
		return(-1);
}

/*
 * get_channel_mode: returns the current mode string for the given channel
 */
char	*
get_channel_mode(channel, server)
	char	*channel;
	int	server;
{
	ChannelList *tmp;

	if ((tmp = lookup_channel(channel, server, 0)) != NULL)
		return (recreate_mode(tmp));
	return (empty_string);
}

/*
 * set_channel_mode: This will set the mode of the given channel.  It will
 * zap any existing mode and replace it with the mode given 
 */
void
set_channel_mode(channel, mode)
	char	*channel,
		*mode;
{
	ChannelList *tmp;
	int	limit;

	if ((tmp = lookup_channel(channel, from_server, 0)) != NULL)
	{
		tmp->mode = 0;
		if ((limit = decifer_mode(mode, &(tmp->mode), &(tmp->chop), &(tmp->voice),
		    &(tmp->nicks), &(tmp->key), channel)) != -1)
		{
			tmp->limit = limit;
		}
	}
}

/*
 * update_channel_mode: This will modify the mode for the given channel
 * according the the new mode given.  
 */
void
update_channel_mode(channel, mode)
	char	*channel,
		*mode;
{
	ChannelList *tmp;
	int	limit;

	if ((tmp = lookup_channel(channel, from_server, 0)) != NULL)
	{
		if ((limit = decifer_mode(mode, &(tmp->mode), &(tmp->chop), &(tmp->voice),
				&(tmp->nicks), &(tmp->key), channel)) != -1)
			tmp->limit = limit;
	}
}

/*
 * is_channel_mode: returns the logical AND of the given mode with the
 * channels mode.  Useful for testing a channels mode 
 */
int
is_channel_mode(channel, mode, server_index)
	char	*channel;
	int	mode;
	int	server_index;
{
	ChannelList *tmp;

	if ((tmp = lookup_channel(channel, server_index, 0)) != NULL)
		return (tmp->mode & mode);
	return 0;
}

/*
 * remove_channel: removes the named channel from the channel_list.  If the
 * channel is not on the channel_list, nothing happens.  If the channel was
 * the current channel, this will select the top of the channel_list to be
 * the current_channel, or 0 if the list is empty. 
 */
void
remove_channel(channel, server)
	char	*channel;
	int	server;
{
	ChannelList *tmp;

	if (channel)
	{
		if ((tmp = lookup_channel(channel, server, 1)) != NULL)
		{
			clear_channel(tmp);
			new_free(&(tmp->channel));
			new_free(&(tmp->key));
			new_free(&tmp);
		}
		if (is_current_channel(channel, 1))
			switch_channels();
	}
	else
	{
		ChannelList *next;

		for (tmp = channel_list; tmp; tmp = next)
		{
			next = tmp->next;
			clear_channel(tmp);
			new_free(&(tmp->channel));
			new_free(&(tmp->key));
			new_free(&tmp);
		}
		channel_list = (ChannelList *) 0;
	}
	update_all_windows();
}

/*
 * remove_from_channel: removes the given nickname from the given channel. If
 * the nickname is not on the channel or the channel doesn't exist, nothing
 * happens. 
 */
void
remove_from_channel(channel, nick, server)
	char	*channel;
	char	*nick;
	int	server;
{
	ChannelList *chan;
	NickList *tmp;

	if (channel)
	{
		if ((chan = lookup_channel(channel, server, 0)) != NULL)
		{
			if ((tmp = (NickList *) list_lookup(&(chan->nicks),
			   nick, !USE_WILDCARDS, REMOVE_FROM_LIST)) != NULL)
			{
				new_free(&(tmp->nick));
				new_free(&tmp);
			}
		}
	}
	else
	{
		for (chan = channel_list; chan; chan = chan->next)
		{
			if (chan->server == server)
			{
				if ((tmp = (NickList *)
				    list_lookup(&(chan->nicks), nick,
				    !USE_WILDCARDS, REMOVE_FROM_LIST)) != NULL)
				{
					new_free(&(tmp->nick));
					new_free(&tmp);
				}
			}
		}
	}
}

/*
 * rename_nick: in response to a changed nickname, this looks up the given
 * nickname on all you channels and changes it the new_nick 
 */
void
rename_nick(old_nick, new_nick, server)
	char	*old_nick,
		*new_nick;
	int	server;
{
	ChannelList *chan;
	NickList *tmp;

	for (chan = channel_list; chan; chan = chan->next)
	{
		if ((chan->server == server) != 0)
		{
			if ((tmp = (NickList *) list_lookup(&chan->nicks, old_nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
			{
				new_free(&tmp->nick);
				malloc_strcpy(&tmp->nick, new_nick);
			}
		}
	}
}

/*
 * is_on_channel: returns true if the given nickname is in the given channel,
 * false otherwise.  Also returns false if the given channel is not on the
 * channel list. 
 */
int
is_on_channel(channel, nick)
	char	*channel;
	char	*nick;
{
	ChannelList *chan;

	chan = lookup_channel(channel, from_server, 0);
	if (chan)
	{
		if (list_lookup(&(chan->nicks), nick, !USE_WILDCARDS,
				!REMOVE_FROM_LIST))
			return (1);
	}
	return (0);
}

int
is_chanop(channel, nick)
	char	*channel;
	char	*nick;
{
	ChannelList *chan;
	NickList *Nick;

	if ((chan = lookup_channel(channel, from_server, 0)) &&
			(Nick = (NickList *) list_lookup(&(chan->nicks),
			nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)) &&
			Nick->chanop)
		return (1);
	return (0);
}

void
show_channel(chan)
	ChannelList *chan;
{
	NickList *tmp;
	int	buffer_len,
		len;
	char	*nicks = NULL;
	char	*s;

	s = recreate_mode(chan);
	*buffer = (char) 0;
	buffer_len = 0;
	for (tmp = chan->nicks; tmp; tmp = tmp->next)
	{
		len = strlen(tmp->nick);
		if (buffer_len + len >= (BIG_BUFFER_SIZE / 2))
		{
			malloc_strcpy(&nicks, buffer);
			say("\t%s +%s (%s): %s", chan->channel, s, get_server_name(chan->server), nicks);
			*buffer = (char) 0;
			buffer_len = 0;
		}
		strmcat(buffer, tmp->nick, BIG_BUFFER_SIZE);
		strmcat(buffer, " ", BIG_BUFFER_SIZE);
		buffer_len += len + 1;
	}
	malloc_strcpy(&nicks, buffer);
	say("\t%s +%s (%s): %s", chan->channel, s, get_server_name(chan->server), nicks);
	new_free(&nicks);
}

/* list_channels: displays your current channel and your channel list */
void
list_channels()
{
	ChannelList *tmp;

	if (channel_list)
	{
		if (get_channel_by_refnum(0))
			say("Current channel %s", get_channel_by_refnum(0));
		else
			say("No current channel for this window");
		say("You are on the following channels:");
		for (tmp = channel_list; tmp; tmp = tmp->next)
		{
			if (tmp->server == from_server)
				show_channel(tmp);
		}
		if (connected_to_server != 1)
		{
			say("Other servers:");
			for (tmp = channel_list; tmp; tmp = tmp->next)
			{
				if (tmp->server != from_server)
					show_channel(tmp);
			}
		}
	}
	else
		say("You are not on any channels");
}

void
switch_channels()
{
	ChannelList *tmp;

	if (channel_list)
	{
		if (get_channel_by_refnum(0))
		{
			if ((tmp = lookup_channel(get_channel_by_refnum(0),
				from_server, 0)) != NULL)
			{
				for (tmp = tmp->next; tmp; tmp = tmp->next)
				{
					if ((tmp->server == from_server) &&
					    !is_current_channel(tmp->channel,
					    0))
					{
						set_channel_by_refnum(0,
							tmp->channel);
						update_all_windows();
						return;
					}
				}
			}
		}
		for (tmp = channel_list; tmp; tmp = tmp->next)
		{
			if ((tmp->server == from_server) &&
			    (!is_current_channel(tmp->channel, 0)))
			{
				set_channel_by_refnum(0, tmp->channel);
				update_all_windows();
				return;
			}
		}
	}
}

/* real_channel: returns your "real" channel (your non-multiple channel) */
char	*
real_channel()
{
	ChannelList *tmp;

	if (channel_list)
	{
		for (tmp = channel_list; tmp; tmp = tmp->next)
		{
			if ((tmp->server == from_server) &&
					(*(tmp->channel) != '#'))
				return (tmp->channel);
		}
	}
	return ((char *) 0);
}

int
get_channel_server(channel)
	char	*channel;
{
	ChannelList *chan;

	if ((chan = (ChannelList *) list_lookup(&channel_list, channel,
			!USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
		return(chan->server);
	return(-1);
}

void
change_server_channels(old, new)
	int	old,
		new;
{
	ChannelList *tmp;

	for(tmp = channel_list; tmp ;tmp = tmp->next)
	{
		if (tmp->server == old)
			tmp->server = new;
	}
}

void
clear_channel_list(server)
	int	server;
{
	ChannelList *tmp;

	tmp = channel_list;
	while(tmp)
	{
		if (tmp->server == server)
		{
			remove_channel(tmp->channel, server);
			tmp = channel_list;
		}
		else
			tmp = tmp->next;
	}
}

/*
 * reconnect_all_channels: used after you get disconnected from a server, 
 * clear each channel nickname list and re-JOINs each channel in the 
 * channel_list ..  
 */
void
reconnect_all_channels()
{
	ChannelList *tmp;
	int	version;
	char	*s;

	for (tmp = channel_list; tmp; tmp = tmp->next)
	{
		if (tmp->server == from_server)
		{
			char	*t;

			s = recreate_mode(tmp);
			if ((t = tmp->window->current_channel) && *t
			&& !is_bound(t, tmp->window->server))
				malloc_strcpy(&tmp->window->waiting_channel, t);
			if ((version = get_server_version(from_server)) >= Server2_8)
			{
				send_to_server("JOIN %s%s%s", tmp->channel, tmp->key ? " " : "", tmp->key ? tmp->key : "");
				if ((char *) 0 != s)
					add_to_mode_list(tmp->channel, s);
			}
			else
				send_to_server("JOIN %s%s%s", tmp->channel, version ? " " : "", version ? s : "");
			clear_channel(tmp);
		}
	}
	clear_channel_list(from_server);
	message_from((char *) 0, LOG_CRAP);
}

extern	char	*
channel_key(channel)
	char	*channel;
{
	ChannelList *tmp;

	for (tmp = channel_list; tmp && strcmp(tmp->channel, channel); tmp = tmp->next)
		;
#ifdef COMSTUD_ORIG
	return tmp ? tmp->key : empty_string;
#else
        if (tmp != 0) {
           if (tmp->key != 0) {
              return(tmp->key);
           }
        }
        return(empty_string);
#endif
}

char	*
what_channel(nick)
	char	*nick;
{
	ChannelList *tmp;

	if (curr_scr_win->current_channel &&
	    is_on_channel(curr_scr_win->current_channel, nick))
		return curr_scr_win->current_channel;
	for (tmp = channel_list; tmp; tmp = tmp->next)
	{
		if ((tmp->server == from_server) && (list_lookup(&(tmp->nicks),
		    nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)))
			return tmp->channel;
	}
	return NULL;
}

char	*
walk_channels(nick, init)
	int	init;
	char	*nick;
{
	static	ChannelList *tmp = (ChannelList *) 0;

	if (init)
		tmp = channel_list;
	else if (tmp)
		tmp = tmp->next;
	for (;tmp ; tmp = tmp->next)
		if ((tmp->server == from_server) && (list_lookup(&(tmp->nicks),
				nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)))
			return (tmp->channel);
	return (char *) 0;
}

int
get_channel_oper(channel, server)
	char	*channel;
	int	server;
{
	ChannelList *chan;

	if ((chan = lookup_channel(channel, server, 0)) != NULL)
		return chan->chop;
	else
		return 1;
}

int
get_channel_voice(channel, server)
	char	*channel;
	int	server;
{
	ChannelList *chan;

	if ((chan = lookup_channel(channel, server, 0)) != NULL)
		return chan->voice;
	else
		return 1;
}

extern	void
set_channel_window(window, channel)
	Window	*window;
	char	*channel;
{
	ChannelList	*tmp;

	if (!channel)
		return;
	for (tmp = channel_list; tmp; tmp = tmp->next)
		if (!my_stricmp(channel, tmp->channel) &&
				from_server == tmp->server)
		{
			tmp->window = window;
			return;
		}
}

extern	char	*
create_channel_list(window)
	Window	*window;
{
	ChannelList	*tmp;
	char	*value = (char *) 0;

	*buffer = '\0';
	for (tmp = channel_list; tmp; tmp = tmp->next)
		if (tmp->server == window->server)
		{
			strcat(buffer, tmp->channel);
			strcat(buffer, " ");
		}
	malloc_strcpy(&value, buffer);

	return value;
}

extern	void
channel_server_delete(i)
	int	i;
{
	ChannelList	*tmp;

	for (tmp = channel_list ; tmp; tmp = tmp->next)
		if (tmp->server >= i)
			tmp->server--;
}

extern	void
set_waiting_channel(i)
	int	i;
{
	Window	*tmp;
	int	flag = 1;

	while ((Window *) 0 != (tmp = traverse_all_windows(&flag)))
		if (tmp->server == i && tmp->current_channel)
		{
			if (!tmp->bind_channel)
				tmp->waiting_channel = tmp->current_channel;
			tmp->current_channel = (char *) 0;
		}
}

static	void
add_to_mode_list(channel, mode)
	char	*channel;
	char	*mode;
{
	struct modelist	*mptr;

	if (!channel || !*channel || !mode || !*mode)
		return;
	mptr = (struct modelist *) new_malloc(sizeof(struct modelist));
	if (mptr) {
		mptr->chan = (char *) 0;
		mptr->mode = (char *) 0;
		mptr->next = mode_list;
		mode_list = mptr;
		malloc_strcpy(&mptr->chan, channel);
		malloc_strcpy(&mptr->mode, mode);
	} else
		say("Out of memory on add_to_mode_list: %s %s", channel, mode);
}

static	void
check_mode_list_join(channel)
	char	*channel;
{
	struct modelist *mptr = mode_list;

	for (;(struct modelist *) 0 != mptr; mptr = mptr->next)
	{
		if (0 == my_stricmp(mptr->chan, channel))
		{
			send_to_server("MODE %s %s", mptr->chan, mptr->mode);
			return;
		}
	}
	remove_from_mode_list(channel);
}

extern	void
remove_from_mode_list(channel)
	char	*channel;
{
	struct modelist *curr, *prev, *next;

	for (curr = mode_list; curr; curr = next)
	{
		next = curr->next;
		if (curr->chan && (0 == my_stricmp(curr->chan, channel)))
		{
			if (curr == mode_list)
				mode_list = curr->next;
			else
				prev->next = curr->next;
			prev = curr;
			new_free(&curr->chan);
			new_free(&curr->mode);
			new_free(&curr);
		}
		else
			prev = curr;
	}
}
