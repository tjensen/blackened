/*
 * autoop.c: Routines dealing with the Auto-Op system.
 *
 * Written By Timothy Jensen
 *
 * Copyright (c) 1999 
 *
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: autoop.c,v 1.6 2001/11/11 23:55:09 toast Exp $";
#endif

#include "irc.h"

#include <time.h>
#include "autoop.h"
#include "ircaux.h"
#include "server.h"
#include "output.h"
#include "whois.h"
#include "comstud.h"
#include "names.h"
#include "vars.h"

typedef struct ao_UserItemStru {
	char *userhost;
	struct ao_UserItemStru *next;
} ao_UserItem;

typedef struct ao_ChanItemStru {
	char *name;
	ao_UserItem *ao_userlist;
	struct ao_ChanItemStru *next;
} ao_ChanItem;

ao_ChanItem *ao_chanlist = 0;

static void
AddTimer(nick, channel, when)
	char	*nick,
		*channel;
	time_t	when;
{
	AutoOp_Timer	*curr;

	curr = (AutoOp_Timer *) new_malloc(sizeof(AutoOp_Timer));
	curr->nick = curr->channel = (char *) 0;
	malloc_strcpy(&curr->nick, nick);
	malloc_strcpy(&curr->channel, channel);
	curr->when = time(NULL) + when;
	curr->next = server_list[from_server].OpTimers;
	server_list[from_server].OpTimers = curr;
}


static int
in_autoop_list(channel, userhost)
	char	*channel,
		*userhost;
{
	ao_ChanItem	*chanitem = ao_chanlist;
	ao_UserItem	*useritem;

	if (!channel || !userhost)
	{
		return 0;
	}

	while (chanitem)
	{
		if (!my_stricmp(channel, chanitem->name) ||
			!strcmp("*", chanitem->name))
		{
			useritem = chanitem->ao_userlist;
			while (useritem)
			{
				if (wild_match(useritem->userhost, userhost))
				{
					return -1;
				}
				useritem = useritem->next;
			}
		}
		chanitem = chanitem->next;
	}
	return 0;
}


static int
in_autoop_list_exact(channel, userhost)
	char	*channel,
		*userhost;
{
	ao_ChanItem	*chanitem = ao_chanlist;
	ao_UserItem	*useritem;

	if (!channel || !userhost)
	{
		return 0;
	}

	while (chanitem)
	{
		if (!my_stricmp(channel, chanitem->name) ||
			!strcmp("*", chanitem->name))
		{
			useritem = chanitem->ao_userlist;
			while (useritem)
			{
				if (!my_stricmp(useritem->userhost, userhost))
				{
					return -1;
				}
				useritem = useritem->next;
			}
		}
		chanitem = chanitem->next;
	}
	return 0;
}


void
handle_autoop(nick, channel, userhost)
	char	*nick,
		*channel,
		*userhost;
{
	int		delay;

	if (!nick || !channel || !userhost)
		return;

	if (in_autoop_list(channel, userhost))
	{
		if ((delay = get_int_var(AUTOOP_DELAY_VAR)) > 0)
		{
			AddTimer(nick, channel,
				delay + (random() % (delay + 1)));
		}
		else if (get_channel_oper(channel, from_server))
		{
			send_to_server("MODE %s +o %s", channel, nick);
		}
	}
}


void
autoop_add(channel, userhost)
	char	*channel,
		*userhost;
{
	ao_ChanItem *chanitem = ao_chanlist, *lastchan = NULL;
	ao_UserItem *useritem = NULL, *tempuser;

	if (in_autoop_list_exact(channel, userhost))
	{
		say("%s is already on %s auto-op list.", userhost, channel);
		return;
	}

	while (chanitem)
	{
		if (!my_stricmp(channel, chanitem->name))
		{
			useritem = (ao_UserItem *)new_malloc(sizeof(ao_UserItem));
			useritem->userhost = NULL;
			malloc_strcpy(&useritem->userhost, userhost);
			useritem->next = NULL;
			if (!chanitem->ao_userlist)
			{
				chanitem->ao_userlist = useritem;
			}
			else
			{
				tempuser = chanitem->ao_userlist;
				while (tempuser->next)
				{
					tempuser = tempuser->next;
				}
				tempuser->next = useritem;
			}
			say("%s added to channel %s auto-op list.",
				useritem->userhost, channel);
			return;
		}
		lastchan = chanitem;
		chanitem = chanitem->next;
	}
	chanitem = (ao_ChanItem *) new_malloc(sizeof(ao_ChanItem));
	useritem = (ao_UserItem *) new_malloc(sizeof(ao_UserItem));
	chanitem->name = NULL;
	malloc_strcpy(&chanitem->name, channel);
	chanitem->next = NULL;
	chanitem->ao_userlist = useritem;
	useritem->next = NULL;
	useritem->userhost = NULL;
	malloc_strcpy(&useritem->userhost, userhost);
	if (!ao_chanlist)
	{
		ao_chanlist = chanitem;
	}
	else
	{
		lastchan->next = chanitem;
	}
	say("%s added to channel %s auto-op list.", useritem->userhost,
		channel);
}


void
autoop_remove(channel, userhost)
	char	*channel,
		*userhost;
{
	ao_ChanItem *chanitem = ao_chanlist;
	ao_UserItem *tempuser, *tempuser2;

	while (chanitem)
	{
		if (!my_stricmp(channel, chanitem->name))
		{
			if (chanitem->ao_userlist &&
				chanitem->ao_userlist->userhost &&
				wild_match(chanitem->ao_userlist->userhost,
				userhost))
			{
				say("%s removed from %s auto-op list.",
					chanitem->ao_userlist->userhost,
					channel);
				new_free(&(chanitem->ao_userlist->userhost));
	 			tempuser = chanitem->ao_userlist;
				chanitem->ao_userlist = chanitem->ao_userlist->next;
				new_free(&tempuser);
				return;
			}
			tempuser2 = chanitem->ao_userlist;
			while (tempuser2)
			{
				if (tempuser2->next &&
					tempuser2->next->userhost &&
					wild_match(tempuser2->next->userhost,
					userhost))
				{
					say("%s removed from %s auto-op list.",
				  		tempuser2->next->userhost,
						channel);
					new_free(&(tempuser2->next->userhost));
					tempuser = tempuser2->next;
					tempuser2->next = tempuser2->next->next;
					new_free(&tempuser);
					return;
				}
				tempuser2 = tempuser2->next;
			}
		}
		chanitem = chanitem->next;
	}
	say("%s not found in %s auto-op list.", userhost, channel);
}


void
userhost_add_autoop(stuff, nick, args)
	WhoisStuff	*stuff;
	char		*nick,
			*args;
{
        char	*userhost = NULL,
		*usercluster = NULL,
		*channel;

        if (!stuff || !stuff->nick || !nick || my_stricmp(stuff->nick, nick))
	{
		return;
	}
	if (stuff->not_on)
	{
		say("%s is not on IRC!", nick);
		return;
	}

	userhost = new_malloc(strlen(stuff->user) + strlen(stuff->host) + 2);
	sprintf(userhost, "%s@%s", stuff->user, stuff->host);

	usercluster = new_malloc(strlen(cluster(userhost)) + 2);
	sprintf(usercluster, "*%s", cluster(userhost));

	channel = args;

	autoop_add(channel, usercluster);
	new_free(&userhost);
	new_free(&usercluster);
}


void
userhost_remove_autoop(stuff, nick, args)
	WhoisStuff	*stuff;
	char		*nick,
			*args;
{
        char	*userhost = NULL,
        	*channel;

        if (!stuff || !stuff->nick || !nick || my_stricmp(stuff->nick, nick))
	{
		return;
	}
	if (stuff->not_on)
	{
		say("%s is not on IRC!", nick);
		return;
	}
	userhost = new_malloc(strlen(stuff->user) + strlen(stuff->host) + 2);
	sprintf(userhost, "%s@%s", stuff->user, stuff->host);
	channel = args;

	autoop_remove(channel, userhost);
	new_free(&userhost);
}


void
autoop(command, args)
	char	*command,
		*args;
{
	char *channel, *userhost;
	ao_ChanItem *chanitem, *tempchan;
	ao_UserItem *useritem, *tempuser;
	int len;
	int remove = 0;

	chanitem = ao_chanlist;

	channel = my_next_arg(&args);
	if (channel && (*channel == '-'))
	{
		channel++;
		len = strlen(channel);
		if (!my_strnicmp(channel, "REMOVE", (len > 1) ? len : 1))
		{
			remove = 1;
		}
		else
		{
			say("Unknown or missing flag");
			return;
		}
		channel = my_next_arg(&args);
	}
	userhost = my_next_arg(&args);
	if (!channel)			/*  /autoop (-remove)  */
	{
		if (!remove)
		  while (chanitem) {
			say("Auto-Op list for channel %s:", chanitem->name);
			useritem = chanitem->ao_userlist;
			while (useritem) {
				say("%s %s", chanitem->name, 
					useritem->userhost);
				useritem = useritem->next;
			}
			chanitem = chanitem->next;
		  }
		else {
		  while (ao_chanlist) {
		    while (ao_chanlist->ao_userlist) {
		      new_free(&(ao_chanlist->ao_userlist->userhost));
		      tempuser = ao_chanlist->ao_userlist;
		      ao_chanlist->ao_userlist = ao_chanlist->ao_userlist->next;
		      new_free(&tempuser);
		    }
		    new_free(&(ao_chanlist->name));
		    tempchan = ao_chanlist;
		    ao_chanlist = ao_chanlist->next;
		    new_free(&tempchan);
		  }
		  say("Auto-Op list cleared.");
		}
	} else if (!userhost) {		/*  /autoop (-remove) #channel  */
		while (chanitem) {
			if (!my_stricmp(channel,chanitem->name)) {
				if (!remove) {
				  say("Auto-Op list for channel %s:",
					channel);
				  useritem = chanitem->ao_userlist;
				  while (useritem) {
					say("%s %s", chanitem->name, 
						useritem->userhost);
					useritem = useritem->next;
				  }
				} else {
				  while (chanitem->ao_userlist) {
		      		    new_free(&(chanitem->ao_userlist->userhost));
		      		    tempuser = chanitem->ao_userlist;
		      		    chanitem->ao_userlist = chanitem->ao_userlist->next;
		      		    new_free(&tempuser);
				  }
				  say("Auto-Op list for channel %s cleared.", channel);
				}
			}
			chanitem = chanitem->next;
		}
	}
	else			/*  /autoop (-remove) #channel user@host  */
	{
	    if (!index(userhost, '@'))
	    {
		/* User specified nick rather than user@host */
		if (!remove)
			typed_add_to_whois_queue(WHOIS_USERHOST, userhost,
				userhost_add_autoop, "%s", channel);
		else
			typed_add_to_whois_queue(WHOIS_USERHOST, userhost,
				userhost_remove_autoop, "%s", channel);
	    }
	    else
	    {
		if (!remove)
		{
			autoop_add(channel, userhost);
		}
		else
		{
			autoop_remove(channel, userhost);
		}
	    }
	}
}

void
save_autoops(fp)
	FILE	*fp;
{
	ao_ChanItem *chanitem;
	ao_UserItem *useritem;

	chanitem = ao_chanlist;
	while (chanitem) {
		useritem = chanitem->ao_userlist;
		while (useritem) {
			fprintf(fp, "AUTOOP %s %s\n", chanitem->name,
				useritem->userhost);
			useritem = useritem->next;
		}
		chanitem = chanitem->next;
	}
}

void
OpTimer()
{
	AutoOp_Timer	*curr, *last, *next;
	time_t	now;
	int	i, save_server;

	now = time(NULL);

	for (i = 0; i < number_of_servers; i++) {
		curr = server_list[i].OpTimers;
		last = (AutoOp_Timer *) 0;
		while (curr) {
			next = curr->next;
			if (now > curr->when) {
				if (get_channel_oper(curr->channel, i) &&
						!is_chanop(curr->channel, curr->nick)) {
					save_server = from_server;
					from_server = i;
					send_to_server("MODE %s +o %s", curr->channel,
						curr->nick);
					from_server = save_server;
				}
				if (last)
					last->next = curr->next;
				else
					server_list[i].OpTimers = curr->next;
				new_free(&curr->nick);
				new_free(&curr->channel);
				new_free(&curr);
			} else
				last = curr;
			curr = next;
		}
	}
}

void
remove_autoop_chan(nick, channel)
	char	*nick,
		*channel;
{
	AutoOp_Timer	*curr, *last, *next;

	last = (AutoOp_Timer *) 0;
	curr = server_list[from_server].OpTimers;

	while (curr) {
		next = curr->next;
		if (!my_stricmp(curr->nick, nick) &&
				!my_stricmp(curr->channel, channel)) {
			if (last)
				last->next = curr->next;
			else
				server_list[from_server].OpTimers = curr->next;
			new_free(&curr->nick);
			new_free(&curr->channel);
			new_free(&curr);
		} else
			last = curr;
		curr = next;
	}
}

void
remove_autoop_all(nick)
	char	*nick;
{
	AutoOp_Timer	*curr, *last, *next;

	last = (AutoOp_Timer *) 0;
	curr = server_list[from_server].OpTimers;

	while (curr) {
		next = curr->next;
		if (!my_stricmp(curr->nick, nick)) {
			if (last)
				last->next = curr->next;
			else
				server_list[from_server].OpTimers = curr->next;
			new_free(&curr->nick);
			new_free(&curr->channel);
			new_free(&curr);
		} else
			last = curr;
		curr = next;
	}
}

void
rename_autoop(nick, newnick)
	char	*nick,
		*newnick;
{
	AutoOp_Timer	*curr = server_list[from_server].OpTimers;

	while (curr) {
		if (!my_stricmp(curr->nick, nick))
			malloc_strcpy(&curr->nick, newnick);
		curr = curr->next;
	}
}
