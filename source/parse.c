/*
 * parse.c: handles messages from the server.   Believe it or not.  I
 * certainly wouldn't if I were you. 
 *
 * Written By Timothy Jensen
 *
 * Copyright (c) 1999-2001
 *
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: parse.c,v 1.17 2001/12/15 02:33:42 toast Exp $";
#endif

#include "irc.h"

#include "server.h"
#include "names.h"
#include "vars.h"
#include "ctcp.h"
#include "hook.h"
#include "edit.h"
#include "ignore.h"
#include "whois.h"
#include "lastlog.h"
#include "ircaux.h"
#include "funny.h"
#include "crypt.h"
#include "term.h"
#include "flood.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "numbers.h"
#include "parse.h"
#include "notify.h"
#include "status.h"
#include "comstud.h"
#include "toast.h"
#include "format.h"
#include "cignore.h"
#include "rejoin.h"
#include "recorder.h"

#define STRING_CHANNEL '+'
#define MULTI_CHANNEL '#'
#define LOCAL_CHANNEL '&'
#define CHANNEL_OPS '@'

#define	MAXPARA	15	/* Taken from the ircd */

extern	void	parse_notice();

/*
 * joined_nick: the nickname of the last person who joined the current
 * channel 
 */
	char	*joined_nick = (char *) 0;

/* public_nick: nick of the last person to send a message to your channel */
	char	*public_nick = (char *) 0;

/* User and host information from server 2.7 */
	char	*FromUserHost = (char *) 0;

/* doing a PRIVMSG */
	int	doing_privmsg = 0;

/*
 * is_channel: determines if the argument is a channel.  If it's a number,
 * begins with MULTI_CHANNEL and has no '*', or STRING_CHANNEL, then its a
 * channel 
 */
int
is_channel(to)
char	*to;
{
	int	Version;

	Version = get_server_version(from_server);
	return ((Version < Server2_7 && (isdigit(*to) || (*to == STRING_CHANNEL)
		|| *to == '-'))
		|| (Version > Server2_5 && *to == MULTI_CHANNEL)
		|| (Version > Server2_7 && *to == LOCAL_CHANNEL));
}


/*
 * is_channel_ops: determines if the argument is a channel and has a
 * CHANNEL_OPS prefix to indicate a message for ops.
 */
int
is_channel_ops(to)
char	*to;
{
	/*
	 * Who cares about server versions.  Isn't everyone running a 2.8
	 * derivative or higher?  Guess I'll find out! :)
	 */
	return ((*to == CHANNEL_OPS) && ((to[1] == MULTI_CHANNEL) ||
		(to[1] == LOCAL_CHANNEL)));
}


char	*
PasteArgs(Args, StartPoint)
	char	**Args;
	int	StartPoint;
{
	int	i;

	for (; StartPoint; Args++, StartPoint--)
		if (!*Args)
			return (char *) 0;
	for (i = 0; Args[i] && Args[i+1]; i++)
		Args[i][strlen(Args[i])] = ' ';
	Args[1] = (char *) 0;
	return Args[0];
}

/*
 * BreakArgs: breaks up the line from the server, in to where its from,
 * setting FromUserHost if it should be, and returns all the arguements
 * that are there.   Re-written by phone, dec 1992.
 */
void
BreakArgs(Input, Sender, OutPut)
	char	*Input;
	char	**Sender;
	char	**OutPut;
{
	char	*s = Input,
		*t;
	int	ArgCount = 0;

	/*
	 * Get sender from :sender and user@host if :nick!user@host
	 */
	FromUserHost = (char *) 0;

	if (*Input == ':')
	{
		char	*tmp;
		*Input++ = '\0';
		if ((s = (char *) index(Input, ' ')) != (char *) 0)
			*s++ = '\0';
		*Sender = Input;
		if ((tmp = (char *) index(*Sender, '!')) != (char *) 0)
		{
			*tmp++ = '\0';
			FromUserHost = tmp;
		}
	}
	else
		*Sender = empty_string;

	if (!s)
		return;

	for (;;)
	{
		while (*s == ' ')
			*s++ = '\0';

		if (!*s)
			break;

		if (*s == ':')
		{
			for (t = s; *t; t++)
				*t = *(t + 1);
			OutPut[ArgCount++] = s;
			break;
		}
		OutPut[ArgCount++] = s;
		if (ArgCount >= MAXPARA)
			break;

		for (; *s != ' ' && *s; s++)
			;
	}
	OutPut[ArgCount] = (char *) 0;
}

/* beep_em: Not hard to figure this one out */
void
beep_em(beeps)
	int	beeps;
{
	int	cnt,
		i;

	for (cnt = beeps, i = 0; i < cnt; i++)
		term_beep();
}

/* in response to a TOPIC message from the server */
static	void
topic(from, ArgList)
	char	*from,
		**ArgList;
{
	int	flag;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	if (flag == IGNORED)
		return;

	if (!ArgList[1])
	{
		message_from((char *) 0, LOG_CRAP);
		if (do_hook(TOPIC_LIST, "%s * %s", from, ArgList[0]))
			say("[%s] Topic by %s: %s", TimeStamp(), from,
				ArgList[0]);
		message_from((char *) 0, LOG_CURRENT);
	}
	else
	{
		if (is_cignored(ArgList[0], CIGNORE_TOPICS))
			return;
		message_from(ArgList[0], LOG_CRAP);
		if (do_hook(TOPIC_LIST, "%s %s %s", from, ArgList[0], ArgList[1]))
			put_it("%s", parseformat(TOPIC_FMT, from, ArgList[0], ArgList[1]));
		message_from((char *) 0, LOG_CURRENT);
	}
}

static	void
linreply(ArgList)
	char	**ArgList;
{
	PasteArgs(ArgList, 0);
	say("%s", ArgList[0]);
}

static	void
p_wall(from, ArgList)
	char	*from,
		**ArgList;
{
	int	flag,
		level;
	char	*line;

if (!from)
		return;
	PasteArgs(ArgList, 0);
	if (!(line = ArgList[0]))
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_WALLS);
	message_from(from, LOG_WALL);
	if (flag != IGNORED)
	{
		if (flag == HIGHLIGHTED)
			do_highlight(1);
		else
			do_highlight(0);
		if ((flag != DONT_IGNORE) && (ignore_usernames & IGNORE_WALLS)
				&& !FromUserHost)
			add_to_whois_queue(from, whois_ignore_walls, "%s",line);
		else
		{
			level = set_lastlog_msg_level(LOG_WALL);
			if (check_flooding(from, WALL_FLOOD, "*", line) &&
					do_hook(WALL_LIST, "%s %s", from, line))
				put_it("%s", parseformat(WALL_FMT, from, line));
			if (beep_on_level & LOG_WALL)
				beep_em(1);
			set_lastlog_msg_level(level);
		}
	}
	message_from((char *) 0, LOG_CURRENT);
}

static	void
wallops(from, ArgList)
	char	*from,
		**ArgList;
{
	int	flag,
		level;
	char	*line;

if (!from)
		return;
	if (!(line = PasteArgs(ArgList, 0)))
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_WALLOPS);
	if (index(from, '.'))
	{
	/* The old server check, don't use the whois stuff for servers */
		int	level;

		if (flag != IGNORED)
		{
			if (flag == HIGHLIGHTED)
				do_highlight(1);
			else
				do_highlight(0);
			message_from(from, LOG_WALLOP);
			level = set_lastlog_msg_level(LOG_WALLOP);
			if (do_hook(WALLOP_LIST, "%s S %s", from, line))
			{
				put_it("%s", parseformat(SERVER_WALLOPS_FMT, from, line));
			}
			if (beep_on_level & LOG_WALLOP)
				beep_em(1);
			set_lastlog_msg_level(level);
			message_from((char *) 0, LOG_CRAP);
		}
	}
	else
	{
		if (strcmp(from, get_server_nickname(get_window_server(0)))
			== 0)
		{
			message_from(from, LOG_WALLOP);
			level = set_lastlog_msg_level(LOG_WALLOP);
			put_it("%s", parseformat(WALLOPS_FMT, from, line,
				get_server_operator(get_window_server(0)) ? "*"
				: empty_string));
			if (beep_on_level & LOG_WALLOP)
				beep_em(1);
			set_lastlog_msg_level(level);
			message_from((char *) 0, LOG_CRAP);
		}
		else if (get_int_var(USER_WALLOPS_VAR))
		{
			if ((flag != DONT_IGNORE) &&
				(check_flooding(from, WALLOP_FLOOD, "*", line)))
			{
				add_to_whois_queue(from, whois_new_wallops,
					"%s", line);
			}
		}
	}
}

static void
add_autowho(channel)
	char	*channel;
{
	AutoWho	*curr;

	if (!channel)
		return;

	curr = (AutoWho *) new_malloc(sizeof(AutoWho));
	curr->channel = (char *) 0;
	malloc_strcpy(&curr->channel, channel);
	curr->next = server_list[from_server].autowho;
	server_list[from_server].autowho = curr;
}

void
remove_autowho(channel)
	char	*channel;
{
	AutoWho	*curr, *last, *next;

	if (!channel)
		return;

	last = (AutoWho *) 0;
	curr = server_list[from_server].autowho;

	while (curr) {
		next = curr->next;
		if (!my_stricmp(curr->channel, channel)) {
			if (last)
				last->next = next;
			else
				server_list[from_server].autowho = next;
			new_free(&curr->channel);
			new_free(&curr);
		} else
			last = curr;
		curr = next;
	}
}

static int
is_autowho(channel)
	char	*channel;
{
	AutoWho	*curr;

	if (!channel)
		return 0;

	curr = server_list[from_server].autowho;
	while (curr) {
		if (!my_stricmp(curr->channel, channel))
			return 1;
		curr = curr->next;
	}
	return 0;
}

/*ARGSUSED*/
void
whoreply(from, ArgList)
	char	**ArgList,
		*from;
{
	static	char	format[40];
	static	int	last_width = -1;
	int	ok = 1, voice, opped;
	char	*channel,
		*user,
		*host,
		*server,
		*nick,
		*stat,
		*name;
	int	i;

	FILE	*fip;
	char	buf_data[BUFSIZ];

	if (last_width != get_int_var(CHANNEL_NAME_WIDTH_VAR))
	{
		if ((last_width = get_int_var(CHANNEL_NAME_WIDTH_VAR)) != 0)
		    sprintf(format, "%%-%u.%us %%-9s %%-3s %%s@%%s (%%s)",
					(unsigned char) last_width,
					(unsigned char) last_width);
		else
		    strcpy(format, "%s\t%-9s %-3s %s@%s (%s)");
	}
	i = 0;
	channel = user = host = server = nick = stat = name = empty_string;
	if (ArgList[i])
		channel = ArgList[i++];
	if (ArgList[i])
		user = ArgList[i++];
	if (ArgList[i])
		host = ArgList[i++];
	if (ArgList[i])
		server = ArgList[i++];
	if (ArgList[i])
		nick = ArgList[i++];
	if (ArgList[i])
		stat = ArgList[i++];
	PasteArgs(ArgList, i);

	if (*stat == 'S')	/* this only true for the header WHOREPLY */
	{
		channel = "Channel";
		if (((who_mask & WHO_FILE) == 0) || (fopen (who_file, "r")))
		{
			if (do_hook(WHO_LIST, "%s %s %s %s %s %s", channel,
					nick, stat, user, host, ArgList[6]))
				put_it(format, channel, nick, stat, user,
					host, ArgList[6]);
			return;
		}
	}

	if (ArgList[i])
		name = ArgList[i];
	strcpy(buf_data, user);
	strcat(buf_data, "@");
	strcat(buf_data, host);
	voice = (strchr(stat, '+') != NULL);
	opped = (strchr(stat, '@') != NULL);
	add_to_channel(channel, nick, from_server, opped, voice, buf_data);
	if (is_autowho(channel))
		return;
	if (toast_action == TACTION_CSTAT)
	{
		handle_cstat(stat, atoi(name));
		return;
	}
	if (com_action == 2)
	{
		if (get_int_var(VERBOSE_WHOKILL_VAR))
			handle_whokill(nick, user, host, server);
		else
			handle_masskill(nick);
		return;
	}
	if (who_mask)
	{
		if (who_mask & WHO_HERE)
			ok = ok && (*stat == 'H');
		if (who_mask & WHO_AWAY)
			ok = ok && (*stat == 'G');
		if (who_mask & WHO_OPS)
			ok = ok && (*(stat + 1) == '*');
		if (who_mask & WHO_LUSERS)
			ok = ok && (*(stat + 1) != '*');
		if (who_mask & WHO_CHOPS)
			ok = ok && ((*(stat + 1) == '@') ||
			(*(stat + 2) == '@'));
		if (who_mask & WHO_NAME)
			ok = ok && wild_match(who_name, user);
		if (who_mask & WHO_NICK)
			ok = ok && wild_match(who_nick, nick);
		if (who_mask & WHO_HOST)
			ok = ok && wild_match(who_host, host);
		if (who_mask & WHO_REAL)
			ok = ok && wild_match(who_real, name);
		if (who_mask & WHO_SERVER)
			ok = ok && wild_match(who_server, server);
		if (who_mask & WHO_FILE)
		{
			ok = 0;
			cannot_open = (char *) 0;
			if ((fip = fopen (who_file, "r")) != (FILE *) 0)
			{
				while (fgets (buf_data, BUFSIZ, fip) !=
								(char *) 0)
				{
					buf_data[strlen(buf_data)-1] = '\0';
					ok = ok || wild_match(buf_data, nick);
				}
				fclose (fip);
			} else
				cannot_open = who_file;
		}
	}
	if (ok)
	{
		    if (do_hook(WHO_LIST, "%s %s %s %s %s %s", channel, nick,
				stat, user, host, name))
		    {
			if (get_int_var(SHOW_WHO_HOPCOUNT_VAR))
				put_it(format, channel, nick, stat, user, host,
					name);
			else
			{
				char	*tmp;

				if ((tmp = (char *) index(name, ' ')) !=
								(char *) 0)
					tmp++;
				else
					tmp = name;
				put_it(format, channel, nick, stat, user, host,
					tmp);
			}
		    }
	}
}

static	void
p_privmsg(from, Args)
	char	*from,
		**Args;
{
	int	level,
		flag,
		list_type,
		flood_type,
		log_type;
	unsigned char	ignore_type;
	char	*ptr,
		*to,
	        *timestr;
	char    junk[600];
	int	no_flood;
	static int com_do_log, com_lines = 0;

	if (!from)
		return;
	PasteArgs(Args, 1);
	to = Args[0];
	ptr = Args[1];
	if (!to || !ptr)
		return;
	if (is_channel(to))
	{
		message_from(to, LOG_MSG);
		malloc_strcpy(&public_nick, from);
		if (!is_on_channel(to, from))
		{
			log_type = LOG_PUBLIC;
			ignore_type = IGNORE_PUBLIC;
			list_type = PUBLIC_MSG_LIST;
			flood_type = PUBLIC_FLOOD;
		}
		else
		{
			log_type = LOG_PUBLIC;
			ignore_type = IGNORE_PUBLIC;
			if (is_current_channel(to, 0))
				list_type = PUBLIC_LIST;
			else
				list_type = PUBLIC_OTHER_LIST;
			flood_type = PUBLIC_FLOOD;
		}
	}
	else if (is_channel_ops(to))
	{
		to++;
		message_from(to, LOG_MSG);
		malloc_strcpy(&public_nick, from);
		log_type = LOG_PUBLIC;
		ignore_type = IGNORE_PUBLIC;
		if (is_current_channel(to, 0))
			list_type = OPS_LIST;
		else
			list_type = OPS_OTHER_LIST;
		flood_type = PUBLIC_FLOOD;
	}
	else
	{
		message_from(from, LOG_MSG);
		flood_type = MSG_FLOOD;
		if (my_stricmp(to, get_server_nickname(from_server)))
		{
			log_type = LOG_WALL;
			ignore_type = IGNORE_WALLS;
			list_type = MSG_GROUP_LIST;
		}
		else
		{
			log_type = LOG_MSG;
			ignore_type = IGNORE_MSGS;
			list_type = MSG_LIST;
		}
	}
	timestr = TimeStamp();
	flag = double_ignore(from, FromUserHost, ignore_type);
	switch (flag)
	{
	case IGNORED:
		if ((list_type == MSG_LIST) && do_warn(from, FromUserHost))
			send_to_server("NOTICE %s :%s is ignoring you", from,
					get_server_nickname(from_server));
		return;
	case HIGHLIGHTED:
		do_highlight(1);
		break;
	default:
		do_highlight(0);
		break;
	}
	ptr = do_ctcp(from, to, ptr);
	if (!*ptr)
		return;
	level = set_lastlog_msg_level(log_type);
	com_do_log = 0;
	if (get_string_var(PUBLOGSTR_VAR))
	{
		char *temp;
		temp = get_string_var(PUBLOGSTR_VAR);
		if (*temp && my_stristr(ptr, temp))
		{
			com_do_log = 1;
			com_lines = 0;
		}
	}
	if ((flag != DONT_IGNORE) && (ignore_usernames & ignore_type) && !FromUserHost)
		add_to_whois_queue(from, whois_ignore_msgs, "%s", ptr);
	else
	{
		no_flood = check_flooding(from, flood_type, to, ptr);
		if ((sed == 1) && (!do_hook(ENCRYPTED_PRIVMSG_LIST,"%s %s %s",from, to, ptr)))
			sed = 0;
		else
		{
		switch (list_type)
		{
		case PUBLIC_MSG_LIST:
			if (is_cignored(to, CIGNORE_MSGS))
				break;
			if (no_flood && do_hook(list_type, "%s %s %s", from, to, ptr))
			    put_it("%s", parseformat(PUBLIC_MSG_FMT, from, to, ptr));
			break;
		case MSG_GROUP_LIST:
			if (no_flood && do_hook(list_type, "%s %s %s", from, to, ptr))
			    put_it("%s", parseformat(MSG_GROUP_FMT, from, to, ptr));
			if (server_list[from_server].away &&
					get_int_var(AWAY_RECORDER_VAR)) {
				record_msg(time(NULL), from, FromUserHost, ptr);
				update_all_status();
			}
			break;
		case MSG_LIST:
			if (!no_flood)
				break;
			malloc_strcpy(&recv_nick, from);

			if (away_set)
				beep_em(get_int_var(BEEP_WHEN_AWAY_VAR));
			logmsg(0, "*", from, ptr, FromUserHost, 0);
			addnick(from);
			if (do_hook(list_type, "%s %s", from, ptr))
			{
				char *msg;
				msg = parseformat(MSG_FMT, from, to, ptr);
				put_it("%s", msg);
				set_string_var(LAST_MSG_VAR, msg);
			}
			if (server_list[from_server].away &&
					get_int_var(AWAY_RECORDER_VAR)) {
				record_msg(time(NULL), from, FromUserHost, ptr);
				update_all_status();
			}
			break;
		case PUBLIC_LIST:
			if (is_cignored(to, CIGNORE_MSGS))
				break;
			doing_privmsg = 1;
			if (com_do_log || com_lines)
			{
				com_lines++;
				if (com_lines == 5)
					com_lines = 0;
				logmsg(0, "<", from, ptr, FromUserHost, 0);
			}
			if (no_flood && do_hook(list_type, "%s %s %s", from, 
			    to, ptr)) {
				int rn = get_refnum_by_channel(to);
				if (!is_last_current_channel(rn, to)) {
				  set_last_current_channel(rn, to);
				  say("You are now talking to channel %s", to);
				}
				put_it("%s", parseformat(PUBLIC_FMT, from, to, ptr));
			}
			doing_privmsg = 0;
			break;
		case OPS_LIST:
			if (is_cignored(to, CIGNORE_MSGS))
				break;
			doing_privmsg = 1;
			if (com_do_log || com_lines)
			{
				com_lines++;
				if (com_lines == 5)
					com_lines = 0;
				logmsg(0, "<", from, ptr, FromUserHost, 0);
			}
			if (no_flood && do_hook(list_type, "%s %s %s", from, 
			    to, ptr)) {
				int rn = get_refnum_by_channel(to);
				if (!is_last_current_channel(rn, to)) {
				  set_last_current_channel(rn, to);
				  say("You are now talking to channel %s", to);
				}
				put_it("%s", parseformat(OPS_FMT, from, to, ptr));
			}
			doing_privmsg = 0;
			break;
		case PUBLIC_OTHER_LIST:
			if (is_cignored(to, CIGNORE_MSGS))
				break;
			doing_privmsg = 1;
			if (com_do_log || com_lines)
			{
				com_lines++;
				if (com_lines == 5)
					com_lines = 0;
				logmsg(0, "<", from, ptr, FromUserHost, 0);
			}
			if (no_flood && do_hook(list_type, "%s %s %s", from,
			    to, ptr))
				put_it("%s", parseformat(PUBLIC_OTHER_FMT, from, to ,ptr));
			doing_privmsg = 0;
			break;
		case OPS_OTHER_LIST:
			if (is_cignored(to, CIGNORE_MSGS))
				break;
			doing_privmsg = 1;
			if (com_do_log || com_lines)
			{
				com_lines++;
				if (com_lines == 5)
					com_lines = 0;
				logmsg(0, "<", from, ptr, FromUserHost, 0);
			}
			if (no_flood && do_hook(list_type, "%s %s %s", from,
			    to, ptr))
				put_it("%s", parseformat(OPS_OTHER_FMT, from, to ,ptr));
			doing_privmsg = 0;
			break;
		}
		if (beep_on_level & log_type)
			beep_em(1);
		}
	}
	set_lastlog_msg_level(level);
	message_from((char *) 0, LOG_CURRENT);
}

static	void
msg(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*channel,
		*text,
	        *timestr;
	int	log_type,
		no_flooding;
	int	flag;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_PUBLIC);
	switch (flag)
	{
	case IGNORED:
		return;
	case HIGHLIGHTED:
		do_highlight(1);
		break;
	default:
		do_highlight(0);
		break;
	}
	timestr = TimeStamp();
	if ((channel = real_channel()) == (char *) 0)
		return;
	text = do_ctcp(from, channel, ArgList[0]);
	if (text == (char *) 0)
		return;
	message_from(channel, LOG_PUBLIC);
	malloc_strcpy(&public_nick, from);
	log_type = set_lastlog_msg_level(LOG_PUBLIC);
	no_flooding = check_flooding(from, PUBLIC_FLOOD, channel, text);
	if (!is_cignored(channel, CIGNORE_MSGS))
	{
	   if (is_current_channel(channel, 0))
	   {
		doing_privmsg = 1;
		if (no_flooding && do_hook(PUBLIC_LIST, "%s %s %s", from, channel, text))
			put_it("%s", parseformat(PUBLIC_FMT, from, channel, text));
		doing_privmsg = 0;
	   }
	   else
	   {
		doing_privmsg = 1;
		if (no_flooding && do_hook(PUBLIC_OTHER_LIST, "%s %s %s", from,
				channel, text))
			put_it("%s", parseformat(PUBLIC_OTHER_FMT, from, channel, text));
		doing_privmsg = 0;
	   }
	   if (beep_on_level & LOG_PUBLIC)
		beep_em(1);
	}
	set_lastlog_msg_level(log_type);
}

/*ARGSUSED*/
static	void
p_quit(from, ArgList)
	char	*from,
		**ArgList;
{
	int	one_prints = 0;
	char	*chan;
	char	*Reason;
	int	flag;

	if (!from)
		return;
	remove_autoop_all(from);
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	if (flag != IGNORED)
	{
		PasteArgs(ArgList, 0);
		Reason = ArgList[0] ? ArgList[0] : "?";
		for (chan = walk_channels(from, 1); chan;
				chan = walk_channels(from, 0))
		{
			message_from(chan, LOG_CRAP);
			if (do_hook(CHANNEL_SIGNOFF_LIST, "%s %s %s", chan, from,
					Reason))
				one_prints = 1;
		}
		if (one_prints)
		{
			message_from(what_channel(from), LOG_CRAP);
			if (do_hook(SIGNOFF_LIST, "%s %s", from, Reason))
				put_it("%s", parseformat(QUIT_FMT, from, Reason));
		}
	}
	notify_mark(from, 0, 0);
	remove_from_channel((char *) 0, from, from_server);
	message_from((char *) 0, LOG_CURRENT);

	/*
	 * someone wanted this for some reason.  i'm not so sure about
	 * it though.
	 */
#if 0
	if (!my_stricmp(from, get_server_nickname(from_server))
		close_server(from_server, empty_string);
#endif
}

/*ARGSUSED*/
static	void
pong(from, ArgList)
	char	*from,
		**ArgList;
{
	int	flag;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	if (flag == IGNORED)
		return;

	if (ArgList[0])
		say("%s: PONG received from %s", ArgList[0], from);
}

/*ARGSUSED*/
static	void
error(from, ArgList)
	char	*from,
		**ArgList;
{
	PasteArgs(ArgList, 0);
	if (!ArgList[0])
		return;
	say("%s", ArgList[0]);
}

static	void
p_channel(from, ArgList)
	char	*from;
	char	**ArgList;
{
	int	join;
	char	*channel;
	int	flag;
	char	*s;
	int	chan_oper = 0, chan_voice = 0;
	int	rejoin;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	if (strcmp(ArgList[0], "0"))
	{
		join = 1;
		channel = ArgList[0];
		/*
		 * this \007 should be \a but a lot of compilers are
		 * broken.  *sigh*  -mrg
		 */
		if (s = index(channel, '\007'))
		{
			*s = '\0';
			while (*++s)
			{
				if (*s == 'o')
					chan_oper = 1;
				if (*s == 'v')
					chan_voice = 1;

			}
		}
		message_from(channel, LOG_CRAP);
		malloc_strcpy(&joined_nick, from);
		notify_mark(from, 1, 0);
	}
	else
	{
		join = 0;
		if ((channel = real_channel()) == (char *) 0)
			return;
		notify_mark(from, 0, 0);
		message_from(channel, LOG_CRAP);
		if (flag != IGNORED && do_hook(LEAVE_LIST, "%s %s", from,
								channel))
			say("%s has left channel %s at %s", from, channel, TimeStamp());
		message_from((char *) 0, LOG_CURRENT);
	}
	if (!my_stricmp(from, get_server_nickname(from_server)))
	{
		if (join)
		{
			rejoin = is_on_rejoin_list(channel, from_server);
			add_channel(channel, from_server, rejoin);
			if (get_server_version(from_server) == Server2_5)
				send_to_server("NAMES %s", channel);
			send_to_server("MODE %s", channel);
			add_autowho(channel);
			if (rejoin)
			{
				rejoin_kill(channel, from_server);
				set_last_current_channel(0, NULL);
			}
			else
			{
				set_last_current_channel(0, channel);
			}
			send_to_server("WHO %s", channel);
			funny_set_ignore_channel(channel);
		}
		else
			remove_channel(channel, from_server);
	}
	else
	{
		if (join)
		{
			add_to_channel(channel, from, from_server, chan_oper, chan_voice, FromUserHost);
		}
		else
			remove_from_channel(channel, from, from_server);
	}
	if (join)
	{
		if (FromUserHost && *FromUserHost &&
				get_int_var(AUTOOP_ENABLE_VAR))
			handle_autoop(from, channel, FromUserHost);
		if (!get_channel_oper(channel, from_server))
			in_on_who = 1;
		if (!is_cignored(channel, CIGNORE_JOINS) && (flag != IGNORED) &&
					do_hook(JOIN_LIST, "%s %s", from, channel))
		{
			char *tmp, *tmp2;
			message_from(channel, LOG_CRAP);
			if (!FromUserHost)
				FromUserHost = empty_string;
			tmp = strchr(FromUserHost, '@');
			tmp++;
			if (!my_stricmp(from, get_server_nickname(from_server)))
				put_it("%s", parseformat(JOIN_SELF_FMT, from, channel));
			else
				put_it("%s", parseformat(JOIN_FMT, from, channel));
			if (get_int_var(AUTO_NSLOOKUP_VAR) &&
				*tmp && isdigit(*(tmp+strlen(tmp)-1)))
			{
				tmp2 = do_nslookup(tmp);       
				say("%s is %s", tmp, tmp2 ? tmp2 : "unknown.");	
			}
			message_from((char *) 0, LOG_CURRENT);
		}
		in_on_who = 0;
	}
}

static	void
p_invite(from, ArgList)
	char	*from,
		**ArgList;
{
	int	flag;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_INVITES);
	switch (flag)
	{
	case IGNORED:
		if (get_int_var(SEND_IGNORE_MSG_VAR))
			send_to_server("NOTICE %s :%s is ignoring you",
				from, get_server_nickname(from_server));
		return;
	case HIGHLIGHTED:
		do_highlight(1);
		break;
	default:
		do_highlight(0);
		break;
	}
	if (ArgList[0] && ArgList[1])
	{
		if ((flag != DONT_IGNORE) && (ignore_usernames & IGNORE_INVITES)
		    && !FromUserHost)
			add_to_whois_queue(from, whois_ignore_invites,
					"%s", ArgList[1]);
		else
		{
			message_from(from, LOG_CRAP);
			if (check_flooding(from, INVITE_FLOOD, ArgList[0], ArgList[1]) &&
				do_hook(INVITE_LIST, "%s %s", from, ArgList[1]))
				put_it("%s", parseformat(INVITE_FMT, from, ArgList[0], ArgList[1]));
			malloc_strcpy(&invite_channel, ArgList[1]);
			malloc_strcpy(&recv_nick, from);
		}
	}
}

static	void
server_kill(from, ArgList)
	char	*from,
		**ArgList;
{
	/*
	 * this is so bogus checking for a server name having a '.'
	 * in it - phone, april 1993.
	 */
	if (index(from, '.'))
		say("You have been rejected by server %s", from);
	else
	{
		say("You have been killed by operator %s %s", from,
			ArgList[1] ? ArgList[1] : "(No Reason Given)");
	}
/*
	say("Use /SERVER to reconnect to a server");
	close_server(from_server, empty_string);
	window_check_servers();
*/
	do_reconnect((char *) 0);
}

static	void
ping(ArgList)
	char	**ArgList;
{
	PasteArgs(ArgList, 0);
	send_to_server("PONG :%s", ArgList[0]);
}

static	void
p_nick(from, ArgList)
	char	*from,
		**ArgList;
{
	int	one_prints = 0,
		its_me = 0;
	char	*chan;
	char	*line;
	int	flag;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	line = ArgList[0];
	rename_autoop(from, line);
	if (my_stricmp(from, get_server_nickname(from_server)) == 0){
		if (from_server == primary_server)
			strmcpy(nickname, line, NICKNAME_LEN);
		set_server_nickname(from_server, line);
		its_me = 1;
	}
	if (flag != IGNORED)
	{
		for (chan = walk_channels(from, 1); chan;
				chan = walk_channels(from, 0))
		{
			message_from(chan, LOG_CRAP);
			if (do_hook(CHANNEL_NICK_LIST, "%s %s %s", chan, from, line))
				one_prints = 1;
		}
		if (one_prints)
		{
			if (its_me)
				message_from((char *) 0, LOG_CRAP);
			else
				message_from(what_channel(from), LOG_CRAP);
			if (do_hook(NICKNAME_LIST, "%s %s", from, line))
				put_it("%s", parseformat(NICK_FMT, from, line));
		}
	}
	rename_nick(from, line, from_server);
}

static	void
mode(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*channel;
	char	*line;
	int	flag;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	PasteArgs(ArgList, 1);
	channel = ArgList[0];
	line = ArgList[1];
	message_from(channel, LOG_CRAP);
	if (channel && line)
	{
		if (is_channel(channel))
		{
			if (!is_cignored(channel, CIGNORE_MODES) && (flag != IGNORED)
					&& do_hook(MODE_LIST, "%s %s %s", from,
					channel, line))
				put_it("%s", parseformat(MODE_FMT, from,
					channel, line));
			update_channel_mode(channel, line);
		}
		else
		{
			if (flag != IGNORED && do_hook(MODE_LIST, "%s %s %s",
					from, channel, line))
				put_it("%s", parseformat(UMODE_FMT, from,
					channel, line));
			update_user_mode(line);
		}
		update_all_status();
	}
}

static	void
kick(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*channel,
		*who,
		*comment;

	if (!from)
		return;
	channel = ArgList[0];
	who = ArgList[1];
	comment = ArgList[2];

	message_from(channel, LOG_CRAP);
	if (channel && who)
	{
		if (my_stricmp(who, get_server_nickname(from_server)) == 0)
		{
			int art;
			art=get_int_var(AUTO_REJOIN_VAR);
			if (art)
			{
				char username[255], *ptr;
				ptr = (char *) 0;
				if (art == 2)
				{
					if (FromUserHost && strchr(FromUserHost, '@'))
					{
						strcpy(username, FromUserHost);
						*strchr(username, '@') = '\0';
						ptr = username;
						if (ptr && (*ptr == '~'))
							ptr++;
						if (ptr && (*ptr == '#'))
							ptr++;
					}
					do_newuser(NULL, ptr, (char *) NULL);
				}
				if (art==3)
					send_to_server("NICK %s", random_str(3,9));
				if (art>=4)
				{
					do_newuser(NULL, random_str(2,9), (char *) NULL);
					if (art >= 5)
						send_to_server("NICK %s", random_str(3,9));
				}
				send_to_server("JOIN %s %s",
				channel, channel_key(channel));
			}
			remove_channel(channel, from_server);
			update_all_status();
			if (do_hook(KICK_LIST, "%s %s %s %s", who,
					from, channel, comment))
				put_it("%s", parseformat(KICK_SELF_FMT,
					from, channel, get_server_nickname(from_server),
					comment));
		}
		else
		{
			remove_from_channel(channel, who, from_server);
			if (!is_cignored(channel, CIGNORE_KICKS) && 
					do_hook(KICK_LIST, "%s %s %s %s", who,
					from, channel, comment))
				put_it("%s", parseformat(KICK_FMT, from, channel, who,
					comment));
		}
	}
}

static	void
part(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*channel;
	int	flag;

	if (!from)
		return;
	flag = double_ignore(from, FromUserHost, IGNORE_CRAP);
	channel = ArgList[0];
	remove_autoop_chan(from, channel);
	message_from(channel, LOG_CRAP);
	in_on_who = 1;
	if (my_stricmp(from, get_server_nickname(from_server)) == 0)
	{
		if (!is_cignored(channel, CIGNORE_PARTS) && (flag != IGNORED) &&
				do_hook(LEAVE_LIST, "%s %s", from, channel))
			put_it("%s", parseformat(PART_SELF_FMT, from, channel));
		remove_channel(channel, from_server);
		remove_from_mode_list(channel);
	}
	else {
		if (!is_cignored(channel, CIGNORE_PARTS) && (flag != IGNORED) &&
				do_hook(LEAVE_LIST, "%s %s", from, channel))
			put_it("%s", parseformat(PART_FMT, from, channel));
		remove_from_channel(channel, from, from_server);
	}
	in_on_who = 0;
}


/*
 * parse_server: parses messages from the server, doing what should be done
 * with them 
 */
void
parse_server(line)
	char	*line;
{
	char	*from,
		*comm,
		*end,
		*copy = (char *) 0;
	int	numeric;
	char	**ArgList;
	char	*TrueArgs[MAXPARA + 1];

	if ((char *) 0 == line)
		return;

	end = strlen(line) + line;
	if (*--end == '\n')
		*end-- = '\0';
	if (*end == '\r')
		*end-- = '\0';

	if (*line == ':')
	{
		if (!do_hook(RAW_IRC_LIST, "%s", line + 1))
			return;
	}
	else if (!do_hook(RAW_IRC_LIST, "%s %s", "*", line))
		return;

	malloc_strcpy(&copy, line);
	ArgList = TrueArgs;
	BreakArgs(copy, &from, ArgList);

	if (!(comm = (*ArgList++)))
		return;		/* Empty line from server - ByeBye */

	if (0 != (numeric = atoi(comm)))
		numbered_command(from, numeric, ArgList);
	else if (strcmp(comm, "WHOREPLY") == 0)
		whoreply(from, ArgList);
	else if (strcmp(comm, "NOTICE") == 0)
		parse_notice(from, ArgList);
	else if (strcmp(comm, "PRIVMSG") == 0)
		p_privmsg(from, ArgList);
	else if (strcmp(comm, "NAMREPLY") == 0)
		funny_namreply(from, ArgList);
	else if (strcmp(comm, "JOIN") == 0)
		p_channel(from, ArgList);
	else if (strcmp(comm, "PART") == 0)
		part(from, ArgList);
		/* CHANNEL will go away with 2.6 */
	else if (strcmp(comm, "CHANNEL") == 0)
		p_channel(from, ArgList);
	else if (strcmp(comm, "MSG") == 0)
		msg(from, ArgList);
	else if (strcmp(comm, "QUIT") == 0)
		p_quit(from, ArgList);
	else if (strcmp(comm, "WALL") == 0)
		p_wall(from, ArgList);
	else if (strcmp(comm, "WALLOPS") == 0)
		wallops(from, ArgList);
	else if (strcmp(comm, "LINREPLY") == 0)
		linreply(ArgList);
	else if (strcmp(comm, "PING") == 0)
		ping(ArgList);
	else if (strcmp(comm, "TOPIC") == 0)
		topic(from, ArgList);
	else if (strcmp(comm, "PONG") == 0)
		pong(from, ArgList);
	else if (strcmp(comm, "INVITE") == 0)
		p_invite(from, ArgList);
	else if (strcmp(comm, "NICK") == 0)
		p_nick(from, ArgList);
	else if (strcmp(comm, "KILL") == 0)
		server_kill(from, ArgList);
	else if (strcmp(comm, "MODE") == 0)
		mode(from, ArgList);
	else if (strcmp(comm, "KICK") == 0)
		kick(from, ArgList);
	else if (strcmp(comm, "ERROR") == 0)
		error(from, ArgList);
	else if (strcmp(comm, "ERROR:") == 0) /* Server bug makes this a must */
		error(from, ArgList);
	else
	{
		PasteArgs(ArgList, 0);
		if (from)
			say("Odd server stuff: \"%s %s\" (%s)", comm,
				ArgList[0], from);
		else
			say("Odd server stuff: \"%s %s\"", comm, ArgList[0]);
	}
	new_free(&copy);
	from_server = -1;
}
