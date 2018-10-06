/*
 * numbers.c:handles all those strange numeric response dished out by that
 * wacky, nutty program we call ircd 
 *
 *
 * Written by Timothy Jensen
 *
 * Copyright (c) 1999-2001 
 *
 * see the copyright file, or do a /help copyright 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: numbers.c,v 1.14 2001/12/08 04:55:55 toast Exp $";
#endif

#include "irc.h"

#include "input.h"
#include "ircaux.h"
#include "vars.h"
#include "lastlog.h"
#include "hook.h"
#include "server.h"
#include "whois.h"
#include "numbers.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "names.h"
#include "whois.h"
#include "funny.h"
#include "parse.h"
#include "comstud.h"
#include "toast.h"
#include "cignore.h"
#include "recorder.h"

extern	void	got_initial_version();
extern  int     check_screen_redirect();
extern  int     check_wait_command();

static	int	already_doing_reset_nickname = 0;
static  int	lusers_user_cnt = 0;
static	char	tempstr[1024];

/*
 * numeric_banner: This returns in a static string of either "xxx" where
 * xxx is the current numeric, or the value of BANNER if SHOW_NUMBERS is OFF 
 */
char	*
numeric_banner()
{
	static	char	thing[5];
	char	*foo = get_string_var(BANNER_VAR);

	if (get_int_var(SHOW_NUMERICS_VAR))
	{
		sprintf(thing, "%3.3u ", -current_numeric);
		return (thing);
	}
	else if (foo)
	{
		return foo;
	}
	else
	{
		return empty_string;
	}
}

/*
 * display_msg: handles the displaying of messages from the variety of
 * possible formats that the irc server spits out.  you'd think someone would
 * simplify this 
 */
void
display_msg(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*ptr;
	char	*rest;

	if (num_format_exists(-current_numeric)) {
		put_it("%s", numericformat(-current_numeric, from, numeric_banner(), ArgList));
		return;
	}
	if (from && (my_strnicmp(get_server_itsname(from_server), from,
			strlen(get_server_itsname(from_server))) == 0))
		from = (char *) 0;
	rest = PasteArgs(ArgList, 0);
	if ((ptr = (char *) index(rest, ':')) != NULL)
	{
		*(ptr++) = (char) 0;
		if (strlen(rest))
		{
			if (from)
				put_it("%s%s: %s (from %s)", numeric_banner(),
					rest, ptr, from);
			else
				put_it("%s%s: %s", numeric_banner(), rest,
					ptr);
		}
		else
		{
			if (from)
				put_it("%s%s (from %s)", numeric_banner(),
					ptr, from);
			else
				put_it("%s%s", numeric_banner(), ptr);
		}
	}
	else
	{
		if (from)
			put_it("%s%s (from %s)", numeric_banner(), rest, from);
		else
			put_it("%s%s", numeric_banner(), rest);
	}
}

/* smart_display: like display_msg in that it will display the server if
 * not the server you're on, but a little more simplistic.
 */
static	void
smart_display(from, line)
	char	*from;
	char	*line;
{
	if (!line)
		return;

	if (from && (my_strnicmp(get_server_itsname(from_server), from,
			strlen(get_server_itsname(from_server))) == 0))
		put_it("%s", line);
	else
		put_it("%s (from %s)", line, from);
}

/*
 * password_sendline: called by send_line() in get_password() to handle
 * hitting of the return key, etc 
 */
static	void
password_sendline(data, line)
	char	*data;
	char	*line;
{
	int	new_server;

	new_server = atoi(line);
	set_server_password(new_server, line);
	connect_to_server(get_server_name(new_server),
		get_server_port(new_server), -1);
}

/*
 * get_password: when a host responds that the user needs to supply a
 * password, it gets handled here!  the user is prompted for a password and
 * then reconnection is attempted with that password.  but, the reality of
 * the situation is that no one really uses user passwords.  ah well 
 */
static	void
get_password()
{
	char	server_num[8];

	say("password required for connection to server %s",
		get_server_name(from_server));
	close_server(from_server, empty_string);
        if (!dumb)
	{
		sprintf(server_num, "%d", from_server);
		add_wait_prompt("Server Password:", password_sendline,
			server_num, WAIT_PROMPT_LINE);
	}
}

/*ARGSUSED*/
static	void
nickname_sendline(data, nick)
	char	*data;
	char	*nick;
{
	int	new_server, server;

	new_server = atoi(data);
	if ((nick = check_nickname(nick)) != NULL)
	{
		server = from_server;
		from_server = new_server;
		send_to_server("NICK %s", nick);
		if (new_server == primary_server)
			strmcpy(nickname, nick, NICKNAME_LEN);
		set_server_nickname(new_server, nick);
		from_server = server;
		already_doing_reset_nickname = 0;
		update_all_status();
	}
	else
	{
		say("illegal nickname, try again");
		if (!dumb)
			add_wait_prompt("Nickname: ", nickname_sendline, data,
					WAIT_PROMPT_LINE);
	}
}

/*
 * reset_nickname: when the server reports that the selected nickname is not
 * a good one, it gets reset here. 
 */
void
reset_nickname()
{
	char	server_num[10];

	if (already_doing_reset_nickname)
		return;
	say("You have specified an illegal nickname");
	if (!dumb)
	{
		already_doing_reset_nickname = 1;
		say("Please enter your nickname");
		sprintf(server_num, "%d", from_server);
		add_wait_prompt("Nickname: ", nickname_sendline, server_num,
			WAIT_PROMPT_LINE);
	}
	update_all_status();
}

/*ARGSUSED*/
static	void
channel_topic(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*topic, *channel;

	if (ArgList[1] && is_channel(ArgList[0]))
	{
		channel = ArgList[0];
		if (is_cignored(channel, CIGNORE_CRAP))
			return;
		topic = ArgList[1];
		message_from(channel, LOG_CRAP);
		put_it("%sTopic for %s: %s", numeric_banner(), channel,
			topic);
	}
	else
	{
		PasteArgs(ArgList, 0);
		message_from((char *) 0, LOG_CURRENT);
		if (num_format_exists(-current_numeric))
			display_msg(from, ArgList);
		else
			put_it("%sTopic: %s", numeric_banner(), ArgList[0]);
	}
}

void
nickname_in_use(from, ArgList)
	char	*from,
		**ArgList;
{
	PasteArgs(ArgList, 0);
	if (is_server_connected(from_server))
		if (do_hook(current_numeric, "%s", *ArgList))
			display_msg(from, ArgList);
	else if (never_connected || from_server != primary_server ||
	    !attempting_to_connect)
	{
		if (do_hook(current_numeric, "%s", *ArgList))
			display_msg(from, ArgList);
		reset_nickname();
	}
	else
	{
		send_to_server("USER %s %s . :%s", username,
			(send_umode && *send_umode) ? send_umode : ".",
			realname);
		send_to_server("NICK %s", get_server_nickname(from_server));
	}
}

static	void
not_valid_channel(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*channel;
	char	*s;

	if (!(channel = ArgList[0]) || !ArgList[1])
		return;
	PasteArgs(ArgList, 1);
	s = get_server_name(from_server);
	if (0 == my_strnicmp(s, from, strlen(s)))
	{
		remove_channel(channel, from_server);
		if (num_format_exists(-current_numeric))
			display_msg(from, ArgList);
		else
			put_it("%s%s %s", numeric_banner(), channel,
				ArgList[1]);
	}
}

/* from ircd .../include/numeric.h */
/*
#define ERR_UNAVAILRESOURCE  437
#define ERR_CHANNELISFULL    471
#define ERR_INVITEONLYCHAN   473
#define ERR_BANNEDFROMCHAN   474
#define ERR_BADCHANNELKEY    475
#define ERR_BADCHANMASK      476
*/
static	void
cannot_join_channel(from, ArgList)
	char	*from,	
		**ArgList;
{
	char	*channel = ArgList[0];
	int	rj = 0;

	if (channel && is_channel(channel))
	{
		remove_channel(channel, from_server);
		switch(-current_numeric)
		{
		case 437:
			rj = rejoin_failed(channel, from_server,
				REJOIN_UNAVAILABLE_VAR);
			break;
		case 471:
			rj = rejoin_failed(channel, from_server,
				REJOIN_FULL_VAR);
			break;
		case 473:
			rj = rejoin_failed(channel, from_server,
				REJOIN_INVITEONLY_VAR);
			break;
		case 474:
			rj = rejoin_failed(channel, from_server,
				REJOIN_BANNED_VAR);
			break;
		case 475:
			rj = rejoin_failed(channel, from_server,
				REJOIN_BADKEY_VAR);
			break;
		case 476:
			/* Huh? */
			break;
		}
		if (rj)
		{
			return;
		}
	}
	PasteArgs(ArgList, 0);
	if (num_format_exists(-current_numeric)) {
		display_msg(from, ArgList);
		return;
	}
	strcpy(buffer, ArgList[0]);
	switch(-current_numeric)
	{
	case 437:
		strcat(buffer, " (Unavailable)");
		break;
	case 471:
		strcat(buffer, " (Channel is full)");
		break;
	case 473:
		strcat(buffer, " (Invite only channel)");
		break;
	case 474:
		strcat(buffer, " (Banned from channel)");
		break;
	case 475:
		strcat(buffer, " (Bad channel key)");
		break;
	case 476:
		strcat(buffer, " (Bad channel mask)");
		break;
	}
	put_it("%s%s", numeric_banner(), buffer);
}


/*ARGSUSED*/
static	void
version(from, ArgList)
	char	*from,
		**ArgList;
{
	if (ArgList[2])
	{
		PasteArgs(ArgList, 2);
		if (num_format_exists(-current_numeric))
			display_msg(from, ArgList);
		else
			put_it("%sServer %s: %s %s", numeric_banner(),
				ArgList[1], ArgList[0], ArgList[2]);
	}
	else
	{
		PasteArgs(ArgList, 1);
		if (num_format_exists(-current_numeric))
			display_msg(from, ArgList);
		else
			put_it("%sServer %s: %s", numeric_banner(), ArgList[1],
				ArgList[0]);
	}
}


/*ARGSUSED*/
static	void
invite(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*who,
		*channel;

	if ((who = ArgList[0]) && (channel = ArgList[1]))
	{
		message_from(channel, LOG_CRAP);
		if (do_hook(current_numeric, "%s %s %s", from, who, channel))
			if (num_format_exists(-current_numeric))
				display_msg(from, ArgList);
			else
				put_it("%sInviting %s to channel %s",
					numeric_banner(), who, channel);
	}
}


/*
 * numbered_command: does (hopefully) the right thing with the numbered
 * responses from the server.  I wasn't real careful to be sure I got them
 * all, but the default case should handle any I missed (sorry) 
 */
void
numbered_command(from, comm, ArgList)
	char	*from;
	int	comm;
	char	**ArgList;
{
	char	*user;
	char	none_of_these = 0;
	char	blah[BIG_BUFFER_SIZE];
	int	flag,
		lastlog_level;
#if 0
	int	user_cnt,
		inv_cnt,
		server_cnt;
#endif

	if (!ArgList[0])
		user = (char *) 0;
	else
		user = ArgList[0];
	if (!ArgList[1])
		return;
	lastlog_level = set_lastlog_msg_level(LOG_CRAP);
	message_from((char *) 0, LOG_CRAP);
	ArgList++;
	current_numeric = -comm;	/* must be negative of numeric! */
	switch (comm)
	{
	case 001:	/* #define RPL_WELCOME          001 */
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList)) 
			display_msg(from, ArgList);
		break;
	case 002:	/* #define RPL_YOURHOST         002 */
		PasteArgs(ArgList, 0);
		sprintf(blah, "*** %s", ArgList[0]);
		got_initial_version(blah);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;

/* should do something with this some day, 2.8 had channel/user mode switches */
	case 004:	/* #define RPL_MYINFO           004 */
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;

	case 005:	/* #define RPL_ISUPPORT         005 */
		/* Undernet only? */
		got_isupport(ArgList);
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		break;

/*
 * this part of ircii has been broken for most of ircd 2.7, so someday I'll
 * make it work for ircd 2.8 ...  phone..
 */
#if 0
	case 251:		/* #define RPL_LUSERCLIENT      251 */
		display_msg(from, ArgList);
		if (server_list[from_server].connected)
			break;
		if ((from_server == primary_server) && ((sscanf(ArgList[1],
		    "There are %d users and %d invisible on %d servers",
		    &user_cnt, &inv_cnt, &server_cnt) == 3) ||
		    (sscanf(ArgList[1], "There are %d users and %d invisible on %d servers",
		    &user_cnt, &inv_cnt, &server_cnt) == 3)))
		{
			user_cnt =+ inv_cnt;
			if ((server_cnt < get_int_var(MINIMUM_SERVERS_VAR)) ||
			    (user_cnt < get_int_var(MINIMUM_USERS_VAR)))
			{
				say("Trying better populated server...");
				get_connected(from_server + 1);
			}
		}
#endif
	case 251:		/* #define RPL_LUSERCLIENT	251 */
		{
		  int user_cnt, inv_cnt, server_cnt;
		  if (num_format_exists(251)) {
			display_msg(from, ArgList);
			break;
		  }
		  if ((sscanf(ArgList[0], "There are %d users and %d invisible on %d servers",
		    &user_cnt, &inv_cnt, &server_cnt) == 3) ||
		    (sscanf(ArgList[0], "There are %d users plus %d invisible on %d servers",
		    &user_cnt, &inv_cnt, &server_cnt) == 3))
		  {
			lusers_user_cnt = user_cnt + inv_cnt;
			/* Can't use %f in put_it() on some systems */
			sprintf(tempstr, "%sThere %s %u user%s (%u inv) on %u server%s (%3.3f clients/serv)",
			  numeric_banner(), lusers_user_cnt == 1 ? "is" : "are",
			  lusers_user_cnt, lusers_user_cnt == 1 ? "" : "s",
			  inv_cnt,
			  server_cnt, server_cnt == 1 ? "" : "s",
			  server_cnt ? 1.0*lusers_user_cnt/server_cnt : 0.0);
			smart_display(from, tempstr);
		  } else {
			lusers_user_cnt = -1;
			display_msg(from, ArgList);
		  }
		}
		break;
	case 252:
		{
		  int oper_cnt;
		  if (num_format_exists(252)) {
			display_msg(from, ArgList);
			break;
		  }
		  oper_cnt = atoi(ArgList[0]);
		  if (oper_cnt) {
			/* Can't use %f in put_it() on some systems */
			sprintf(tempstr, "%s%u IRC Operator%s (%3.3f clients/oper)",
			  numeric_banner(), oper_cnt, oper_cnt == 1 ? "" : "s",
			  1.0*(lusers_user_cnt-oper_cnt)/oper_cnt);
			smart_display(from, tempstr);
		  } else {
			sprintf(tempstr, "%sNo IRC Operators! D'oh!",
			  numeric_banner());
			smart_display(from, tempstr);
		  }
		}
		break;
	case 254:
		{
		  int chan_cnt;
		  if (num_format_exists(254)) {
			display_msg(from, ArgList);
			break;
		  }
		  chan_cnt = atoi(ArgList[0]);
		  if (chan_cnt) {
			/* Can't use %f in put_it() on some systems */
			sprintf(tempstr, "%s%u channel%s formed (%3.3f avg users/channel)",
			  numeric_banner(), chan_cnt, chan_cnt == 1 ? "" : "s",
			  1.0*lusers_user_cnt/chan_cnt);
			smart_display(from, tempstr);
		  } else {
			sprintf(tempstr, "%sNo channels formed!",
			  numeric_banner());
			smart_display(from, tempstr);
		  }
		}
		break;
	case 255:
		{
		  int my_clients, my_servers;
		  if (num_format_exists(255)) {
			display_msg(from, ArgList);
			break;
		  }
		  if ((sscanf(ArgList[0],
		    "I have %d clients and %d servers",
		    &my_clients, &my_servers) == 2) &&
		    (lusers_user_cnt > 0))
		  {
			sprintf(tempstr, "%sI have %u client%s and %u server%s (%3.3f%% of total)",
			  numeric_banner(), my_clients,
			  my_clients == 1 ? "" : "s", my_servers,
			  my_servers == 1 ? "" : "s",
			  100.0*my_clients/lusers_user_cnt);
			smart_display(from, tempstr);
		  } else
			display_msg(from, ArgList);
		}
		break;
	case 301:		/* #define RPL_AWAY             301 */
		user_is_away(from, ArgList);
		break;

	case 302:		/* #define RPL_USERHOST         302 */
		userhost_returned(from, ArgList);
		break;

	case 303:		/* #define RPL_ISON             303 */
		ison_returned(from, ArgList);
		break;

	case 311:		/* #define RPL_WHOISUSER        311 */
		whois_name(from, ArgList);
		break;

	case 312:		/* #define RPL_WHOISSERVER      312 */
		whois_server(from, ArgList);
		break;

	case 308:
		whois_admin(from, ArgList);
		break;

	case 313:		/* #define RPL_WHOISOPERATOR    313 */
		whois_oper(from, ArgList);
		break;

	case 314:		/* #define RPL_WHOWASUSER       314 */
		whowas_name(from, ArgList);
		break;

	case 316:		/* #define RPL_WHOISCHANOP      316 */
		whois_chop(from, ArgList);
		break;

	case 317:		/* #define RPL_WHOISIDLE        317 */
		whois_lastcom(from, ArgList);
		break;

	case 318:		/* #define RPL_ENDOFWHOIS       318 */
		end_of_whois(from, ArgList);
		break;

	case 319:		/* #define RPL_WHOISCHANNELS    319 */
		whois_channels(from, ArgList);
		break;

	case 321:		/* #define RPL_LISTSTART        321 */
		ArgList[0] = "Channel\0Users\0Topic";
		ArgList[1] = ArgList[0] + 8;
		ArgList[2] = ArgList[1] + 6;
		ArgList[3] = (char *) 0;
		funny_list(from, ArgList);
		break;

	case 322:		/* #define RPL_LIST             322 */
		funny_list(from, ArgList);
		break;

	case 324:		/* #define RPL_CHANNELMODEIS    324 */
		funny_mode(from, ArgList);
		break;

	case 329:		/* #define RPL_CHANNELCREATED	329 */
		channel_created(from, ArgList);
		break;

	case 341:		/* #define RPL_INVITING         341 */
		invite(from, ArgList);
		break;

	case 352:		/* #define RPL_WHOREPLY         352 */
		whoreply((char *) 0, ArgList);
		break;

	case 353:		/* #define RPL_NAMREPLY         353 */
		funny_namreply(from, ArgList);
		break;

	case 366:		/* #define RPL_ENDOFNAMES       366 */
		{
			int	flag, cflag;

			PasteArgs(ArgList, 0);
			if (!(cflag = is_cignored(from, CIGNORE_CRAP)))
			   flag = do_hook(current_numeric, "%s %s", from,
				ArgList[0]);
		
			if (!funny_is_ignore_channel())
			{
				if (!cflag && get_int_var(SHOW_END_OF_MSGS_VAR) && flag)
					display_msg(from, ArgList);
			}
			else
				funny_set_ignore_mode();
		}
		break;

	case 381: 		/* #define RPL_YOUREOPER        381 */
		PasteArgs(ArgList, 0);
		if (do_hook(current_numeric, "%s %s", from, *ArgList)) {
			Window  *old_to_window = to_window;
			to_window = get_window_by_name(get_string_var(SERVER_NOTICE_WINDOW_VAR));
			display_msg(from, ArgList);
			to_window = old_to_window;
		}
		set_server_operator(from_server, 1);
		set_server_flag(from_server, USER_MODE_S, 1);
		set_server_flag(from_server, USER_MODE_W, 1);
		update_all_status();	/* fix the status line */
		break;

	case 401:		/* #define ERR_NOSUCHNICK       401 */
		no_such_nickname(from, ArgList);
		break;

	case 421:		/* #define ERR_UNKNOWNCOMMAND   421 */
		if (check_screen_redirect(ArgList[0]))
			break;
		if (check_wait_command(ArgList[0]))
			break;
		PasteArgs(ArgList, 0);
		flag = do_hook(current_numeric, "%s %s", from, *ArgList);
		if (!strncmp("ISON", *ArgList, 4) || !strncmp("USERHOST",
		    *ArgList, 8))
		{
			set_server_2_6_2(from_server, 0);
			convert_to_whois(from_server);
		}
		else if (flag)
			display_msg(from, ArgList);
		break;

	case 432:		/* #define ERR_ERRONEUSNICKNAME 432 */
		if (do_hook(current_numeric, "%s %s", from, *ArgList))
			display_msg(from, ArgList);
		reset_nickname();
		break;

	case 433:		/* #define ERR_NICKNAMEINUSE    433 */ 
		nickname_in_use(from, ArgList);
		reset_nickname();
		break;

	case 463:		/* #define ERR_NOPERMFORHOST    463 */
		display_msg(from, ArgList);
		close_server(from_server, empty_string);
		window_check_servers();
		if (from_server == primary_server)
			get_connected(from_server + 1);
		break;

	case 464:		/* #define ERR_PASSWDMISMATCH   464 */
		PasteArgs(ArgList, 0);
		flag = do_hook(current_numeric, "%s %s", from, ArgList[0]);
		if (oper_command)
			if (flag)
				display_msg(from, ArgList);
		else
			get_password();
		break;

	case 465:		/* #define ERR_YOUREBANNEDCREEP 465 */
		{
			int	klined_server = from_server;

			PasteArgs(ArgList, 0);
			if (do_hook(current_numeric, "%s %s", from, ArgList[0]))
				display_msg(from, ArgList);
			close_server(from_server, empty_string);
			/*if (server_list_size() > 1)
				remove_from_server_list(klined_server);*/
			if (get_int_var(LOOP_RECONNECT_VAR))
				get_connected(from_server);
			window_check_servers();
			break;
		}

		/*
		 * The following accumulates the remaining arguments
		 * in ArgSpace for hook detection. We can't use
		 * PasteArgs here because we still need the arguments
		 * separated for use elsewhere.
		 */
	default:
		{
			char	*ArgSpace = (char *) 0;
			int	i, len, do_message_from;

			if ((comm == 305) && (get_int_var(AWAY_RECORDER_VAR) || (have_msgs() > 0)))
				tell_msg_count();
			if ((comm == 306) && (get_int_var(AWAY_RECORDER_VAR)))
				say("Recording messages while you are away.");
			if (comm == 315)
				if (toast_action == TACTION_CSTAT)
				{
					handle_cstat((char *)0, 0);
					return;
				} else if (com_action == 2)
					handle_masskill((char *)0);
			if (comm == 351)
				if (!handle_sping(from))
					return;
			if (comm == 402)
				handle_sping_nosuchserver(ArgList[0]);
 			if (comm == 367) {	/* BANLIST */
			  if (com_action == 1)
 			  {
 				com_unban(ArgList[0], ArgList[1]);
 				return;
 			  }
			  if (toast_action==TACTION_BANCOUNT)
			  {
				bc_count++;
				return;
			  }
			}
			if ((comm == 368) && (toast_action==TACTION_BANCOUNT))
			{
				toast_action = TACTION_NONE;
				say("%s has a bancount of %u", bc_chname, 
					bc_count);
				return;
			}
			if (toast_action==TACTION_FINDU)
			{
				if ((comm == 203) || (comm == 209))
					return;
 				if (comm == 206)
 					return;
 				if ((comm == 204) || (comm == 205))
 				{
 					handle_findu(atoi(ArgList[1]), ArgList[2]);
 					return;
 				}
 				if (comm == 262) /* End of TRACE */
 				{
 					handle_findu(-1, NULL);
 					return;
 				}
			}
			if (toast_action==TACTION_FINDD)
			{
				if (comm == 225)
				{
					handle_findd(ArgList);
					return;
				}
				if (comm == 219) /* End of STATS D */
				{
					handle_findd(NULL);
					return;
				}
			}
			if (toast_action==TACTION_FINDK)
			{
				if (comm == 216)
				{
					handle_findk(ArgList);
					return;
				}
				if (comm == 219) /* End of STATS K */
				{
					handle_findk(NULL);
					return;
				}
			}
 			if ((com_action==3)||(com_action==4))
 			{
				/* We don't want to see 203 and 209 */
				if ((comm == 203) || (comm == 209))
					return;
 				if ((comm == 204) || (comm == 206))
 					return;
 				if (comm == 205)
 				{
 					handle_tracekill(ArgList[2]);
 					return;
 				}
 				/*if (comm == 209)*/
				/* 262 is End of TRACE */
 				if (comm == 262)
 				{
 					handle_masskill(NULL);
 					return;
 				}
 			}

			for (i = len = 0; ArgList[i]; len += strlen(ArgList[i++]))
				;
			len += (i - 1);
			ArgSpace = new_malloc(len + 1);
			ArgSpace[0] = '\0';
			/* this is cheating */
			if (ArgList[0] && is_channel(ArgList[0]))
				do_message_from = 1;
			for (i = 0; ArgList[i]; i++)
			{
				if (i)
					strcat(ArgSpace, " ");
				strcat(ArgSpace, ArgList[i]);
			}
			if (do_message_from)
				message_from(ArgList[0], LOG_CRAP);
			if (!do_hook(current_numeric, "%s %s", from, ArgSpace))
			{
				new_free(&ArgSpace);
				if (do_message_from)
					message_from((char *) 0, lastlog_level);
				return;
			}
			if (do_message_from)
				message_from((char *) 0, lastlog_level);
			new_free(&ArgSpace);
			none_of_these = 1;
		}
	}
	/* the following do not hurt the ircII if intercepted by a hook */
	if (none_of_these)
	{
		switch (comm)
		{
		case 221: 		/* #define RPL_UMODEIS          221 */
			if (num_format_exists(221))
				display_msg(from, ArgList);
			else
				put_it("%sYour user mode is \"%s\"",
					numeric_banner(), ArgList[0]);
			break;

		case 242:		/* #define RPL_STATSUPTIME      242 */
			PasteArgs(ArgList, 0);
			if (num_format_exists(242))
				display_msg(from, ArgList);
			else {
			 if (from && !my_strnicmp(get_server_itsname(from_server),
			    from, strlen(get_server_itsname(from_server))))
				from = NULL;
			 if (from)
				put_it("%s%s from (%s)", numeric_banner(),
					*ArgList, from);
			 else
				put_it("%s%s", numeric_banner(), *ArgList);
			}
			break;

		case 332:		/* #define RPL_TOPIC            332 */
			channel_topic(from, ArgList);
			break;

		case 333:
			channel_topictime(from, ArgList);
			break;

		case 351:		/* #define RPL_VERSION          351 */
			version(from, ArgList);
			break;

		case 364:		/* #define RPL_LINKS            364 */
			if (num_format_exists(364))
				display_msg(from, ArgList);
			else {
				if (ArgList[2])
				{
					PasteArgs(ArgList, 2);
					put_it("%s%-20s %-20s %s",
						numeric_banner(), ArgList[0],
						ArgList[1], ArgList[2]);
				}
				else
				{
					PasteArgs(ArgList, 1);
					put_it("%s%-20s %s", numeric_banner(),
						ArgList[0], ArgList[1]);
				}
			}
			break;

		case 367:		/* #define RPL_BANLIST		367 */
			if (num_format_exists(367))
				display_msg(from, ArgList);
			else
				put_it("%s%s %s (%s %s)", numeric_banner(),
					ArgList[0], ArgList[1], ArgList[2],
					MakeTimeStamp(atoi(ArgList[3])));
			break;

		case 372:		/* #define RPL_MOTD             372 */
			if (!get_int_var(SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd(from_server))
			{
				PasteArgs(ArgList, 0);
				if (num_format_exists(372))
					display_msg(from, ArgList);
				else
					put_it("%s%s", numeric_banner(),
						ArgList[0]);
			}
			break;

		case 375:		/* #define RPL_MOTDSTART        375 */
			if (!get_int_var(SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd(from_server))
			{
				PasteArgs(ArgList, 0);
				if (num_format_exists(375))
					display_msg(from, ArgList);
				else
					put_it("%s%s", numeric_banner(),
						ArgList[0]);
			}
			break;

		case 376:		/* #define RPL_ENDOFMOTD        376 */
			if (get_int_var(SHOW_END_OF_MSGS_VAR) &&
			    (!get_int_var(SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd(from_server)))
			{
				PasteArgs(ArgList, 0);
				if (num_format_exists(376))
					display_msg(from, ArgList);
				else
					put_it("%s%s", numeric_banner(),
						ArgList[0]);
			}
			set_server_motd(from_server, 0);
			break;

		case 384:		/* #define RPL_MYPORTIS         384 */
			PasteArgs(ArgList, 0);
			if (num_format_exists(384))
				display_msg(from, ArgList);
			else
				put_it("%s%s %s", numeric_banner(),
					ArgList[0], user);
			break;

		case 385:		/* #define RPL_NOTOPERANYMORE   385 */
			set_server_operator(from_server, 0);
			display_msg(from, ArgList);
			update_all_status();
			break;

		case 403:		/* #define ERR_NOSUCHCHANNEL    403 */
			not_valid_channel(from, ArgList);
			break;

		case 432:		/* #define ERR_ERRONEUSNICKNAME 432 */
			display_msg(from, ArgList);
			reset_nickname();
			break;

		case 451:		/* #define ERR_NOTREGISTERED    451 */
	/*
	 * Sometimes the server doesn't catch the USER line, so
	 * here we send a simplified version again  -lynx 
	 */
			send_to_server("USER %s %s . :%s", username,
				(send_umode && *send_umode) ? send_umode : ".",
				realname);
			send_to_server("NICK %s",
				get_server_nickname(from_server));
			break;

		case 462:		/* #define ERR_ALREADYREGISTRED 462 */
			send_to_server("NICK %s",
				get_server_nickname(from_server));
			break;

		case 437:		/* #define ERR_UNAVAILRESOURCE  437 */
		case 471:		/* #define ERR_CHANNELISFULL    471 */
		case 473:		/* #define ERR_INVITEONLYCHAN   473 */
		case 474:		/* #define ERR_BANNEDFROMCHAN   474 */
		case 475: 		/* #define ERR_BADCHANNELKEY    475 */
		case 476:		/* #define ERR_BADCHANMASK      476 */
			cannot_join_channel(from, ArgList);
			break;

#define RPL_CLOSEEND         363
#define RPL_SERVLISTEND      235
		case 315:		/* #define RPL_ENDOFWHO         315 */
			if (cannot_open != (char *) 0)
				yell("Cannot open: %s", cannot_open);
			if (comm == 315)
			{
				remove_autowho(ArgList[0]);
			}
		case 323:               /* #define RPL_LISTEND          323 */
			funny_print_widelist();

		case 219:		/* #define RPL_ENDOFSTATS       219 */
		case 232:		/* #define RPL_ENDOFSERVICES    232 */
		case 365:		/* #define RPL_ENDOFLINKS       365 */
		case 368:		/* #define RPL_ENDOFBANLIST     368 */
			if ((comm == 368) && (com_action == 1))
			{
				com_action = 0;
				com_unban(ArgList[0], (char *) 0);
				break;
			}
		case 369:		/* #define RPL_ENDOFWHOWAS      369 */
		case 374:		/* #define RPL_ENDOFINFO        374 */
#if 0	/* this case needs special handing - see above */
		case 376:		/* #define RPL_ENDOFMOTD        376 */
#endif
		case 394:		/* #define RPL_ENDOFUSERS       394 */
			if (!get_int_var(SHOW_END_OF_MSGS_VAR))
				break;
		default:
			display_msg(from, ArgList);
		}
	}
	set_lastlog_msg_level(lastlog_level);
}
