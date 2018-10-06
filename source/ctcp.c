/*
 * ctcp.c:handles the client-to-client protocol(ctcp). 
 *
 * Written By Timothy Jensen
 *
 * Copyright (c) 1999-2001 
 *
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: ctcp.c,v 1.7 2001/04/01 15:58:26 toast Exp $";
#endif

#include "irc.h"
#include "flood.h"

#include <pwd.h>

#ifdef HAVE_UNAME
# include <sys/utsname.h>
#endif

#include "ircaux.h"
#include "hook.h"
#include "crypt.h"
#include "ctcp.h"
#include "vars.h"
#include "server.h"
#include "status.h"
#include "lastlog.h"
#include "ignore.h"
#include "output.h"
#include "window.h"
#include "dcc.h"
#include "names.h"
#include "parse.h"
#include "toast.h"
#include "format.h"
#include "cignore.h"

#define	CTCP_SHUTUP	0
#define	CTCP_VERBOSE	1
#define	CTCP_NOREPLY	2

static	char	CTCP_Reply_Buffer[BIG_BUFFER_SIZE + 1] = "";

static	void	do_new_notice_ctcp();

extern	void	strmcat();

/* forward declarations for the built in CTCP functions */
static	char	*do_sed();
static	char	*do_version();
static	char	*do_clientinfo();
static	char	*do_echo();
static	char	*do_userinfo();
static	char	*do_finger();
static	char	*do_time();
static	char	*do_atmosphere();
static	char	*do_dcc();
extern	char	*do_utc();

static CtcpEntry ctcp_cmd[] =
{
	{ "SED",	"contains simple_encrypted_data",
		CTCP_SHUTUP | CTCP_NOREPLY, do_sed },
	{ "VERSION",	"shows client type, version and environment",
		CTCP_VERBOSE, do_version },
	{ "CLIENTINFO",	"gives information about available CTCP commands",
		CTCP_VERBOSE, do_clientinfo },
	{ "USERINFO",	"returns user settable information",
		CTCP_VERBOSE, do_userinfo },
	{ "ERRMSG",	"returns error messages",
		CTCP_VERBOSE, do_echo },
	{ "FINGER",	"shows real name, login name and idle time of user",
		CTCP_VERBOSE, do_finger },
	{ "TIME",	"tells you the time on the user's host",
		CTCP_VERBOSE, do_time },
	{ "ACTION",	"contains action descriptions for atmosphere",
		CTCP_SHUTUP, do_atmosphere },
#ifndef DISABLE_DCC
	{ "DCC",	"requests a direct_client_connection",
		CTCP_SHUTUP | CTCP_NOREPLY, do_dcc },
#endif
	{ "UTC",	"substitutes the local timezone",
		CTCP_SHUTUP | CTCP_NOREPLY, do_utc },
	{ "PING", 	"returns the arguments it receives",
		CTCP_VERBOSE, do_echo },
	{ "ECHO", 	"returns the arguments it receives",
		CTCP_VERBOSE, do_echo }
};

char	*ctcp_type[] =
{
	"PRIVMSG",
	"NOTICE"
};

static	char	ctcp_buffer[BIG_BUFFER_SIZE + 1];

/* This is set to one if we parsed an SED */
int     sed = 0;

/*
 * in_ctcp_flag is set to true when IRCII is handling a CTCP request.  This
 * is used by the ctcp() sending function to force NOTICEs to be used in any
 * CTCP REPLY 
 */
int	in_ctcp_flag = 0;

/*
 * ctcp_last_reply_time: Used to stop flooding...  We only send one ctcp
 * reply a second.. if the variable NOCTCP_FLOOD is set...  If the current
 * second is still the same, we drop the request, and info the user.
 */

/* Why not use check_flooding() ? -- Comstud
time_t	ctcp_last_reply_time = 0;

int	ctcp_dropped;
int	not_warned = 0;
*/

/*
 * quote_it: This quotes the given string making it sendable via irc.  A
 * pointer to the length of the data is required and the data need not be
 * null terminated (it can contain nulls).  Returned is a malloced, null
 * terminated string.   
 */
char	*
ctcp_quote_it(str, len)
	char	*str;
	int	len;
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	*ptr;
	int	i;

	ptr = buffer;
	for (i = 0; i < len; i++)
	{
		switch (str[i])
		{
		case CTCP_DELIM_CHAR:
			*(ptr++) = CTCP_QUOTE_CHAR;
			*(ptr++) = 'a';
			break;
		case '\n':
			*(ptr++) = CTCP_QUOTE_CHAR;
			*(ptr++) = 'n';
			break;
		case '\r':
			*(ptr++) = CTCP_QUOTE_CHAR;
			*(ptr++) = 'r';
			break;
		case CTCP_QUOTE_CHAR:
			*(ptr++) = CTCP_QUOTE_CHAR;
			*(ptr++) = CTCP_QUOTE_CHAR;
			break;
		case '\0':
			*(ptr++) = CTCP_QUOTE_CHAR;
			*(ptr++) = '0';
			break;
		default:
			*(ptr++) = str[i];
			break;
		}
	}
	*ptr = '\0';
	str = (char *) 0;
	malloc_strcpy(&str, buffer);
	return (str);
}

/*
 * ctcp_unquote_it: This takes a null terminated string that had previously
 * been quoted using ctcp_quote_it and unquotes it.  Returned is a malloced
 * space pointing to the unquoted string.  NOTE: a trailing null is added for
 * convenied, but the returned data may contain nulls!.  The len is modified
 * to contain the size of the data returned. 
 */
char	*
ctcp_unquote_it(str, len)
	char	*str;
	int	*len;
{
	char	*buffer;
	char	*ptr;
	char	c;
	int	i,
		new_size = 0;

	buffer = (char *) new_malloc(sizeof(char) * *len);
	ptr = buffer;
	i = 0;
	while (i < *len)
	{
		if ((c = str[i++]) == CTCP_QUOTE_CHAR)
		{
			switch (c = str[i++])
			{
			case CTCP_QUOTE_CHAR:
				*(ptr++) = CTCP_QUOTE_CHAR;
				break;
			case 'a':
				*(ptr++) = CTCP_DELIM_CHAR;
				break;
			case 'n':
				*(ptr++) = '\n';
				break;
			case 'r':
				*(ptr++) = '\r';
				break;
			case '0':
				*(ptr++) = '\0';
				break;
			default:
				*(ptr++) = c;
				break;
			}
		}
		else
			*(ptr++) = c;
		new_size++;
	}
	*ptr = '\0';
	*len = new_size;
	return (buffer);
}

/*
 * do_sed: Performs the Simple Encrypted Data trasfer for ctcp.  Returns in a
 * malloc string the decryped message (if a key is set for that user) or the
 * text "[ENCRYPTED MESSAGE]" 
 */
static	char	*
do_sed(ctcp, from, to, args)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*args;
{
	char	*key,
		*crypt_who;
	char	*ret = NULL;

	if (my_stricmp(to, get_server_nickname(from_server)))
		crypt_who = to;
	else
		crypt_who = from;
	if ((key = is_crypted(crypt_who)) && !(ret = crypt_msg(args, key, 0)))
			malloc_strcpy(&ret, "[ENCRYPTED MESSAGE]");
                else
                        sed = 1;
	return (ret);
}

/*
 * do_clientinfo: performs the CLIENTINFO CTCP.  If cmd is empty, returns the
 * list of all CTCPs currently recognized by IRCII.  If an arg is supplied,
 * it returns specific information on that CTCP.  If a matching CTCP is not
 * found, an ERRMSG ctcp is returned 
 */
static	char	*
do_clientinfo(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
	int	i;

	if (cmd && *cmd)
	{
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
		{
			if (my_stricmp(cmd, ctcp_cmd[i].name) == 0)
			{
				send_ctcp_reply(from, ctcp->name, "%s %s",
					ctcp_cmd[i].name, ctcp_cmd[i].desc);
				return NULL;
			}
		}
		send_ctcp_reply(from, ctcp_cmd[CTCP_ERRMSG].name,
				"%s: %s is not a valid function",
				ctcp_cmd[CTCP_CLIENTINFO].name, cmd);
	}
	else
	{
		*buffer = '\0';
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
		{
			strmcat(buffer, ctcp_cmd[i].name, BIG_BUFFER_SIZE);
			strmcat(buffer, " ", BIG_BUFFER_SIZE);
		}
		send_ctcp_reply(from, ctcp->name, 
			"%s :Use CLIENTINFO <COMMAND> to get more specific \
information",
			buffer);
	}
	return NULL;
}

/* do_version: does the CTCP VERSION command */
static	char	*
do_version(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
	char	*tmp;

#ifdef HAVE_UNAME
	struct utsname un;
	char	*the_unix,
		*the_version;

	if (uname(&un) < 0)
	{
		the_version = empty_string;
		the_unix = "unknown";
	}
	else
	{
		the_version = un.release;
		the_unix = un.sysname;
	}
	send_ctcp_reply(from, ctcp->name, "Blackened %s %s %s :%s", irc_version, the_unix, the_version,
#else
	send_ctcp_reply(from, ctcp->name, "Blackened %s *IX :%s", irc_version,
#endif
		(tmp = get_string_var(CLIENTINFO_VAR)) ?  tmp : IRCII_COMMENT);
	return NULL;
}

/* do_time: does the CTCP TIME command --- done by Veggen */
static	char	*
do_time(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
	time_t	tm = time((time_t *) 0);
	char	*s, *t = ctime(&tm);

	if ((char *) 0 != (s = index(t, '\n')))
		*s = '\0';
	send_ctcp_reply(from, ctcp->name, "%s", t);
	return NULL;
}

/* do_userinfo: does the CTCP USERINFO command */
static	char	*
do_userinfo(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
	send_ctcp_reply(from, ctcp->name, get_string_var(USERINFO_VAR));
	return NULL;
}

/*
 * do_echo: does the CTCP ECHO, CTCP ERRMSG and CTCP PING commands. Does
 * not send an error for ERRMSG and if the CTCP was sent to a channel.
 */
static	char	*
do_echo(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,	
		*cmd;
{
	if (!is_channel(to) || strncmp(cmd, "ERRMSG", 6))
		send_ctcp_reply(from, ctcp->name, "%s", cmd);
	return NULL;
}

static	char	*
do_finger(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
	struct	passwd	*pwd;
	time_t	diff;
	int	uid;
	char	c;
	char	tempbuf[30];

	/*
	 * sojge complained that ircII says 'idle 1 seconds'
	 * well, now he won't ever get the chance to see that message again
	 *   *grin*  ;-)    -lynx
	 *
	 * Made this better by saying 'idle 1 second'  -phone
	 */

	diff = time(0) - idle_time;
	c = (diff == 1)? ' ': 's';
	uid = getuid();
#ifdef DAEMON_UID
	if (uid != DAEMON_UID)
	{
#endif /* DAEMON_UID */	
		if ((pwd = getpwuid(uid)) != NULL)
		{
			char	*tmp;

#ifdef GECOS_DELIMITER
			if ((tmp = index(pwd->pw_gecos, GECOS_DELIMITER)) != NULL)
#else
			if ((tmp = index(pwd->pw_gecos, ',')) != NULL)
#endif /* GECOS_DELIMITER */
				*tmp = '\0';
			sprintf(tempbuf, "Idle %ld second%c",
				diff, c);
			send_ctcp_reply(from, ctcp->name,
				"%s (%s@%s) %s", realname,
				username, get_int_var(USE_FAKE_HOST_VAR) ? get_string_var(FAKE_HOST_VAR) : hostname,
				get_int_var(SHOW_CTCP_IDLE_VAR) ? tempbuf : "");
		}
#ifdef DAEMON_UID
	}
	else
		send_ctcp_reply(from, ctcp->name,
			"IRCII Telnet User (%s) Idle %ld second%c",
			realname, diff, c);
#endif /* DAEMON_UID */	
	return NULL;
}

/*
 * do_ctcp: handles the client to client protocol embedded in PRIVMSGs.  Any
 * such messages are removed from the original str, so after do_ctcp()
 * returns, str will be changed
 */
char	*
do_ctcp(from, to, str)
	char	*from,
		*to,
		*str;
{
	int	i = 0,
		ctcp_flag = 1;
	char	*end,
		*cmd,
		*args,
		*ptr;
	char	*arg_copy = NULL;
	int	flag, cflag;
	int	messages = 0;
	char	temp[1024];

	flag = double_ignore(from, FromUserHost, IGNORE_CTCPS);
	cflag = is_cignored(to, CIGNORE_CTCPS);

	if (!in_ctcp_flag)
		in_ctcp_flag = 1;
	*ctcp_buffer = '\0';
/*
	ctcp_dropped = 0;
*/
	while ((cmd = index(str, CTCP_DELIM_CHAR)) != NULL)
	{
/*		if (messages > 3)
			break;
*/		*(cmd++) = '\0';
		strcat(ctcp_buffer, str);
		if ((end = index(cmd, CTCP_DELIM_CHAR)) != NULL)
		{
			if (messages >= 2)
				continue;
			messages++;
/*
			if (!ctcp_dropped && (time(NULL) - ctcp_last_reply_time) < 2 && get_int_var(NO_CTCP_FLOOD_VAR))
				ctcp_dropped = 1;
			if (!ctcp_dropped)
				not_warned = 1;
*/
			if ((flag == IGNORED) || cflag)
				continue;
			*(end++) = '\0';
			if ((args = index(cmd, ' ')) != NULL)
				*(args++) = '\0';
			else
				args = empty_string;
			malloc_strcpy(&arg_copy, args);
			for (i = 0; i < NUMBER_OF_CTCPS; i++)
			{
			    if (strcmp(cmd, ctcp_cmd[i].name) == 0)
			    {
/*				if (!ctcp_dropped || ctcp_cmd[i].flag & CTCP_NOREPLY)
*/				{
		/*
		 * This test here to stop irc operators seeing ctcp replies
		 * when using global messages - phone, dec, 1992.
		 */
					if (*to != '$' && !(*to == '#' && !lookup_channel(to, from_server, 0)) &&
						(ptr = ctcp_cmd[i].func(&(ctcp_cmd[i]), from, to, arg_copy)))
					{
						strcat(ctcp_buffer, ptr);
						new_free(&ptr);
					}
					ctcp_flag = ctcp_cmd[i].flag;
					cmd = ctcp_cmd[i].name;
					break;
				}
/*
			        else if (get_int_var(VERBOSE_CTCP_VAR) && not_warned)
				{
					say("CTCP flood from %s", from);
					not_warned = 0;
				}
*/
			    }
			}
			new_free(&arg_copy);
			strcpy(temp, cmd);
			if (args && *args)
			{
				strcat(temp, " ");
				strcat(temp, args);
			}
			if (/* !ctcp_dropped && */ in_ctcp_flag == 1 &&
                            check_flooding(from, CTCP_FLOOD, to, temp) &&
			    do_hook(CTCP_LIST, "%s %s %s %s", from, to, cmd,
			    args) && get_int_var(VERBOSE_CTCP_VAR))
			{
				int	lastlog_level;

				lastlog_level = set_lastlog_msg_level(LOG_CTCP);
				message_from(NULL, LOG_CTCP);
				if (i == NUMBER_OF_CTCPS)
				{
					/*say("[%s] Unknown CTCP %s from %s to %s: %s%s", TimeStamp(),
						cmd, from, to, *args ? ": " :
						empty_string, args);*/
					put_it("%s", parseformat(CTCP_UNKNOWN_FMT, from, to, cmd, args));
				}
				else if (ctcp_flag & CTCP_VERBOSE)
				{
					if (my_stricmp(to,
					    get_server_nickname(from_server)))
						/*say("[%s] CTCP %s from %s to %s: %s", TimeStamp(),
							cmd, from, to, args);*/
						put_it("%s", parseformat(CTCP_FMT, from, to, cmd, args));
					else
						/*say("[%s] CTCP %s from %s (%s) %s%s", TimeStamp(),
							cmd, from,
							FromUserHost ?
								FromUserHost :
								"unknown",
							*args ? ": " :
							empty_string, args);*/
						put_it("%s", parseformat(CTCP_PRIVATE_FMT, from, to, cmd, args));
				}
				set_lastlog_msg_level(lastlog_level);
			}
			str = end;
		}
		else
		{
			strcat(ctcp_buffer, CTCP_DELIM_STR);
			str = cmd;
		}
	}
	if (in_ctcp_flag == 1)
		in_ctcp_flag = 0;
/*
	if (CTCP_Reply_Buffer && *CTCP_Reply_Buffer)
		ctcp_last_reply_time = time(NULL);
*/
	strcat(ctcp_buffer, str);
	send_to_server("%s", CTCP_Reply_Buffer);
	*CTCP_Reply_Buffer = '\0';
	return (ctcp_buffer);
}

char	*
do_notice_ctcp(from, to, str)
	char	*from,
		*to,
		*str;
{
	char	*cmd;

	in_ctcp_flag = -1;
	*ctcp_buffer = '\0';
	/*
	 * The following used to say "While". It now says "if" because people
	 * Started using CTCP ERRMSG replies to CTCP bomb. The effect of this
	 * is that IRCII users can only send one CTCP/message if they expect a
	 * reply. This shouldn't be a problem as that is the way IRCII operates
	 *
	 * Changed this behavouir to follow NO_CTCP_FLOOD
	 */

	if (get_int_var(NO_CTCP_FLOOD_VAR))
	{
		if ((cmd = index(str, CTCP_DELIM_CHAR)) != NULL)
			do_new_notice_ctcp(from, to, &str, cmd);
	}
	else
		while ((cmd = index(str, CTCP_DELIM_CHAR)) != NULL)
			do_new_notice_ctcp(from, to, &str, cmd);
	in_ctcp_flag = 0;
	strcat(ctcp_buffer, str);
	return (ctcp_buffer);
}

static	void
do_new_notice_ctcp(from, to, str, cmd)
	char	*from,
		*to,
		**str,
		*cmd;
{
	char	*end,
		*args,
		*ptr,
		*arg_copy = NULL;
	int	flags,
		i,
		lastlog_level;

	flags = 0;
	*(cmd++) = '\0';
	strcat(ctcp_buffer, *str);
	if ((end = index(cmd, CTCP_DELIM_CHAR)) != NULL)
	{
		*(end++) = '\0';
		if ((args = index(cmd, ' ')) != NULL)
			*(args++) = '\0';
		malloc_strcpy(&arg_copy, args);
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
		{ if ((strcmp(cmd, ctcp_cmd[i].name) == 0) && ctcp_cmd[i].flag & CTCP_NOREPLY)
			{
				if ((ptr = ctcp_cmd[i].func(&(ctcp_cmd[i]), from, to, arg_copy)) != NULL)
				{
					strcat(ctcp_buffer, ptr);
					new_free(&ptr);
					flags = ctcp_cmd[i].flag;
				}
				break;
			}
		}
		new_free(&arg_copy);
		if (!args)
			args = empty_string;
		if (do_hook(CTCP_REPLY_LIST, "%s %s %s", from, cmd,
				args) && !(flags & CTCP_NOREPLY))
		{
			if (!strcmp(cmd, "PING"))
			{
				char	buf[64];
				time_t	timediff,
					currenttime,
					timediff2;
				char	*seconds;
				time_t	now, nowus;
				struct  timeval         tp;
				struct  timezone        tzp;
				long	sec, usec;


				currenttime = time(NULL);
				seconds = next_arg(args, &args);
				if (args && *args)
				{
					sec = atol(seconds);
					if (sec < 0) sec = 0;
					usec = atol(args);
					if (usec < 0) usec = 0;
					gettimeofday(&tp, &tzp);
					utimediff(tp.tv_sec, tp.tv_usec, sec, usec, &timediff, &timediff2);
					timediff2 /= 1000;
				}
				else
				if (seconds && *seconds) {
					sec = atol(seconds);
					if (sec < 0) sec = 0;
					timediff = currenttime -
						(time_t) sec;
				} else
					timediff = (time_t) 0;

				/* This prevents the client for bus errors */
				if (timediff < 0) timediff = 0;
				if (timediff2 < 0) timediff2 = 0;

				if (args && *args)
					sprintf(buf, "%d.%0.3d seconds",
					(int) timediff, (int) timediff2);
				else
				sprintf(buf, "%d second%s", timediff,
					(timediff == 1) ? "" : "s");
				args = buf;
			}
			lastlog_level = set_lastlog_msg_level(LOG_CTCP);
			message_from(NULL, LOG_CTCP);
			/*say("[%s] CTCP %s reply from %s: %s", TimeStamp(), cmd, from,
				args);*/
			put_it("%s", parseformat(CTCP_REPLY_FMT, from, to, cmd, args));
			set_lastlog_msg_level(lastlog_level);
		}
		*str = end;
	}
	else
	{
		strcat(ctcp_buffer, CTCP_DELIM_STR);
		*str = cmd;
	}
}

/* in_ctcp: simply returns the value of the ctcp flag */
int
in_ctcp()
{
	return (in_ctcp_flag);
}

/*
 * do_atmosphere: does the CTCP ACTION command --- done by lynX
 * Changed this to make the default look less offensive to people
 * who don't like it and added a /on ACTION. This is more in keeping
 * with the design philosophy behind IRCII
 */
static	char	*
do_atmosphere(ctcp, from, to, cmd)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*cmd;
{
        char    *timestr;
        timestr = (char *)TimeStamp();
	if (cmd && *cmd)
	{
		int old;

		old = set_lastlog_msg_level(LOG_ACTION);
		if (is_channel(to))
		{
			message_from(to, LOG_ACTION);
			if (do_hook(ACTION_LIST, "%s %s %s", from, to, cmd))
			{
				if (is_current_channel(to, 0)) {
					int rn = get_refnum_by_channel(to);
                                	if (!is_last_current_channel(rn, to)) {
                                  	  set_last_current_channel(rn, to);
                                  	  say("You are now talking to channel %s", to);
                                	}
					put_it("%s", parseformat(PUBLIC_ACTION_FMT, from, to, cmd));
				} else
					put_it("%s", parseformat(ACTION_OTHER_FMT, from, to, cmd));
			}
		}
		else
		{
			message_from(from, LOG_ACTION);
			if (do_hook(ACTION_LIST, "%s %s %s", from, to, cmd))
				put_it("%s", parseformat(ACTION_FMT, from, to, cmd));
		}
		message_from(NULL, LOG_CRAP);
		set_lastlog_msg_level(old);
	}
	return NULL;
}

/*
 * do_dcc: Records data on an incoming DCC offer. Makes sure it's a
 *	user->user CTCP, as channel DCCs don't make any sense whatsoever
 */
static	char	*
do_dcc(ctcp, from, to, args)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*args;
{
	char	*type;
	char	*description;
	char	*inetaddr;
	char	*port;
	char	*size;

	if (my_stricmp(to, get_server_nickname(from_server)))
		return NULL;
	if (!(type = next_arg(args, &args)) ||
			!(description = next_arg(args, &args)) ||
			!(inetaddr = next_arg(args, &args)) ||
			!(port = next_arg(args, &args)))
		return NULL;
	size = next_arg(args, &args);
	register_dcc_offer(from, type, description, inetaddr, port, size);
	return NULL;
}

char	*
do_utc(ctcp, from, to, args)
	CtcpEntry	*ctcp;
	char	*from,
		*to,
		*args;
{
	time_t	tm;
	char	*date = NULL;

	if (!args || !*args)
		return NULL;
	tm = atol(args);
	malloc_strcpy(&date, ctime(&tm));
	date[strlen(date)-1] = '\0';
	return date;
}

/* These moved here because they belong here - phone */

/*
 * send_ctcp_notice: A simply way to send CTCP replies.   I put this here
 * rather than in ctcp.c to keep my compiler quiet 
 */
void
#ifdef USE_STDARG_H
send_ctcp(char *type, char *to, char *datatag, char *format, ...)
{
	va_list vl;
#else
send_ctcp(type, to, datatag, format, arg0, arg1, arg2, arg3, arg4,
	arg5, arg6, arg7, arg8, arg9)
	char	*type,
		*to,
		*datatag,
		*format;
	char	*arg0,
		*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9;
{
#endif
	char putbuf[BIG_BUFFER_SIZE + 1];

	if (in_on_who)
		return;	/* Silently drop it on the floor */
	if (format)
	{
#ifdef USE_STDARG_H
		va_start(vl, format);
		vsprintf(putbuf, format, vl);
		va_end(vl);
#else
		sprintf(putbuf, format, arg0, arg1, arg2, arg3, arg4, arg5,
			arg6, arg7, arg8, arg9);
#endif
		send_to_server("%s %s :%c%s %s%c", type, to, CTCP_DELIM_CHAR,
			datatag, putbuf, CTCP_DELIM_CHAR);
	}
	else
		send_to_server("%s %s :%c%s%c", type, to, CTCP_DELIM_CHAR,
			datatag, CTCP_DELIM_CHAR);
}


/*
 * send_ctcp_notice: A simply way to send CTCP replies.   I put this here
 * rather than in ctcp.c to keep my compiler quiet 
 */
void
#ifdef USE_STDARG_H
send_ctcp_reply(char *to, char *datatag, char *format, ...)
{
	va_list vl;
#else
send_ctcp_reply(to, datatag, format, arg0, arg1, arg2, arg3, arg4,
		arg5, arg6, arg7, arg8, arg9)
	char	*to,
		*datatag,
		*format;
	char	*arg0,
		*arg1,
		*arg2,
		*arg3,
		*arg4,
		*arg5,
		*arg6,
		*arg7,
		*arg8,
		*arg9;
{
#endif
	char	putbuf[BIG_BUFFER_SIZE + 1];

	if (in_on_who)
		return;	/* Silently drop it on the floor */
	if (!*CTCP_Reply_Buffer)
		sprintf(CTCP_Reply_Buffer, "NOTICE %s :", to);
	strmcat(CTCP_Reply_Buffer, "\001", BIG_BUFFER_SIZE);
	strmcat(CTCP_Reply_Buffer, datatag, BIG_BUFFER_SIZE);
	strmcat(CTCP_Reply_Buffer, " ", BIG_BUFFER_SIZE);
	if (format)
	{
#ifdef USE_STDARG_H
		va_start(vl, format);
		vsprintf(putbuf, format, vl);
		va_end(vl);
#else
		sprintf(putbuf, format, arg0, arg1, arg2, arg3, arg4, arg5,
			arg6, arg7, arg8, arg9);
#endif
		strmcat(CTCP_Reply_Buffer, putbuf, BIG_BUFFER_SIZE);
	}
	else
		strmcat(CTCP_Reply_Buffer, putbuf, BIG_BUFFER_SIZE);
	strmcat(CTCP_Reply_Buffer, "\001", BIG_BUFFER_SIZE);
}
