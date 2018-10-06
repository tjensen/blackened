/*
 * notify.c: a few handy routines to notify you when people enter and leave irc 
 *
 * Written By Michael Sandrof
 * Copyright(c) 1990 
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * Revamped by lynX - Dec '91
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: notify.c,v 1.1.1.1 1999/07/16 21:21:43 toast Exp $";
#endif

#include "irc.h"

#include "list.h"
#include "notify.h"
#include "ircaux.h"
#include "whois.h"
#include "hook.h"
#include "server.h"
#include "output.h"
#include "vars.h"
#include "toast.h"
#include "format.h"

/* NotifyList: the structure for the notify stuff */
typedef	struct	notify_stru
{
	struct	notify_stru	*next;	/* pointer to next notify person */
	char	*nick;			/* nickname of person to notify about */
	int	flag;			/* 1=person on irc, 0=person not on irc */
}	NotifyList;

static	NotifyList	*notify_list = (NotifyList *) 0;

extern void	ison_notify();

/* Rewritten, -lynx */
void
show_notify_list(all)
	int	all;
{
	NotifyList	*tmp;
	char	*list = (char *) 0;

	malloc_strcpy(&list, empty_string);
	for (tmp = notify_list; tmp; tmp = tmp->next)
	{
		if (tmp->flag)
		{
			malloc_strcat(&list, " ");
			malloc_strcat(&list, tmp->nick);
		}
	}
	if (*list)
		say("Currently present:%s", list);
	if (all)
	{
		malloc_strcpy(&list, empty_string);
		for (tmp = notify_list; tmp; tmp = tmp->next)
		{
			if (!(tmp->flag))
			{
				malloc_strcat(&list, " ");
				malloc_strcat(&list, tmp->nick);
			}
		}
		if (*list) say("Currently absent:%s", list);
	}
	new_free(&list);
}

/* notify: the NOTIFY command.  Does the whole ball-o-wax */
/*ARGSUSED*/
void
notify(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*nick,
		*list = (char *) 0,
		*ptr;
	int	no_nicks = 1;
	int	do_ison = 0;
	NotifyList	*new;

	malloc_strcpy(&list, empty_string);
	while ((nick = next_arg(args, &args)) != NULL)
	{
		no_nicks = 0;
		while (nick)
		{
			if ((ptr = index(nick, ',')) != NULL)
				*ptr++ = '\0';
			if (*nick == '-')
			{
				nick++;
				if (*nick)
				{
					if ((new = (NotifyList *) remove_from_list(&notify_list, nick)) != NULL)
					{
						new_free(&(new->nick));
						new_free(&new);
						say("%s removed from notification list", nick);
					}
					else
						say("%s is not on the notification list", nick);
				}
				else
				{
					for (;(new = notify_list);)
					{
						notify_list = new->next;
						new_free(&new->nick);
						new_free(&new);
					}
					say("Notify list cleared");
				}
			}
			else
			{
				/* compatibility */
				if (*nick == '+')
					nick++;
				if (*nick)
				{
					do_ison = 1;
					if (index(nick, '*'))
						say("Wildcards not allowed in NOTIFY nicknames!");
					else
					{
						if ((new = (NotifyList *) remove_from_list(&notify_list, nick)) != NULL)
						{
							new_free(&(new->nick));
							new_free(&new);
						}
						new = (NotifyList *) new_malloc(sizeof(NotifyList));
						new->nick = (char *) 0;
						malloc_strcpy(&(new->nick), nick);
						new->flag = 0;
						add_to_list(&notify_list, new);
						from_server = primary_server;
						if (get_server_2_6_2(from_server))
						{
							malloc_strcat(&list, new->nick);
							malloc_strcat(&list, " ");
						}
						else
							add_to_whois_queue( new->nick, whois_notify, (char *) 0);
						say("%s added to the notification list", nick);
					}
				} else
					show_notify_list(0);
			}
			nick = ptr;
		}
	}
	if (do_ison)
		add_ison_to_whois(list, ison_notify);
	new_free(&list);
	if (no_nicks)
		show_notify_list(1);
}

/*
 * do_notify: This simply goes through the notify list, sending out a WHOIS
 * for each person on it.  This uses the fancy whois stuff in whois.c to
 * figure things out.  Look there for more details, if you can figure it out.
 * I wrote it and I can't figure it out.
 *
 * Thank you Michael... leaving me bugs to fix :) Well I fixed them!
 */
void
do_notify()
{
	static	int	location = 0;
	int	count,
		c2;
	char	buf[BIG_BUFFER_SIZE+1];
	NotifyList	*tmp;

	*buf = '\0';
	from_server = primary_server;
	for (tmp = notify_list, c2 = count = 0; tmp; tmp = tmp->next, count++)
	{
		if (count >= location && count < location + 40)
		{
			c2++;
			strcat(buf, " ");
			strcat(buf, tmp->nick);
		}
	}
	if (c2)
		add_ison_to_whois(buf, ison_notify);
	if ((location += 40) > count)
		location = 0;
}

/*
 * notify_mark: This marks a given person on the notify list as either on irc
 * (if flag is 1), or not on irc (if flag is 0).  If the person's status has
 * changed since the last check, a message is displayed to that effect.  If
 * the person is not on the notify list, this call is ignored 
 * doit if passed as 0 means it comes from a join, or a msg, etc, not from
 * an ison reply.  1 is the other..
 */
void
notify_mark(nick, flag, doit)
	char	*nick;
	int	flag;
	int	doit;
{
	NotifyList	*tmp;
	char	*s = get_string_var(NOTIFY_HANDLER_VAR);

	if (!s || (!doit && 'O' == *s))		/* old notify */
		return;	
	if ('N' == *s)			/* noisy notify */
		doit = 1;
	if ((tmp = (NotifyList *) list_lookup(&notify_list, nick,
			!USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
	{
		if (flag)
		{
			if (tmp->flag != 1)
			{
				if (tmp->flag != -1 && do_hook(NOTIFY_SIGNON_LIST, "%s", nick) && doit)
					if (get_int_var(USERHOST_NOTIFY_VAR)) {
					  add_userhost_to_whois(nick, userhost_notify);
					} else
					  put_it("%s", parseformat(NOTIFY_SIGNON_FMT, nick));
				/*
				 * copy the correct case of the nick
				 * into our array  ;)
				 */
				malloc_strcpy(&(tmp->nick), nick);
				malloc_strcpy(&last_notify_nick, nick);
				tmp->flag = 1;
			}
		}
		else
		{
			if (tmp->flag == 1 && do_hook(NOTIFY_SIGNOFF_LIST, "%s", nick) && doit)
				put_it("%s", parseformat(NOTIFY_SIGNOFF_FMT, nick));
			tmp->flag = 0;
		}
	}
}

void
save_notify(fp)
	FILE	*fp;
{
	NotifyList	*tmp;

	if (notify_list)
	{
		fprintf(fp, "NOTIFY");
		for (tmp = notify_list; tmp; tmp = tmp->next)
			fprintf(fp, " %s", tmp->nick);
		fprintf(fp, "\n");
	}
}

/* I hate broken compilers -mrg */
static	char	*vals[] = { "NOISY", "QUIET", "OLD", (char *) 0 };

void
set_notify_handler(value)
	char	*value;
{
	int	len;
	int	i;
	char	*s;

	if (!value)
		value = empty_string;
	for (i = 0, len = strlen(value); (s = vals[i]); i++)
		if (0 == my_strnicmp(value, s, len))
			break;
	set_string_var(NOTIFY_HANDLER_VAR, s);
	return;
}
