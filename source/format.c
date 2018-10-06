/*
 * format.c: Output format configuration stuff
 *
 * Written By Timothy Jensen
 *
 * Copyright (c) 1999-2001 
 *
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: format.c,v 1.14.2.1 2002/03/07 22:29:52 toast Exp $";
#endif

#include "irc.h"

#include "format.h"
#include "ircaux.h"
#include "parse.h"
#include "ignore.h"
#include "comstud.h"
#include "toast.h"
#include "vars.h"

extern	int	from_server;
extern  char    *get_server_itsname();

int	hilite;

void	set_format(int, char *);
void	set_num_format(int, char *);
void	set_fmt(int, char *);

#define	VIF_CHANGED	0x01
#define	VIF_GLOBAL	0x02

static NumFormat num_format = {-1, NULL, 0, NULL};

static IrcFormat irc_format[] =
{
	{ "ACTION",			NULL, "FTM", 	0 },
	{ "ACTION_OTHER",		NULL, "FTM", 	0 },
	{ "CTCP",			NULL, "FTCM",	0 },
	{ "CTCP_PRIVATE",		NULL, "FTCM",	0 },
	{ "CTCP_REPLY",			NULL, "FTCM",	0 },
	{ "CTCP_UNKNOWN",		NULL, "FTCM",	0 },
	{ "DCC_CHAT",			NULL, "FM",	0 },
	{ "DCC_CHAT_ACTION",		NULL, "FM",	0 },
	{ "DCC_TALK",			NULL, "FM",	0 },
	{ "GONE",			NULL, "M",	0 },
	{ "INVITE",			NULL, "FTC",	0 },
	{ "JOIN",			NULL, "FC",	0 },
	{ "JOIN_SELF",			NULL, "FC",	0 },
	{ "KICK",			NULL, "FCTM",	0 },
	{ "KICK_SELF",			NULL, "FCTM",	0 },
	{ "KILL",			NULL, "FHTSM",	0 },
	{ "MODE",			NULL, "FCM",	0 },
	{ "MSG",			NULL, "FTM",	0 },
	{ "MSG_GROUP",			NULL, "FTM",	0 },
	{ "NICK",			NULL, "FN",	0 },
	{ "NOTICE",			NULL, "FTM",	0 },
	{ "NOTIFY_SIGNOFF",		NULL, "N",	0 },
	{ "NOTIFY_SIGNON",		NULL, "N",	0 },
	{ "NOTIFY_SIGNON_USERHOST",	NULL, "N",	0 },
	{ "OPS",			NULL, "FTM",	0 },
	{ "OPS_OTHER",			NULL, "FTM",	0 },
	{ "PART",			NULL, "FC",	0 },
	{ "PART_SELF",			NULL, "FC",	0 },
	{ "PUBLIC",			NULL, "FTM",	0 },
	{ "PUBLIC_ACTION",		NULL, "FTM",	0 },
	{ "PUBLIC_MSG",			NULL, "FTM",	0 },
	{ "PUBLIC_NOTICE",		NULL, "FTM",	0 },
	{ "PUBLIC_OTHER",		NULL, "FTM",	0 },
	{ "RECORDER_BODY",		NULL, "DFMN",	0 },
	{ "RECORDER_HEAD",		NULL, "DFMN",	0 },
	{ "SEND_ACTION",		NULL, "FTM",	0 },
	{ "SEND_DCC_CHAT",		NULL, "TM",	0 },
	{ "SEND_DCC_TALK",		NULL, "TM",	0 },
	{ "SEND_LOCOPS",		NULL, "FMO",	0 },
	{ "SEND_MSG",			NULL, "FTM",	0 },
	{ "SEND_NOTICE",		NULL, "FTM",	0 },
	{ "SEND_OPERWALL",		NULL, "FMO",	0 },
	{ "SEND_OPS",			NULL, "FTM",	0 },
	{ "SEND_OPS_OTHER",		NULL, "FTM",	0 },
	{ "SEND_PUBLIC",		NULL, "FTM",	0 },
	{ "SEND_PUBLIC_ACTION",		NULL, "FTM",	0 },
	{ "SEND_PUBLIC_MSG",		NULL, "FTM",	0 },
	{ "SEND_PUBLIC_OTHER",		NULL, "FTM",	0 },
	{ "SEND_WALLOPS",		NULL, "FMO",	0 },
	{ "SERVER_KILL",		NULL, "FTM",	0 },
	{ "SERVER_NOTICE",		NULL, "M",	0 },
	{ "SERVER_WALLOPS",		NULL, "FM",	0 },
	{ "TOPIC",			NULL, "FCM",	0 },
	{ "QUIT",			NULL, "FM",	0 },
	{ "UMODE",			NULL, "FTM",	0 },
	{ "WALL",			NULL, "FM",	0 },
	{ "WALLOPS",			NULL, "FMO",	0 },
	{ "WHOIS_ADMIN",		NULL, "NM",	0 },
	{ "WHOIS_AWAY",			NULL, "NM",	0 },
	{ "WHOIS_CHANNELS",		NULL, "NM",	0 },
	{ "WHOIS_IDLE",			NULL, "NSM",	0 },
	{ "WHOIS_IRCNAME",		NULL, "NM",	0 },
	{ "WHOIS_NSLOOKUP",		NULL, "NIH",	0 },
	{ "WHOIS_OPER",			NULL, "NM",	0 },
	{ "WHOIS_SERVER",		NULL, "NSM",	0 },
	{ "WHOIS_SIGNON",		NULL, "NSM",	0 },
	{ "WHOIS_USERHOST",		NULL, "NIH",	0 },
	{ "WHOWAS_USERHOST",		NULL, "NIH",	0 },
	{ (char *) NULL, (char *) NULL, (char *) NULL,	0 }
};

void
init_formats()
{
	set_fmt(ACTION_FMT, FDEF_ACTION);
	set_fmt(ACTION_OTHER_FMT, FDEF_ACTION_OTHER);
	set_fmt(CTCP_FMT, FDEF_CTCP);
	set_fmt(CTCP_PRIVATE_FMT, FDEF_CTCP_PRIVATE);
	set_fmt(CTCP_REPLY_FMT, FDEF_CTCP_REPLY);
	set_fmt(CTCP_UNKNOWN_FMT, FDEF_CTCP_UNKNOWN);
	set_fmt(DCC_CHAT_FMT, FDEF_DCC_CHAT);
	set_fmt(DCC_CHAT_ACTION_FMT, FDEF_DCC_CHAT_ACTION);
	set_fmt(DCC_TALK_FMT, FDEF_DCC_TALK);
	set_fmt(GONE_FMT, FDEF_GONE);
	set_fmt(INVITE_FMT, FDEF_INVITE);
	set_fmt(JOIN_FMT, FDEF_JOIN);
	set_fmt(JOIN_SELF_FMT, FDEF_JOIN_SELF);
	set_fmt(KICK_FMT, FDEF_KICK);
	set_fmt(KICK_SELF_FMT, FDEF_KICK_SELF);
	set_fmt(KILL_FMT, FDEF_KILL);
	set_fmt(MODE_FMT, FDEF_MODE);
	set_fmt(MSG_FMT, FDEF_MSG);
	set_fmt(MSG_GROUP_FMT, FDEF_MSG_GROUP);
	set_fmt(NICK_FMT, FDEF_NICK);
	set_fmt(NOTICE_FMT, FDEF_NOTICE);
	set_fmt(NOTIFY_SIGNOFF_FMT, FDEF_NOTIFY_SIGNOFF);
	set_fmt(NOTIFY_SIGNON_FMT, FDEF_NOTIFY_SIGNON);
	set_fmt(NOTIFY_SIGNON_USERHOST_FMT, FDEF_NOTIFY_SIGNON_USERHOST);
	set_fmt(OPS_FMT, FDEF_OPS);
	set_fmt(OPS_OTHER_FMT, FDEF_OPS_OTHER);
	set_fmt(PART_FMT, FDEF_PART);
	set_fmt(PART_SELF_FMT, FDEF_PART_SELF);
	set_fmt(PUBLIC_FMT, FDEF_PUBLIC);
	set_fmt(PUBLIC_ACTION_FMT, FDEF_PUBLIC_ACTION);
	set_fmt(PUBLIC_MSG_FMT, FDEF_PUBLIC_MSG);
	set_fmt(PUBLIC_NOTICE_FMT, FDEF_PUBLIC_NOTICE);
	set_fmt(PUBLIC_OTHER_FMT, FDEF_PUBLIC_OTHER);
	set_fmt(PUBLIC_ACTION_FMT, FDEF_PUBLIC_ACTION);
	set_fmt(RECORDER_BODY_FMT, FDEF_RECORDER_BODY);
	set_fmt(RECORDER_HEAD_FMT, FDEF_RECORDER_HEAD);
	set_fmt(SEND_ACTION_FMT, FDEF_SEND_ACTION);
	set_fmt(SEND_DCC_CHAT_FMT, FDEF_SEND_DCC_CHAT);
	set_fmt(SEND_DCC_TALK_FMT, FDEF_SEND_DCC_TALK);
	set_fmt(SEND_LOCOPS_FMT, FDEF_SEND_LOCOPS);
	set_fmt(SEND_MSG_FMT, FDEF_SEND_MSG);
	set_fmt(SEND_NOTICE_FMT, FDEF_SEND_NOTICE);
	set_fmt(SEND_OPERWALL_FMT, FDEF_SEND_OPERWALL);
	set_fmt(SEND_OPS_FMT, FDEF_SEND_OPS);
	set_fmt(SEND_OPS_OTHER_FMT, FDEF_SEND_OPS_OTHER);
	set_fmt(SEND_PUBLIC_FMT, FDEF_SEND_PUBLIC);
	set_fmt(SEND_PUBLIC_ACTION_FMT, FDEF_SEND_PUBLIC_ACTION);
	set_fmt(SEND_PUBLIC_MSG_FMT, FDEF_SEND_PUBLIC_MSG);
	set_fmt(SEND_PUBLIC_OTHER_FMT, FDEF_SEND_PUBLIC_OTHER);
	set_fmt(SEND_WALLOPS_FMT, FDEF_SEND_WALLOPS);
	set_fmt(SERVER_KILL_FMT, FDEF_SERVER_KILL);
	set_fmt(SERVER_NOTICE_FMT, FDEF_SERVER_NOTICE);
	set_fmt(SERVER_WALLOPS_FMT, FDEF_SERVER_WALLOPS);
	set_fmt(TOPIC_FMT, FDEF_TOPIC);
	set_fmt(QUIT_FMT, FDEF_QUIT);
	set_fmt(UMODE_FMT, FDEF_UMODE);
	set_fmt(WALL_FMT, FDEF_WALL);
	set_fmt(WALLOPS_FMT, FDEF_WALLOPS);
	set_fmt(WHOIS_ADMIN_FMT, FDEF_WHOIS_ADMIN);
	set_fmt(WHOIS_AWAY_FMT, FDEF_WHOIS_AWAY);
	set_fmt(WHOIS_CHANNELS_FMT, FDEF_WHOIS_CHANNELS);
	set_fmt(WHOIS_IDLE_FMT, FDEF_WHOIS_IDLE);
	set_fmt(WHOIS_IRCNAME_FMT, FDEF_WHOIS_IRCNAME);
	set_fmt(WHOIS_NSLOOKUP_FMT, FDEF_WHOIS_NSLOOKUP);
	set_fmt(WHOIS_OPER_FMT, FDEF_WHOIS_OPER);
	set_fmt(WHOIS_SERVER_FMT, FDEF_WHOIS_SERVER);
	set_fmt(WHOIS_SIGNON_FMT, FDEF_WHOIS_SIGNON);
	set_fmt(WHOIS_USERHOST_FMT, FDEF_WHOIS_USERHOST);
	set_fmt(WHOWAS_USERHOST_FMT, FDEF_WHOWAS_USERHOST);
}

int
find_format(org_name, cnt)
char	*org_name;
int	*cnt;
{
	IrcFormat *f,
		  *first;
	int	len,
		fmt_index;
	char	*name = (char *) 0;

	malloc_strcpy(&name, org_name);
	upper(name);
	len = strlen(name);
	fmt_index = 0;
	for (first = irc_format; first->name; first++, fmt_index++) {
		if (strncmp(name, first->name, len) == 0) {
			*cnt = 1;
			break;
		}
	}
	if (first->name) {
		if (strlen(first->name) != len) {
			f = first;
			for (f++; f->name; f++, (*cnt)++) {
				if (strncmp(name, f->name, len) != 0)
					break;
			}
		}
		new_free(&name);
		return fmt_index;
	} else {
		*cnt = 0;
		new_free(&name);
		return -1;
	}
}

void
formatcmd(command, args)
char	*command,
	*args;
{
	NumFormat *temp;
	char	*which;
	int	i, cnt;
	
	which = my_next_arg(&args);
	if (which && *which) {
		if (is_number(which))
			set_num_format(atoi(which), args);
		else {
			i = find_format(which, &cnt);
			switch (cnt) {
			case 0:
				say("No such format \"%s\"", which);
				break;
			case 1:
				set_format(i, args);
				break;
			default:
				say("%s is ambiguous", which);
				for (cnt += i; i < cnt; i++)
					set_format(i, empty_string);
			}
		}
	} else {
		temp = num_format.next;
		while (temp) {
			set_num_format(temp->num, empty_string);
			temp = temp->next;
		}
		for (i = 0; i < NUMBER_OF_FORMATS; i++)
				set_format(i, empty_string);
	}
}

char *
do_format(FormatStr, ParamStr, Args)
char	*FormatStr,
	*ParamStr,
	**Args;
{
	static char	result[1024];
	char	this;
	int	pos = 0;

	bzero(result, 1024);

	while ((pos < 1023) && ((this = *FormatStr++) != 0)) {
		if (this == '%')
			switch (this = *FormatStr++) {
			case '%':
			  result[pos++] = '%';
			  break;
			case '!':
			  if (hilite)
			    result[pos++] = highlight_char;
			  break;
			case '@':
			  {
			    char *TS;
			    TS = TimeStamp();
			    strmcat(result, TS, 1023);
			    pos += strlen(TS);
			  }
			  break;
			case 'B':
			case 'b':
			  {
			    char *foo = get_string_var(BANNER_VAR);
			    if (foo)
			    {
			      strmcat(result, foo, 1023);
			      pos += strlen(foo);
			    }
			  }
			  break;
			case 'u':
			case 'U':
			  if (FromUserHost) {
			  	strmcat(result, FromUserHost, 1023);
			  	pos += strlen(FromUserHost);
			  }
			  break;
			case '*':
			  if (FromUserHost) {
			    char *uh;
			    uh = cluster(FromUserHost);
			    strmcat(result, uh, 1023);
			    pos += strlen(uh);
			  }
			  break;
			default:
			  {
			    int i;
			    for (i = 0; ParamStr[i]; i++)
			      if ((ParamStr[i] == toupper(this)) && Args[i]) {
				strmcat(result, Args[i], 1023);
				pos += strlen(Args[i]);
				break;
			      }
			  }
			}
		else
			result[pos++] = this;
	}
	hilite = 0;
	return (char *) &result;
}

char *
parseformat(FmtNum, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
int	FmtNum;
char	*arg1, *arg2, *arg3, *arg4, *arg5,
	*arg6, *arg7, *arg8, *arg9, *arg10;
{
	char	*args[10];

	args[0] = arg1; args[1] = arg2; args[2] = arg3; args[3] = arg4;
	args[4] = arg5; args[5] = arg6; args[6] = arg7; args[7] = arg8;
	args[8] = arg9; args[9] = arg10;

	return do_format(irc_format[FmtNum].fmt, irc_format[FmtNum].args, args);
}

char *
numericformat(FmtNum, Server, Banner, Args)
int	FmtNum;
char	*Server,
	*Banner,
	**Args;
{
	NumFormat *temp;
	char	*srvr = (char *) 0;
	char	*args[12] =	{ empty_string, empty_string, empty_string,
				empty_string, empty_string, empty_string,
				empty_string, empty_string, empty_string,
				empty_string, empty_string, empty_string };
	int	i;

	if (Server)  {
		srvr = new_malloc(strlen(Server) + 7);
		sprintf(srvr, "(from %s)", Server);
	} else
		malloc_strcpy(&srvr, empty_string);
	if (Server && (my_strnicmp(get_server_itsname(from_server), Server,
			strlen(get_server_itsname(from_server))) == 0))
		args[0] = empty_string;
	else
		args[0] = srvr;
	args[1] = Banner;
	for (i = 0; i < 10 && Args[i]; i++)
	  args[2+i] = Args[i];

	temp = num_format.next;
	while (temp) {
		if (temp->num == FmtNum)
			return do_format(temp->fmt, "S#0123456789", args);
		temp = temp->next;
	}
	new_free(srvr);
	return (char *) 0;
}

void
set_num_format(FmtNum, Fmt)
int	FmtNum;
char	*Fmt;
{
	NumFormat *temp, *last;
	int remove = 0;

	if (FmtNum < 0) {
		FmtNum = -FmtNum;
		remove = 1;
	}

	temp = num_format.next;
	last = &num_format;
	while (temp && (temp->num <= FmtNum)) {
		if (temp->num == FmtNum) {
			if (remove) {
				last->next = temp->next;
				new_free(&temp->fmt);
				new_free(&temp);
				temp = last->next;
				say("%03d format removed.", FmtNum);
				return;
			} else if (Fmt && *Fmt) {
				malloc_strcpy(&temp->fmt, Fmt);
				temp->int_flags |= VIF_CHANGED;
				if (loading_global)
					temp->int_flags |= VIF_GLOBAL;
				say("Format of %03d set to \"%s\"", FmtNum, Fmt);
				return;
			} else {
				if (temp->fmt)
					say("%03d = \"%s\"", FmtNum, temp->fmt);
				else
					say("%03d = <empty>", FmtNum);
				return;
			}
		}
		last = temp;
		temp = temp->next;
	}
	if (remove)
		say("No format for %03d has been set.", FmtNum);
	else if (Fmt && *Fmt) {
		NumFormat *new = (NumFormat *) new_malloc(sizeof(NumFormat));
		new->num = FmtNum;
		new->fmt = (char *) 0;
		malloc_strcpy(&new->fmt, Fmt);
		temp = num_format.next;
		last = &num_format;
		while (temp && (temp->num < FmtNum)) {
			last = temp;
			temp = temp->next;
		}
		new->next = temp;
		last->next = new;
		say("Format of %03d set to \"%s\"", FmtNum, Fmt);
	} else
		say("No format for %03d has been set.", FmtNum);
}

void
set_format(FmtNum, Fmt)
int	FmtNum;
char	*Fmt;
{
	if ((FmtNum < 0) || (FmtNum >= NUMBER_OF_FORMATS))
		return;
	if (Fmt && *Fmt) {
		irc_format[FmtNum].int_flags |= VIF_CHANGED;
		if (loading_global)
			irc_format[FmtNum].int_flags |= VIF_GLOBAL;
		malloc_strcpy(&irc_format[FmtNum].fmt, Fmt);
		say("Format of %s set to \"%s\"", irc_format[FmtNum].name, Fmt);
	} else {
		if (irc_format[FmtNum].fmt)
			say("%-22s = \"%s\"", irc_format[FmtNum].name,
				irc_format[FmtNum].fmt);
		else
			say("%-22s = <empty>", irc_format[FmtNum].name);
	}
}

void
set_fmt(FmtNum, Fmt)
int	FmtNum;
char	*Fmt;
{
	if ((FmtNum < 0) || (FmtNum >= NUMBER_OF_FORMATS))
		return;
	malloc_strcpy(&irc_format[FmtNum].fmt, Fmt);
}

void
save_formats(fp)
FILE	*fp;
{
	int	i;
	NumFormat *temp = num_format.next;

	for (temp = num_format.next; temp; temp = temp->next)
		if (temp->fmt)
			fprintf(fp, "FORMAT %03d %s\n", temp->num,
				temp->fmt);
	for (i = 0; i < NUMBER_OF_FORMATS; i++)
		if ((irc_format[i].int_flags & VIF_CHANGED) && irc_format[i].fmt)
			fprintf(fp, "FORMAT %s %s\n", irc_format[i].name,
				irc_format[i].fmt);
}

void
do_highlight(yesno)
int	yesno;
{
	hilite = yesno;
}

int
num_format_exists(FmtNum)
int	FmtNum;
{
	NumFormat *temp = num_format.next;
	while (temp && (temp->num <= FmtNum)) {
		if (temp->num == FmtNum)
			return 1;
		temp = temp->next;
	}
	return 0;
}

int
format_get(fmt_name, set_fmt)
	char		*fmt_name;
	IrcFormat	*set_fmt;
{
	IrcFormat	*fmt;
	int		i = 0;

	for (fmt = irc_format; fmt->name; fmt++)
	{
		if (!my_stricmp(fmt_name, fmt->name))
		{
			if (set_fmt)
			{
				set_fmt->name = fmt->name;
				set_fmt->args = fmt->args;
				malloc_strcpy(&set_fmt->fmt, fmt->fmt);
				set_fmt->int_flags = fmt->int_flags;
			}
			return i;
		}
		i++;
	}
	return -1;
}


int
format_set(fmt_name, set_fmt)
	char		*fmt_name;
	IrcFormat	*set_fmt;
{
	IrcFormat	*fmt;
	int		i = 0;

	for (fmt = irc_format; fmt->name; fmt++)
	{
		if (!my_stricmp(fmt_name, fmt->name))
		{
			if (set_fmt)
			{
				malloc_strcpy(&fmt->fmt, set_fmt->fmt);
				fmt->int_flags = set_fmt->int_flags;
			}
			return i;
		}
		i++;
	}
	return -1;
}


int
format_nget(fmt_num, set_fmt)
	int		fmt_num;
	NumFormat	*set_fmt;
{
	NumFormat	*fmt;

	for (fmt = num_format.next; fmt; fmt = fmt->next)
	{
		if (fmt->num == fmt_num)
		{
			if (set_fmt)
			{
				set_fmt->num = fmt->num;
				malloc_strcpy(&set_fmt->fmt, fmt->fmt);
				set_fmt->int_flags = fmt->int_flags;
			}
			return fmt->num;
		}
	}
	return -1;
}


int
format_nset(fmt_num, set_fmt)
	int		fmt_num;
	NumFormat	*set_fmt;
{
	NumFormat	*fmt,
			*last = &num_format;

	for (fmt = last->next; ; fmt = fmt->next)
	{
		if (!fmt || (fmt->num > fmt_num))
		{
			if (set_fmt)
			{
				NumFormat	*new;

				new = (NumFormat *) new_malloc(sizeof(NumFormat));
				memset(new, 0, sizeof(NumFormat));
				new->num = fmt_num;
				malloc_strcpy(&new->fmt, set_fmt->fmt);
				new->int_flags = set_fmt->int_flags;
				new->next = last->next;
				last->next = new;
			}
			return fmt_num;
		}
		else if (fmt->num == fmt_num)
		{
			if (set_fmt)
			{
				fmt->num = set_fmt->num;
				malloc_strcpy(&fmt->fmt, set_fmt->fmt);
				fmt->int_flags = set_fmt->int_flags;
			}
			return fmt_num;
		}
		last = fmt;
	}
	return -1;
}

