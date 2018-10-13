/*
 * edit.c: This is really a mishmash of function and such that deal with IRCII
 * commands (both normal and keybinding commands) 
 *
 * Written By Timothy Jensen
 *
 * Copyright(c) 1999-2001 
 *
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: edit.c,v 1.19.2.3 2002/03/11 19:24:04 toast Exp $";
#endif

#include "irc.h"

#include <sys/stat.h>

#ifdef ESIX
# include <lan/net_types.h>
#endif

#include "parse.h"
#include "term.h"
#include "server.h"
#include "edit.h"
#include "crypt.h"
#include "vars.h"
#include "ircaux.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "whois.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "names.h"
#include "alias.h"
#include "history.h"
#include "funny.h"
#include "ctcp.h"
#include "dcc.h"
#include "translat.h"
#include "output.h"
#include "exec.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "if.h"
#include "help.h"
#include "stack.h"
#include "queue.h"
#include "comstud.h"
#include "toast.h"
#include "format.h"
#include "cignore.h"
#include "recorder.h"
#include "fileio.h"
#include "autoop.h"
#include "rejoin.h"

/*
 * current_exec_timer - used to make sure we don't remove a timer
 * from within itself.
 */
static	int	current_exec_timer = -1;

static	int	save_which;
static	int	save_do_all;

static	void	show_timer();
static	int	create_timer_ref();

TimerList *PendingTimers = (TimerList *) 0;

/* used with input_move_cursor */
#define RIGHT 1
#define LEFT 0

/* used with /save */
#define	SFLAG_ALIAS	0x0001
#define	SFLAG_BIND	0x0002
#define	SFLAG_ON	0x0004
#define	SFLAG_SET	0x0008
#define	SFLAG_NOTIFY	0x0010
#define	SFLAG_DIGRAPH	0x0020
#define SFLAG_AUTOOP	0x0040
#define SFLAG_FORMAT	0x0080
#define SFLAG_ALL	(SFLAG_ALIAS | SFLAG_BIND | SFLAG_ON | SFLAG_SET \
			| SFLAG_NOTIFY | SFLAG_DIGRAPH | SFLAG_AUTOOP \
			| SFLAG_FORMAT)

/* The maximum number of recursive LOAD levels allowed */
#define MAX_LOAD_DEPTH 10

/* recv_nick: the nickname of the last person to send you a privmsg */
	char	*recv_nick = NULL;

/* sent_nick: the nickname of the last person to whom you sent a privmsg */
	char	*sent_nick = NULL;
	char	*sent_body = NULL;

/* Used to keep down the nesting of /LOADs and to determine if we
 * should activate the warning for /ONs if the NOVICE variable is set.
 */
	int	load_depth = 0;

/* Used to prevent global messaging */
extern	int	in_on_who;

typedef	struct	WaitCmdstru
{
	char	*stuff;
	struct	WaitCmdstru	*next;
}	WaitCmd;

static	WaitCmd	*start_wait_list = NULL,
		*end_wait_list = NULL;

	char	lame_wait_nick[] = "1#LAME";

/* a few advance declarations */
static	void	sendlinecmd _((char *, char *, char *));
static	void	do_send_text _((char *, char *, char *));
static	void	funny_stuff _((char *, char *, char *));
static	void	cd _((char *, char *, char *));
static	void	e_quit _((char *, char *, char *));
static	void	e_wall _((char *, char *, char *));
static	void	send_2comm _((char *, char *, char *));
static	void	send_comm _((char *, char *, char *));
/*static	void	send_topic _((char *, char *, char *));*/
static	void	send_kick _((char *, char *, char *));
/*static	void	send_channel_com _((char *, char *, char *));*/
static	void	clear _((char *, char *, char *));
static	void	quote _((char *, char *, char *));
static	void	e_privmsg _((char *, char *, char *));
static	void	flush _((char *, char *, char *));
static	void	away _((char *, char *, char *));
static	void	oper _((char *, char *, char *));
static	void	e_channel _((char *, char *, char *));
/*static	void	who _((char *, char *, char *));
static	void	whois _((char *, char *, char *));*/
static	void	ison _((char *, char *, char *));
static	void	userhost _((char *, char *, char *));
static	void	info _((char *, char *, char *));
static	void	e_nick _((char *, char *, char *));
static	void	comment _((char *, char *, char *));
static	void	sleepcmd _((char *, char *, char *));
static	void	version _((char *, char *, char *));
/*static	void	ctcp _((char *, char *, char *));*/
static	void	dcc _((char *, char *, char *));
static	void	deop _((char *, char *, char *));
static	void	echo _((char *, char *, char *));
static	void	save_settings _((char *, char *, char *));
static	void	redirect _((char *, char *, char *));
static	void	waitcmd _((char *, char *, char *));
static	void	describe _((char *, char *, char *));
static	void	me _((char *, char *, char *));
static	void	mload _((char *, char *, char *));
static	void	mlist _((char *, char *, char *));
static	void	evalcmd _((char *, char *, char *));
static	void	hook _((char *, char *, char *));
static	void	timercmd _((char *, char *, char *));
static	void	inputcmd _((char *, char *, char *));
static	void	pingcmd _((char *, char *, char *));
static	void	xtypecmd _((char *, char *, char *));
static	void	beepcmd _((char *, char *, char *));
static	void	abortcmd _((char *, char *, char *));
static	void	really_save _((char *, char *));
static	void	rel_msg _((char *, char *, char *));

#ifdef ALLOC_DEBUG
extern	void	alloc_cmd();
#endif

/* IrcCommand: structure for each command in the command table */
typedef	struct
{
	char	*name;					/* what the user types */
	char	*server_func;				/* what gets sent to the server
							 * (if anything) */
	void	(*func) _((char *, char *, char *));	/* function that is the command */
	unsigned	flags;
}	IrcCommand;

#define NONOVICEABBREV 0x0001
#define	NOINTERACTIVE  0x0002
#define	NOSIMPLESCRIPT 0x0004
#define	NOCOMPLEXSCRIPT 0x0008

/*
 * irc_command: all the availble irc commands:  Note that the first entry has
 * a zero length string name and a null server command... this little trick
 * makes "/ blah blah blah" to always be sent to a channel, bypassing queries,
 * etc.  Neato.  This list MUST be sorted.
 */
static	IrcCommand irc_command[] =
{
	{ "",		empty_string,	do_send_text,		NOSIMPLESCRIPT| NOCOMPLEXSCRIPT },
	/*
	 * I really want to remove #, but it will break a lot of scripts.  - mycroft
	 *
	 * I agree - it should be converted to a special character in parse_line.
	 *                                                            - Troy
	 */
	{ "#",		NULL,		comment, 		0 },
	{ ":",		NULL,		comment, 		0 },
        { "ABORT",      NULL,           abortcmd,               0 },
	{ "ADMIN",	"ADMIN",	send_comm, 		0 },
	{ "ALIAS",	"0",		alias,			0 },
#ifdef ALLOC_DEBUG
	{ "ALLOC",	NULL,		alloc_cmd,		0 },
#endif
	{ "ASSIGN",	"1",		alias,			0 },
	{ "AUTOOP",	NULL,		autoop,			0 },
	{ "AWAY",	"AWAY",		away,			0 },
	{ "AWAYLIST",	NULL,		awaylistcmd,		0 },
	{ "B",		NULL,		ban,			0 },
	{ "BAN",	NULL,		ban,			0 },
	{ "BANCOUNT",	NULL,		bancount,		0 },
	{ "BEEP",	0,		beepcmd,		0 },
	{ "BIND",	NULL,		bindcmd,		0 },
	{ "BYE",	"QUIT",		e_quit,			NONOVICEABBREV},
	{ "C",		NULL,		domode,			0 },
	{ "CD",		NULL,		cd,			0 },
	{ "CHANNEL",	"JOIN",		e_channel,		0 },
	{ "CHAT",	NULL,		chat_cmd,		0 },
	{ "CIGNORE",	NULL,		cignore,		0 },
	{ "CLEAR",	NULL,		clear,			0 },
	{ "COMMENT",	NULL,		comment,		0 },
	{ "CONNECT",	"CONNECT",	send_comm,		0 },
	{ "CSTAT",	NULL,		cstat,			0 },
	{ "CTCC",	NULL,		dcc,			0 },
	{ "CTCP",	NULL,		ctcp,			0 },
	{ "CYCLE",	NULL,		cycle,			0 },
	{ "DATE",	"TIME",		send_comm,		0 },
	{ "DCC",	NULL,		dcc,			0 },
	{ "DEOP",	NULL,		dodeop,			0 },
	{ "DEOPER",	NULL,		deop,			0 },
	{ "DESCRIBE",	NULL,		describe,		0 },
	{ "DEVOICE",	NULL,		dodevoice,		0 },
	{ "DIE",	"DIE",		send_comm,		0 },
	{ "DIGRAPH",	NULL,		digraph,		0 },
	{ "DISCONNECT",	NULL,		disconnectcmd,		0 },
	{ "DLINE",	"DLINE",	dokline,		0 },
	{ "DMSG",	"PRIVMSG",	dmsg,			0 },
	{ "ECHO",	NULL,		echo,			0 },
	{ "ENCRYPT",	NULL,		encrypt_cmd,		0 },
	{ "ERASE",	NULL,		clear_msgs,		0 },
	{ "EVAL",	NULL,		evalcmd,		0 },
	{ "EXEC",	NULL,		execcmd,		0 },
	{ "EXIT",	"QUIT",		e_quit,			NONOVICEABBREV},
	{ "FCLOSE",	NULL,		fclosecmd,		0 },
	{ "FE",		NULL,		foreach_handler,	0 },
	{ "FEC",	NULL,		fec,			0 },
	{ "FINDD",	NULL,		findd,			0 },
	{ "FINDK",	NULL,		findk,			0 },
	{ "FINDU",	NULL,		findu,			0 },
	{ "FINGER",	NULL,		finger,			0 },
	{ "FLUSH",	NULL,		flush,			0 },
	{ "FOR",	NULL,		foreach_handler,	0 },
	{ "FOREACH",	NULL,		foreach_handler,	0 },
	{ "FORMAT",	NULL,		formatcmd,		0 },
	{ "FPUTC",	NULL,		fputccmd,		0 },
	{ "FSEEK",	NULL,		fseekcmd,		0 },
	{ "FWRITE",	NULL,		fwritecmd,		0 },
	{ "FWRITELN",	NULL,		fwritelncmd,		0 },
	{ "GONE",	"GONE",		away,			0 },
	{ "HASH",	"HASH",		send_comm,		0 },
	{ "HELP",	NULL,		help,			0 },
	{ "HISTORY",	NULL,		history,		0 },
	{ "HOOK",	NULL,		hook,			0 },
	{ "HOST",	"USERHOST",	userhost,		0 },
	{ "IF",		NULL,		ifcmd,			0 },
	{ "IGH",	NULL,		my_ignorehost,		0 },
	{ "IGN",	NULL,		my_ignore,		0 },
	{ "IGNORE",	NULL,		ignore,			0 },
	{ "INFO",	"INFO",		info,			0 },
	{ "INPUT",	NULL,		inputcmd,		0 },
	{ "INVITE",	"INVITE",	invitecmd,		0 },
	{ "ISON",	"ISON",		ison,			0 },
	{ "J",		"J",		e_channel,		0 },
	{ "JOIN",	"JOIN",		e_channel,		0 },
	{ "K",		NULL,		dokick,			0 },
	{ "KB",		NULL,		kickban,		0 },
	{ "KICK",	"KICK",		send_kick,		0 },
	{ "KILL",	"KILL",		send_2comm,		0 },
	{ "KLINE",	"KLINE",	dokline,		0 },
	{ "L",		NULL,		doleave,		0 },
	{ "LASTLOG",	NULL,		lastlog,		0 },
	{ "LEAVE",	"PART",		send_channel_com,	0 },
	{ "LINKS",	"LINKS",	send_comm,		NONOVICEABBREV},
	{ "LIST",	"LIST",		funny_stuff,		0 },
	{ "LOAD",	"LOAD",		load,			0 },
	{ "LOCOPS",	"LOCOPS",	e_wall,			0 },
	{ "LUSERS",	"LUSERS",	send_comm,		0 },
	{ "M",		"PRIVMSG",	e_privmsg,		0 },
	{ "MB",		NULL,		massban,		0 },
	{ "MD",		NULL,		massdeop,		0 },
	{ "MDEVOICE",	NULL,		massdevoice,		0 },
	{ "MDV",	NULL,		massdevoice,		0 },
	{ "ME",		NULL,		me,			0 },
	{ "MK",		NULL,		masskick,		0 },
	{ "MKB",	NULL,		masskickban,		0 },
	{ "MLIST",	NULL,		mlist,			0 },
	{ "MLOAD",	NULL,		mload,			0 },
	{ "MODE",	"MODE",		send_channel_com,	0 },
	{ "MOP",	NULL,		massop,			0 },
	{ "MOTD",	"MOTD",		send_comm,		0 },
	{ "MSG",	"PRIVMSG",	e_privmsg,		0 },
	{ "MUB",	NULL,		unban,			0 },
	{ "MULT",	NULL,		multkick,		0 },
	{ "MV",		NULL,		massvoice,		0 },
	{ "MVOICE",	NULL,		massvoice,		0 },
	{ "NAMES",	"NAMES",	funny_stuff,		0 },
	{ "NEWHOST",	NULL,		newhost,		0 },
	{ "NEWNICK",	NULL,		newnick,		0 },
	{ "NEWUSER",	NULL,		newuser,		0 },
	{ "NICK",	"NICK",		e_nick,			0 },
	{ "NOLOG",	NULL,		nolog,			0 },
	{ "NOTE",	"NOTE",		send_comm,		0 },
	{ "NOTICE",	"NOTICE",	e_privmsg,		0 },
	{ "NOTIFY",	NULL,		notify,			0 },
	{ "NSLOOKUP",	NULL,		nslookup,		0 },
	{ "ON",		NULL,		on,			0 },
	{ "OOPS",	NULL,		oopscmd,		0 },
	{ "OP",		NULL,		doop,			0 },
	{ "OPER",	"OPER",		oper,			0 },
	{ "OPERWALL",	"OPERWALL",	e_wall,			0 },
	{ "OPS",	NULL,		tochanops,		0 },
	{ "OV",		NULL,		do_opervision,		0 },
	{ "PARSEKEY",	NULL,		parsekeycmd,		0 },
	{ "PART",	"PART",		send_channel_com,	0 },
	{ "PING",	NULL, 		pingcmd,		0 },
	{ "PLAY",	NULL,		play_msgs,		0 },
	{ "QUERY",	NULL,		query,			0 },
	{ "QUEUE",      NULL,           queuecmd,               0 },
	{ "QUIT",	"QUIT",		e_quit,			NONOVICEABBREV},
	{ "QUOTE",	NULL,		quote,			0 },
	{ "RBIND",	0,		rbindcmd,		0 },
	{ "RCHAT",	NULL,		rchat_cmd,		0 },
	{ "REDIRECT",	NULL,		redirect,		0 },
	{ "REHASH",	"REHASH",	send_comm,		0 },
	{ "REJOIN",	NULL,		rejoin,			0 },
	{ "RELM",       "RELM",		rel_msg,		0 },
	{ "RELN",       "RELN",		rel_msg,		0 },
	{ "RELS",       "RELS",		rel_msg,		0 },
	{ "RELSN",	"RELSN",	rel_msg,		0 },
	{ "REPEAT",	"REPEAT",	repeatcmd,		0 },
	{ "REQUEST",	NULL,		ctcp,			0 },
	{ "RESET",	NULL,		reset,			0 },
	{ "RESTART",	"RESTART",	send_comm,		0 },
	{ "SAVE",	NULL,		save_settings,		0 },
	{ "SAY",	empty_string,	do_send_text,		0 },
	{ "SB",		NULL,		siteban,		0 },
	{ "SBAN",	NULL,		siteban,		0 },
	{ "SBK",	NULL,		sitekickban,		0 },
	{ "SEND",	NULL,		do_send_text,		0 },
	{ "SENDLINE",	empty_string,	sendlinecmd,		0 },
	{ "SERVER",	NULL,		server,			0 },
	{ "SET",	NULL,		set_variable,		0 },
	{ "SIGNOFF",	"QUIT",		e_quit,			NONOVICEABBREV},
	{ "SKB",	NULL,		sitekickban,		0 },
	{ "SLEEP",	NULL,		sleepcmd,		0 },
	{ "SPING",	NULL,		sping,			0 },
	{ "SQUIT",	"SQUIT",	send_2comm,		0 },
	{ "STACK",	NULL,		stackcmd,		0 },
	{ "STATS",	"STATS",	send_comm,		0 },
	{ "SUMMON",	"SUMMON",	send_comm,		0 },
	{ "SWITCH",	"SWITCH",	switchcmd,		0 },
	{ "T",		NULL,		dotopic,		0 },
	{ "TIME",	"TIME",		send_comm,		0 },
	{ "TIMER",	"TIMER",	timercmd,		0 },
	{ "TKLINE",	"TKLINE",	dokline,		0 },
	{ "TOPIC",	"TOPIC",	send_topic,		0 },
	{ "TRACE",	"TRACE",	send_comm,		0 },
	{ "TRACEKILL",	NULL,		tracekill,		0 },
	{ "TRACESERV",	NULL,		traceserv,		0 },
	{ "TYPE",	NULL,		type,			0 },
	{ "UB",		NULL,		unban,			0 },
	{ "UMODE",	NULL,		umode,			0 },
	{ "UNBAN",	NULL,		unban,			0 },
	{ "UNKLINE",	NULL,		unkline,		0 },
	{ "USERHOST",	NULL,		userhost,		0 },
/*	{ "USERS",	"USERS",	send_comm,		0 }, */
	{ "USERS",	NULL,		users,			0 },
	{ "VER",	NULL,		vercmd,			0 },
	{ "VERSION",	"VERSION",	version,		0 },
	{ "VOICE",	NULL,		dovoice,		0 },
	{ "W",		"W",		who,			0 },
	{ "WAIT",	NULL,		waitcmd,		0 },
	{ "WALL",	NULL,		tochanops,		0 },
	{ "WALLOPS",	"WALLOPS",	e_wall,			0 },
	{ "WHICH",	"WHICH",	load,			0 },
	{ "WHILE",	NULL,		whilecmd,		0 },
	{ "WHO",	"WHO",		who,			0 },
	{ "WHOIS",	"WHOIS",	whois,			0 },
	{ "WHOKILL",	NULL,		whokill,		0 },
	{ "WHOWAS",	"WHOWAS",	whois,			0 },
	{ "WI",		"WHOIS",	whois,			0 },
	{ "WII",	NULL,		dowii,			0 },
	{ "WINDOW",	NULL,		window,			0 },
	{ "WW",		"WHOWAS",	whois,			0 },
	{ "XECHO",	"XECHO",	echo,			0 },
	{ "XTRA",	"XTRA",		e_privmsg,		0 },
	{ "XTYPE",	NULL,		xtypecmd,		0 },
	{ NULL,		NULL,		comment,		0 }
};

/* number of entries in irc_command array */
# define	NUMBER_OF_COMMANDS (sizeof(irc_command) / sizeof(IrcCommand)) - 2

/*
 * find_command: looks for the given name in the command list, returning a
 * pointer to the first match and the number of matches in cnt.  If no
 * matches are found, null is returned (as well as cnt being 0). The command
 * list is sorted, so we do a binary search.  The returned commands always
 * points to the first match in the list.  If the match is exact, it is
 * returned and cnt is set to the number of matches * -1.  Thus is 4 commands
 * matched, but the first was as exact match, cnt is -4.
 */
static	IrcCommand *
find_command(com, cnt)
	char	*com;
	int	*cnt;
{
	int	len;

	if (com && (len = strlen(com)))
	{
		int	min,
		max,
		pos,
		old_pos = -1,
		c;

		min = 1;
		max = NUMBER_OF_COMMANDS + 1;
		while (1)
		{
			pos = (max + min) / 2;
			if (pos == old_pos)
			{
				*cnt = 0;
				return ((IrcCommand *) 0);
			}
			old_pos = pos;
			c = strncmp(com, irc_command[pos].name, len);
			if (c == 0)
				break;
			else if (c > 0)
				min = pos;
			else
				max = pos;
		}
		*cnt = 0;
		(*cnt)++;
		min = pos - 1;
		while ((min > 0) && (strncmp(com, irc_command[min].name,
				len) == 0))
		{
			(*cnt)++;
			min--;
		}
		min++;
		max = pos + 1;
		while ((max < NUMBER_OF_COMMANDS + 1) && (strncmp(com,
				irc_command[max].name, len) == 0))
		{
			(*cnt)++;
			max++;
		}
		if (*cnt)
		{
			if (strlen(irc_command[min].name) == len)
				*cnt *= -1;
			else if (*cnt == 1 && 
					irc_command[min].flags&NONOVICEABBREV &&
					get_int_var(NOVICE_VAR))
			{
				say("As a novice you may not abbreviate the %s command", irc_command[min].name);
				*cnt=0;
				return ((IrcCommand *) 0);
			}
			return (&(irc_command[min]));
		}
		else
			return ((IrcCommand *) 0);
	}
	else
	{
		*cnt = -1;
		return (irc_command);
	}
}

/*ARGSUSED*/
void
ctcp(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*to,
		*tag;
	int	type;

	if ((to = next_arg(args, &args)) != NULL)
	{
		if (!strcmp(to, "*"))
			if ((to = get_channel_by_refnum(0)) == NULL)
				to = "0";
		if ((tag = next_arg(args, &args)) != NULL)
			upper(tag);
		else
			tag = "VERSION";
		if ((type = in_ctcp()) == -1)
			echo(NULL, "*** You may not use the CTCP command in an ON CTCP_REPLY!", empty_string);
		else
		{
			if (args && *args)
				send_ctcp(ctcp_type[type], to, tag, "%s", args);
			else
				send_ctcp(ctcp_type[type], to, tag, NULL);
		}
	}
	else
		say("Request from whom?");
}

/*ARGSUSED*/
static	void
hook(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (*args)
		do_hook(HOOK_LIST, "%s", args);
	else
		say("Must supply an argument to HOOK");
}

/*ARGSUSED*/
static	void
dcc(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
#ifdef DISABLE_DCC
	say("DCC has been disabled.");
#else
	if (*args)
		process_dcc(args);
	else
		dcc_list((char *) NULL);
#endif
}

/*ARGSUSED*/
static	void
deop(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	send_to_server("MODE %s -o", get_server_nickname(from_server));
	set_server_operator(from_server, NULL);
	clear_server_operator_pwd(from_server);
	set_server_operator_nick(from_server, NULL);
}

static	void
funny_stuff(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg,
		*stuff;
	int	min = 0,
		max = 0,
		flags = 0,
		len;

	stuff = empty_string;
	while ((arg = next_arg(args, &args)) != NULL)
	{
		len = strlen(arg);
		if (my_strnicmp(arg, "-MAX", len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
				max = atoi(arg);
		}
		else if (my_strnicmp(arg, "-MIN", len) == 0)
		{
			if ((arg = next_arg(args, &args)) != NULL)
				min = atoi(arg);
		}
		else if (my_strnicmp(arg, "-ALL", len) == 0)
		{
			flags &= ~(FUNNY_PUBLIC | FUNNY_PRIVATE);
		}
		else if (my_strnicmp(arg, "-PUBLIC", len) == 0)
		{
			flags |= FUNNY_PUBLIC;
			flags &= ~FUNNY_PRIVATE;
		}
		else if (my_strnicmp(arg, "-PRIVATE", len) == 0)
		{
			flags |= FUNNY_PRIVATE;
			flags &= ~FUNNY_PUBLIC;
		}
		else if (my_strnicmp(arg, "-TOPIC", len) == 0)
			flags |= FUNNY_TOPIC;
		else if (my_strnicmp(arg, "-WIDE", len) == 0)
			flags |= FUNNY_WIDE;
		else if (my_strnicmp(arg, "-USERS", len) == 0)
			flags |= FUNNY_USERS;
		else if (my_strnicmp(arg, "-NAME", len) == 0)
			flags |= FUNNY_NAME;
		else
			stuff = arg;
	}
	set_funny_flags(min, max, flags);
	if (strcmp(stuff, "*") == 0)
		if (!(stuff = get_channel_by_refnum(0)))
			stuff = empty_string;
	if (index(stuff, '*'))
	{
		funny_match(stuff);
		send_to_server("%s %s", command, empty_string);
	}
	else
	{
		funny_match(NULL);
		send_to_server("%s %s", command, stuff);
	}
}

/*ARGSUSED*/
static void
waitcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	wait_index;
	char	*flag;
	char	*procindex;
	int	cmd = 0,
		len;

	while (args && *args == '-')
	{
		flag = next_arg(args, &args);
		len = strlen(++flag);
		if (!my_strnicmp("CMD", flag, len))
		{
			cmd = 1;
			break;
		}
		else
			yell("Unknown argument to WAIT: %s", flag);
	}
	if ((procindex = next_arg(args, &args)) && *procindex == '%' &&
			(wait_index = get_process_index(&procindex)) != -1)
	{
		if (is_process_running(wait_index))
		{
			if (cmd)
			{
				add_process_wait(wait_index, args?args:empty_string);
				return;
			}
			else
				set_wait_process(wait_index);
		}
		else
		{
			say("Not a valid process!");
			return;
		}
	}
	else if (cmd)
	{
		WaitCmd	*new;

		sprintf(buffer, "%s %s", procindex, args);
		new = (WaitCmd *) new_malloc(sizeof(WaitCmd));
		new->stuff = NULL;
		malloc_strcpy(&new->stuff, buffer);
		new->next = NULL;
		if (end_wait_list)
			end_wait_list->next = new;
		end_wait_list = new;
		if (!start_wait_list)
			start_wait_list = new;
		send_to_server("%s", wait_nick);
		return;
	}
	else
		send_to_server("%s", lame_wait_nick);
	if (waiting)
		yell("WAIT has been called recursively.");

	waiting++;
	irc_io(NULL, NULL, 0, 1);
	waiting--;
}

int
check_wait_command(nick)
	char 	*nick;
{
	if (waiting && !strcmp(nick, lame_wait_nick))
	{
		irc_io_loop = 0;
		return 1;
	}
	if (start_wait_list && !strcmp(nick, wait_nick))
	{
		if (start_wait_list->stuff)
		{
			parse_command(start_wait_list->stuff, 0, empty_string);
			new_free(&start_wait_list->stuff);
		}
		start_wait_list = start_wait_list->next;
		return 1;
	}
	return 0;
}

/*ARGSUSED*/
static	void
redirect(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*who;

	if ((who = next_arg(args, &args)) != NULL)
	{
		if (!strcmp(who, "*"))
			if (!(who = get_channel_by_refnum(0)))
			{
				say("Must be on a channel to redirect to '*'");
				return;
			}
		if (!my_stricmp(who, get_server_nickname(from_server)))
		{
			say("You may not redirect output to yourself");
			return;
		}
		if (*who == '=')
		{
			DCC_list *Client;
			Client = dcc_searchlist("chat", who+1, DCC_CHAT, 1, (char *) 0);
			if (!Client || !(Client->flags&DCC_ACTIVE))
			{
				say("No active DCC CHAT:chat connection for %s",
					who+1);
				return;
			}
		} 
		window_redirect(who, from_server);
		server_list[from_server].sent = 0;
		parse_line((char *) 0, args, (char *) 0, 0, 0);
		if (server_list[from_server].sent)
			send_to_server("%s", current_screen->redirect_token,
				current_screen->screennum);
		else
			window_redirect(NULL, from_server);
	}
	else
		say("Usage: REDIRECT <nick|channel|%process> <cmd>");
}

/*ARGSUSED*/
static	void
sleepcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg;

	if ((arg = next_arg(args, &args)) != NULL)
		sleep(atoi(arg));
	else
		say("SLEEP: you must specify the amount of time to sleep (in seconds)");
}

/*
 * echo: simply displays the args to the screen, or, if it's XECHO,
 * processes the flags first, then displays the text on
 * the screen
 */
static void
echo(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	unsigned int	display;
	int	lastlog_level = 0;
	int	from_level = 0;
	char	*flag_arg;
	int	temp;
	Window *old_to_window;

	old_to_window = to_window;
	if (command && *command == 'X')
	{
		while (args && *args == '-')
		{
			flag_arg = next_arg(args, &args);
			switch(toupper(flag_arg[1]))
			{
			case 'L':
				if (!(flag_arg = next_arg(args, &args)))
					break;
				if ((temp = parse_lastlog_level(flag_arg, "LASTLOG")) != 0)
				{
					lastlog_level =
						set_lastlog_msg_level(temp);
					from_level = message_from_level(temp);
				}
				break;
			case 'W':
				if (!(flag_arg = next_arg(args, &args)))
					break;
				if (isdigit(*flag_arg))
					to_window =
					   get_window_by_refnum(atoi(flag_arg));
				else
					to_window =
						get_window_by_name(flag_arg);
				break;
			}
			if (!args)
				args = empty_string;
		}
	}
	display = window_display;
	window_display = 1;
	put_it("%s", args);
	window_display = display;
	if (lastlog_level)
	{
		set_lastlog_msg_level(lastlog_level);
		message_from_level(from_level);
	}
	to_window = old_to_window;
}

/*
 */
static	void
oper_password_received(data, line)
	char	*data;
	char	*line;
{
	send_to_server("OPER %s %s", data, line);
	if (get_int_var(AUTO_REOPER_VAR))
	{
        	set_server_operator_nick(from_server, data);
        	set_server_operator_pwd(from_server, line);
	}
}

/* oper: the OPER command.  */
/*ARGSUSED*/
static	void
oper(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*password;
	char	*nick;

	oper_command = 1;
	if (!(nick = next_arg(args, &args)))
		nick = nickname;
	if (!(password = next_arg(args, &args)))
	{
		add_wait_prompt("Operator Password:",
			oper_password_received, nick, WAIT_PROMPT_LINE);
		return;
	}
	send_to_server("OPER %s %s", nick, password);
	if (get_int_var(AUTO_REOPER_VAR))
	{
		set_server_operator_nick(from_server, nick);
		set_server_operator_pwd(from_server, password);
	}
}

/* Full scale abort.  Does a "save" into the filename in line, and
        then does a coredump */
static  void   
abortcmd(command, args, subargs)
	char    *command,
		*args,
		*subargs;
{
        char    *filename = next_arg(args, &args);

        filename = filename ? filename : "irc.aborted";
	save_which = SFLAG_ALL;
        really_save(filename, "y");
#ifdef ALLOC_DEBUG
        alloc_cmd("ALLOC","d");
#endif
        abort();
}
        
/* This generates a file of your ircII setup */
static	void
really_save(ircrc_file, line)
	char	*ircrc_file;
	char	*line;
{
	FILE	*fp;
	int	save_do_all = 0;

	if (*line != 'y' && *line != 'Y')
		return;
	if ((fp = fopen(ircrc_file, "w")) != NULL)
	{
		if (save_which & SFLAG_BIND)
			save_bindings(fp, save_do_all);
		if (save_which & SFLAG_ON)
			save_hooks(fp, save_do_all);
		if (save_which & SFLAG_NOTIFY)
			save_notify(fp);
		if (save_which & SFLAG_SET)
			save_variables(fp, save_do_all);
		if (save_which & SFLAG_ALIAS)
			save_aliases(fp, save_do_all);
		if (save_which & SFLAG_DIGRAPH)
			save_digraphs(fp);
		if (save_which & SFLAG_AUTOOP)
			save_autoops(fp);
		if (save_which & SFLAG_FORMAT)
			save_formats(fp);
		fclose(fp);
		say("IRCII settings saved to %s", ircrc_file);
	}
	else
		say("Error opening %s: %s", ircrc_file, strerror(errno));
}

/* save_settings: saves the current state of IRCII to a file */
/*ARGSUSED*/
static	void
save_settings(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg;
	int	all = 1;

	save_which = save_do_all = 0;
	while ((arg = next_arg(args, &args)) != NULL)
	{
		if ('-' == *arg)
		{
			all = 0;
			arg++;
			if (0 == my_strnicmp("ALIAS", arg, 5))
				save_which |= SFLAG_ALIAS;
			else if (0 == my_strnicmp("ASSIGN", arg, 6))
				save_which |= SFLAG_ALIAS;
			else if (0 == my_strnicmp("AUTOOP", arg, 6))
				save_which |= SFLAG_AUTOOP;
			else if (0 == my_strnicmp("BIND", arg, 4))
				save_which |= SFLAG_BIND;
			else if (0 == my_strnicmp("FORMAT", arg, 6))
				save_which |= SFLAG_FORMAT;
			else if (0 == my_strnicmp("ON", arg, 2))
				save_which |= SFLAG_ON;
			else if (0 == my_strnicmp("SET", arg, 3))
				save_which |= SFLAG_SET;
			else if (0 == my_strnicmp("NOTIFY", arg, 6))
				save_which |= SFLAG_NOTIFY;
			else if (0 == my_strnicmp("DIGRAPH", arg, 7))
				save_which |= SFLAG_DIGRAPH;
			else if (0 == my_strnicmp("ALL", arg, 7))
				save_do_all = 1;
			else
			{
				say("%s: unknown argument", arg);
				return;
			}
			continue;
		}
#ifdef DAEMON_UID
		if (getuid() == DAEMON_UID)
		{
			say("You may only use the default value");
			return;
		}
#endif /* DAEMON_UID */
		if (!(ircrc_file = expand_twiddle(arg)))
		{
			say("Unknown user");
			return;
		}
	}
	if (all)
		save_which = SFLAG_ALL;

	if (dumb)
		really_save(ircrc_file, "y"); /* REAL dumb!  -lynx */
	else
	{
		sprintf(buffer, "Really write %s? ", ircrc_file);
		add_wait_prompt(buffer, really_save, ircrc_file,
				WAIT_PROMPT_LINE);
	}
}

/*
 * e_channel: does the channel command.  I just added displaying your current
 * channel if none is given 
 */
static	void
e_channel(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*chan;
	char	*old;
	int	len, j = 0;

	if (get_server_version(from_server) == Server2_5)
		command = "CHANNEL";
	message_from(NULL, LOG_CURRENT);
	if ((chan = next_arg(args, &args)) != NULL)
	{
		len = strlen(chan);
		if (my_strnicmp(chan, "-invite", len) == 0)
		{
			if (invite_channel)
				send_to_server("%s %s",command, invite_channel);
			else
				say("You have not been invited to a channel!");
		}
		else
		{
			if (!strcmp(command, "J")) {
				command = "JOIN";
				if (!is_channel(chan)) {
				  old = chan;
				  chan = (char *) 0;
				  chan = new_malloc(strlen(old)+1);
				  sprintf(chan, "#%s", old);
				  j = 1;
				}
			}
			if (is_on_channel(chan, get_server_nickname(from_server)))
			{
				if (is_bound(chan, curr_scr_win->server))
				{
					say("Channel %s is bound", chan);
				}
				else
				{
					is_current_channel(chan, 1);
					set_last_current_channel(0, chan);
					say("You are now talking to channel %s", set_channel_by_refnum(0, chan));
					update_all_windows();
				}
			}
			else
			{
				/* only do this if we're actually joining a new channel */
				if (get_int_var(NOVICE_VAR))
				{
					if ((old = get_channel_by_refnum(0)) && strcmp(old, "0"))
						send_to_server("PART %s", old);
				}
				send_to_server("%s %s %s", command, chan, args);
				if (!is_bound(chan, curr_scr_win->server))
					malloc_strcpy(&curr_scr_win->waiting_channel, chan);
			}
		}
	}
	else
		list_channels();
	if (j)
	  new_free(&chan);
	message_from(NULL, LOG_CRAP);
}

/* comment: does the /COMMENT command, useful in .ircrc */
/*ARGSUSED*/
static	void
comment(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	/* nothing to do... */
}

/*
 * e_nick: does the /NICK command.  Records the users current nickname and
 * sends the command on to the server 
 */
/*ARGSUSED*/
static	void
e_nick(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*nick;

	if (!(nick = next_arg(args, &args)))
	{
		say("Your nickname is %s",
			get_server_nickname(get_window_server(0)));
		return;
	}
	if ((nick = check_nickname(nick)) != NULL)
	{
		send_to_server("NICK %s", nick);
		if (attempting_to_connect)
			set_server_nickname(get_window_server(0),nick);
		if (get_server_version(from_server) == Server2_5)
			add_to_whois_queue(nick, whois_nickname,
				NULL);
	}
	else
		say("Bad nickname");
}

/* version: does the /VERSION command with some IRCII version stuff */
static	void
version(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*host;

	if ((host = next_arg(args, &args)) != NULL)
		send_to_server("%s %s", command, host);
	else
	{ 
		say("Client: Blackened %s (internal version %s)", irc_version, internal_version);
		send_to_server("%s", command);
	}
}

/*
 * info: does the /INFO command.  I just added some credits
 * I updated most of the text -phone, feb 1993.
 */
static	void
info(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (!args || !*args)
	{
		say("Blackened IRC: written by Timothy L. Jensen");
		say("       E-Mail: toast@blackened.com  IRC: Toast");
		say("       Copyright (C) 1999-2001");
		say("       Based on ircii-2.8.2.  See below for details.");
		say("");
		say("ircii: originally written by Michael Sandrof");
		say("       versions 2.1 to 2.2pre7 by Troy Rollo");
		say("       development continued by matthew green");
		say("       e-mail: mrg@mame.mu.oz.au  irc: phone");
		say("       copyright (c) 1990-1994");
		say("       do a /help ircii copyright for the full copyright");
		say("       ircii includes software developed by the university");
		say("       of california, berkeley and its contributors");
		say("");
		say("major 2.6 helpers:");
		say("       \tNicolas Pioch        <poich@poly.polytechnique.fr>");
		say("       \tIan Frechette        <frechett@spot.colorado.edu>");
		say("       \tChris Williams       <cgw@io.com>");
		say("       \tJeff Grills          <jefftep@cs.utexas.edu>");
		say("");
		say("ircii contributors");
		say("");
		say("       \tMichael Sandrof       Mark T. Dameu");
		say("       \tStellan Klebom        Carl v. Loesch");
		say("       \tTroy Rollo            Martin  Friedrich");
		say("       \tMichael Weber         Bill Wisner");
		say("       \tRiccardo Facchetti    Stephen van den Berg");
		say("       \tVolker Paulsen        Kare Pettersson");
		say("       \tIan Frechette         Charles Hannum");
		say("       \tmatthew green         Christopher Williams");
		say("       \tJonathan Lemon        Brian Koehmstedt");
		say("       \tNicolas Pioch         Brian Fehdrau");
		say("       \tDarren Reed           Jeff Grills");
		say("       \tJeremy Nelson");
	}
	send_to_server("%s %s", command, args);
}

extern	void
ison_ison(notused, nicklist)
	char	*notused;
	char	*nicklist;
{
	if (do_hook(current_numeric, "%s", nicklist))
		put_it("%sCurrently online: %s", numeric_banner(), nicklist);
}

static	void
ison(command, args, subargs)
	char	*command;
	char	*args,
	*subargs;
{
	if (!args[strspn(args, " ")])
		args = get_server_nickname(from_server);
	add_ison_to_whois(args, ison_ison);

}

/*
 * userhost: Does the USERHOST command.  Need to split up the queries,
 * since the server will only reply to 5 at a time.
 */
static	void
userhost(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	n = 0,
		total = 0,
		userhost_cmd = 0;
	char	*nick;

	while ((nick = next_arg(args, &args)) != NULL)
	{
		int	len;

		++total;
		len = strlen(nick);
		if (!my_strnicmp(nick, "-CMD", len))
		{
			if (total < 2)
			{
				yell("userhost -cmd with no nick!");
				return;
			}
			userhost_cmd = 1;
			break;
		}
		else
		{
			if (n++)
				strmcat(buffer, " ", BIG_BUFFER_SIZE);
			else
				*buffer = '\0';
			strmcat(buffer, nick, BIG_BUFFER_SIZE);
		}
	}
	if (n)
	{
		char	*the_list = (char *) 0;
		char	*s, *t;
		int	i;

		malloc_strcpy(&the_list, buffer);
		s = t = the_list;
		while (n)
		{
			for (i = 5; i && *s; s++)
				if (' ' == *s)
					i--, n--;
			if (' ' == *(s - 1))
				*(s - 1) = '\0';
			else
				n--;
			strcpy(buffer, t);
			t = s;

			if (userhost_cmd)
				add_to_whois_queue(buffer, userhost_cmd_returned, "%s", args);
			else
				add_to_whois_queue(buffer, USERHOST_USERHOST, 0);
		}
		new_free(&the_list);
	}
	else if (!total)
		/* Default to yourself.  */
		add_to_whois_queue(get_server_nickname(from_server), USERHOST_USERHOST, 0);
}

/*
 * whois: the WHOIS and WHOWAS commands.  This translates the 
 * to the whois handlers in whois.c 
 */
void
whois(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (args && *args)
		send_to_server("%s %s", command, args);
	else /* Default to yourself  -lynx */
		send_to_server("%s %s", command, get_server_nickname(from_server));
}

/*
 * who: the /WHO command. Parses the who switches and sets the who_mask and
 * whoo_stuff accordingly.  Who_mask and who_stuff are used in whoreply() in
 * parse.c 
 */
void
who(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg,
		*channel = NULL;
	int	no_args = 1,
		len;

	who_mask = 0;
	new_free(&who_name);
	new_free(&who_host);
	new_free(&who_server);
	new_free(&who_file);
	new_free(&who_nick);
	new_free(&who_real);
	while ((arg = next_arg(args, &args)) != NULL)
	{
		no_args = 0;
		if ((*arg == '-') && (!isdigit(*(arg + 1))))
		{
			arg++;
			if ((len = strlen(arg)) == 0)
			{
				say("Unknown or missing flag");
				return;
			}
			if (my_strnicmp(arg, "operators", len) == 0)
				who_mask |= WHO_OPS;
			else if (my_strnicmp(arg, "lusers", len) == 0)
				who_mask |= WHO_LUSERS;
			else if (my_strnicmp(arg, "chops", len) == 0)
				who_mask |= WHO_CHOPS;
			else if (my_strnicmp(arg, "hosts", len) == 0)
			{
				if ((arg = next_arg(args, &args)) != NULL)
				{
					who_mask |= WHO_HOST;
					malloc_strcpy(&who_host, arg);
					channel = who_host;
				}
				else
				{
					say("%s -HOSTS: missing arguement", command);
					return;
				}
			}
			else if (my_strnicmp(arg, "here", len) ==0)
				who_mask |= WHO_HERE;
			else if (my_strnicmp(arg, "away", len) ==0)
				who_mask |= WHO_AWAY;
			else if (my_strnicmp(arg, "servers", len) == 0)
			{
				if ((arg = next_arg(args, &args)) != NULL)
				{
					who_mask |= WHO_SERVER;
					malloc_strcpy(&who_server, arg);
					channel = who_server;
				}
				else
				{
					say("%s -SERVERS: missing arguement", command);
					return;
				}
			}
			else if (my_strnicmp(arg, "name", len) == 0)
			{
				if ((arg = next_arg(args, &args)) != NULL)
				{
					who_mask |= WHO_NAME;
					malloc_strcpy(&who_name, arg);
					channel = who_name;
				}
				else
				{
					say("%s -NAME: missing arguement", command);
					return;
				}
			}
			else if (my_strnicmp(arg, "realname", len) == 0)
			{
				if ((arg = next_arg(args, &args)) != NULL)
				{
					who_mask |= WHO_REAL;
					malloc_strcpy(&who_real, arg);
					channel = who_real;
				}
				else
				{
					say("%s -REALNAME: missing arguement", command);
					return;
				}
			}
			else if (my_strnicmp(arg, "nick", len) == 0)
			{
				if ((arg = next_arg(args, &args)) != NULL)
				{
					who_mask |= WHO_NICK;
					malloc_strcpy(&who_nick, arg);
					channel = who_nick;
				}
				else
				{
					say("%s -NICK: missing arguement", command);
					return;
				}
				/* WHO -FILE by Martin 'Efchen' Friedrich */
			}
			else if (my_strnicmp(arg, "file", len) == 0)
			{
				who_mask |= WHO_FILE;
				if ((arg = next_arg(args, &args)) != NULL)
				{
					malloc_strcpy(&who_file, arg);
				}
				else
				{
					say("%s -FILE: missing arguement", command);
					return;
				}
			}
			else
			{
				say("Unknown or missing flag");
				return;
			}
		}
		else 
		{
			if (strcmp(arg, "*") == 0)
			{
				channel = get_channel_by_refnum(0);
				if (!channel || *channel == '0')
				{
					say("I wouldn't do that if I were you");
					return;
				}
			}
			else
				channel = arg;
		}
	}
	if (no_args) {
		if (!strcmp(command, "W")) {
			channel = get_channel_by_refnum(0);
			if (!channel || *channel == '0')
			{
				say("I wouldn't do that if I were you");
				return;
			}
		} else {
			say("No argument specified");
			return;
		}
	}
	if (!channel && who_mask & WHO_OPS)
		channel = "*";
	send_to_server("WHO %s %c", channel ? channel : empty_string,
				(who_mask & WHO_OPS) ?  'o' : '\0');
}

/*
 * query: the /QUERY command.  Works much like the /MSG, I'll let you figure
 * it out.
 */
/*ARGSUSED*/
void
query(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*nick,
		*rest;

	message_from(NULL, LOG_CURRENT);
	if ((nick = next_arg(args, &rest)) != NULL)
	{
		if (strcmp(nick, ".") == 0)
		{
			if (!(nick = sent_nick))
			{
				say("You have not messaged anyone yet");
				return;
			}
		}
		else if (strcmp(nick, ",") == 0)
		{
			if (!(nick = recv_nick))
			{
				say("You have not recieved a message from \
						anyone yet");
				return;
			}
		}
		else if (strcmp(nick, "*") == 0)
			if (!(nick = get_channel_by_refnum(0)))
			{
				say("You are not on a channel");
				return;
			}

		if (*nick == '%')
		{
			if (is_process(nick) == 0)
			{
				say("Invalid processes specification");
				message_from(NULL, LOG_CRAP);
				return;
			}
		}
		say("Starting conversation with %s", nick);
		set_query_nick(nick);
	}
	else
	{
		if (query_nick())
		{
			say("Ending conversation with %s", query_nick());
			set_query_nick(NULL);
		}
		else
			say("You aren't querying anyone!");
	}
	update_input(UPDATE_ALL);
	message_from(NULL, LOG_CRAP);
}

/*
 * away: the /AWAY command.  Keeps track of the away message locally, and
 * sends the command on to the server.
 */
static	void
away(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	len;
	char	*arg = NULL;
	int	flag = AWAY_ONE;
	int	i;
	char	gonetext[512];

	if (*args)
	{
		if (*args == '-')
		{
			arg = index(args, ' ');
			if (arg)
				*arg++ = '\0';
			else
				arg = "";
			len = strlen(args);
			if (0 == my_strnicmp(args, "-ALL", len))
			{
				flag = AWAY_ALL;
				args = arg;
			}
			else if (0 == my_strnicmp(args, "-ONE", len))
			{
				flag = AWAY_ONE;
				args = arg;
			}
			else
			{
				say("%s: %s unknown flag", command, args);
				return;
			}
		}
	}
	if (command && !my_stricmp(command, "GONE")) {
		if (args && *args) {
			strmcpy(&gonetext, parseformat(GONE_FMT, args), 511);
			args = (char *)&gonetext;
		}
		command = "AWAY";
	}
	if (flag == AWAY_ALL)
		if (*args)
		{
			away_set = 1;
			MarkAllAway(command, args);
		}
		else
		{
			away_set = 0;
			for(i = 0; (i < number_of_servers); i++)
				if (server_list[i].whois_stuff.away)
					new_free(&(server_list[i].away));
		}
	else
	{
		send_to_server("%s :%s", command, args);
		if (*args)
		{
			away_set = 1;
			malloc_strcpy(&(server_list[
				curr_scr_win->server].away), args);
		}
		else
		{
			new_free(&(server_list[
				curr_scr_win->server].away));
			away_set = 0;
			for(i = 0; (i < number_of_servers) && !away_set ; i++)
				if (server_list[i].read != -1 &&
						server_list[i].away)
					away_set = 1;
		}
	}
	update_all_status();
}

/* e_quit: The /QUIT, /EXIT, etc command */
/*ARGSUSED*/
static	void
e_quit(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	max;
	int	i;
	char	*Reason;

	/*Reason = ((args && *args) ? args : "Real programmers code it into the client");*/
	if (args && *args)
	  Reason = args;
	else {
	  switch (getrandom(1,8)) {
	  case 1:
	    Reason = "Real programmers code it into the client";
	    break;
	  case 2:
	    Reason = "BitchX sucks";
	    break;
	  case 3:
	    Reason = "What's a script?";
	    break;
	  case 4:
	    Reason = "*monkey grin*";
	    break;
	  case 5:
	    Reason = "\026\002 * Blackened * \002\026";
	    break;
	  case 6:
	    Reason = "Real IRC clients know \"to\" is a preposition, not an adverb";
	    break;
	  case 7:
	    Reason = "http://www.blackened.com/blackened/";
	    break;
	  default:
	    Reason = get_reason();
	  }
	}
	max = server_list_size();
	for (i = 0; i < max; i++)
		if (is_server_connected(i))
		{
			from_server = i;
			send_to_server("QUIT :%s", Reason);
			put_it("%s", parseformat(QUIT_FMT, get_server_nickname(get_window_server(0)), Reason));
		}
	irc_exit();
}

/* flush: flushes all pending stuff coming from the server */
/*ARGSUSED*/
static	void
flush(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (get_int_var(HOLD_MODE_VAR))
	{
		while (curr_scr_win->held_lines)
			remove_from_hold_list(curr_scr_win);
		hold_mode((Window *) 0, OFF, 1);
	}
	say("Standby, Flushing server output...");
	flush_server();
	say("Done");
}

/* e_wall: used for WALL and WALLOPS */
static	void
e_wall(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int level;

	if (!in_on_who)
		send_to_server("%s :%s", command, args);
	if (get_int_var(SHOW_MY_WALLOPS_VAR)) {
		message_from(NULL, LOG_WALLOP);
		level = set_lastlog_msg_level(LOG_WALLOP);
		if (!strcasecmp(command, "OPERWALL"))
			put_it("%s", parseformat(SEND_OPERWALL_FMT,
				get_server_nickname(get_window_server(0)), args,
				get_server_operator(get_window_server(0)) ? "*"
				: empty_string));
		else if (!strcasecmp(command, "LOCOPS"))
			put_it("%s", parseformat(SEND_LOCOPS_FMT,
				get_server_nickname(get_window_server(0)), args,
				get_server_operator(get_window_server(0)) ? "*"
				: empty_string));
		else
			put_it("%s", parseformat(SEND_WALLOPS_FMT,
				get_server_nickname(get_window_server(0)), args,
				get_server_operator(get_window_server(0)) ? "*"
				: empty_string));
		if (beep_on_level & LOG_WALLOP)
			beep_em(1);
		set_lastlog_msg_level(level);
		message_from(NULL, LOG_CRAP);
	}
}

/*
 * e_privmsg: The MSG command, displaying a message on the screen indicating
 * the message was sent.  Also, this works for the NOTICE command. 
 */
static	void
e_privmsg(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*nick;

	if ((nick = next_arg(args, &args)) != NULL)
	{
		if (strcmp(nick, ".") == 0)
		{
			if (!(nick = sent_nick))
			{
				say("You have not sent a message to anyone yet");
				return;
			}
		}

		else if (strcmp(nick, ",") == 0)
		{
			if (!(nick = recv_nick))
			{
				say("You have not received a message from anyone yet");
				return;
			}
		}
		else if (!strcmp(nick, "*"))
			if (!(nick = get_channel_by_refnum(0)))
				nick = "0";
		send_text(nick, args, command);
	}
	else
		say("You must specify a nickname or channel!");
}

/*
 * rel_msg: The RELM command, redirects the last received message.
 *
 * added by Tim "Toast" Jensen
 */
static  void
rel_msg(command, args, subargs)
        char    *command,
                *args,
                *subargs;
{
	char	*nick;
	char	*last_msg;

	if ((nick = next_arg(args, &args)) != NULL)
	{
		if (strcmp(nick, ".") == 0)
		{
			if (!(nick = sent_nick))
			{
				say("You have not sent a message to anyone yet");
				return;
			}
		}

		else if (strcmp(nick, ",") == 0)
		{
			if (!(nick = recv_nick))
			{
				say("You have not received a message from anyone yet");
				return;
			}
		}
		else if (!strcmp(nick, "*"))
			if (!(nick = get_channel_by_refnum(0)))
				nick = "0";
	}
	else
		nick = get_target_by_refnum(0);
	if (strcmp(command, "RELM") == 0)
		last_msg = get_string_var(LAST_MSG_VAR);
	else if (strcmp(command, "RELS") == 0)
		last_msg = get_string_var(LAST_SENDMSG_VAR);
	else if (strcmp(command, "RELN") == 0)
		last_msg = get_string_var(LAST_NOTICE_VAR);
	else if (strcmp(command, "RELSN") == 0)
		last_msg = get_string_var(LAST_SENDNOTICE_VAR);
	else
		last_msg = NULL;
	if (last_msg != NULL)
		send_text(nick, last_msg, "PRIVMSG");
	else
		say("Nothing to send!");
}

/*
 * quote: handles the QUOTE command.  args are a direct server command which
 * is simply send directly to the server 
 */
/*ARGSUSED*/
static	void
quote(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (!in_on_who)
		send_to_server("%s", args);
}

/* clear: the CLEAR command.  Figure it out */
/*ARGSUSED*/
static	void
clear(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg;
	int	all = 0,
		unhold = 0;

	while ((arg = next_arg(args, &args)) != NULL)
	{
	/* -ALL and ALL here becuase the help files used to be wrong */
		if (!my_strnicmp(arg, "ALL", strlen(arg))
				|| !my_strnicmp(arg, "-ALL", strlen(arg)))
			all = 1;
		else if (!my_strnicmp(arg, "-UNHOLD", strlen(arg)))
			unhold = 1;
		else
			say("Unknown flag: %s", arg);
	}
	if (all)
		clear_all_windows(unhold);
	else
	{
		if (unhold)
			hold_mode((Window *) 0, OFF, 1);
		clear_window_by_refnum(0);
	}
	update_input(UPDATE_JUST_CURSOR);
}

/*
 * send_comm: the generic command function.  Uses the full command name found
 * in 'command', combines it with the 'args', and sends it to the server 
 */
static	void
send_comm(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (args && *args)
		send_to_server("%s %s", command, args);
	else
		send_to_server("%s", command);
}


void
send_topic(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg;
	char	*arg2;

	if ((arg = next_arg(args, &args)) != NULL)
	{
		if (!strcmp("*", arg))
			arg = get_channel_by_refnum(0);
		/* Client segfaults if not in a channel and * is used -TLJ */
		if (!arg)
			say("You can't set the topic if you aren't in a channel!");
		else
		  if (is_channel(arg))
		  {
			if ((arg2 = next_arg(args, &args)) != NULL)
				send_to_server("%s %s :%s %s", command, arg,
						arg2, args);
			else
				send_to_server("%s %s", command, arg);
		  }
		  else
			send_to_server("%s :%s %s", command, arg, args);
	}
	else
		send_to_server("%s", command);
}

static	void
send_2comm(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*reason;

	if ((reason = index(args, ' ')) != NULL)
		*reason++ = '\0';
	else
		reason = empty_string;

	if (reason && *reason)
		send_to_server("%s %s :%s", command, args, reason);
	else
		send_to_server("%s %s", command, args);
}

/*
 * send_kick: sends a kick message to the server.  Works properly with
 * kick comments.
 */

static	void
send_kick(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*kickee,
		*comment;

	if ((kickee = index(args, ' ')) != NULL)
		*(kickee++) = (char) 0;
	else 
		kickee = index(args, (char) 0);

	if ((comment = index(kickee, ' ')) != NULL)
		*(comment++) = (char) 0;
	else 
		comment = index(kickee, (char) 0);

	if (strcmp(args, "*") == 0)
		send_to_server("%s %s %s :%s", command,
				get_channel_by_refnum(0), kickee, comment);
	else
		send_to_server("%s %s %s :%s", command, args, kickee, comment);
}

/*
 * send_channel_com: does the same as send_com for command where the first
 * argument is a channel name.  If the first argument is *, it is expanded
 * to the current channel (a la /WHO).
 */
void
send_channel_com(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*ptr,
		*s;

	if ((ptr = index(args, ' ')) != NULL)
		*(ptr++) = (char) 0;
	else
		ptr = index(args, (char) 0);

	if (!strcmp(args, "*"))
	{
		if ((s = get_channel_by_refnum(0)) != NULL)
			send_to_server("%s %s %s", command, s, ptr);
		else
			say("You aren't on a channel in this window");
	}
	else
		send_to_server("%s %s %s", command, args, ptr);
}

/*
 * send_text: Sends the line of text to whomever the user is currently
 * talking.  If they are quering someone, it goes to that user, otherwise
 * it's the current channel.  Some tricky parameter business going on. If
 * nick is null (as if you had typed "/ testing" on the command line) the
 * line is sent to your channel, and the command parameter is never used. If
 * nick is not null, and command is null, the line is sent as a PRIVMSG.  If
 * nick is not null and command is not null, the message is sent using
 * command.  Currently, only NOTICEs and PRIVMSGS work. 
 * fixed to not be anal about "/msg foo,bar foobar" -phone
 */
void
send_text(org_nick, line, command)
	char	*org_nick;
	char	*line;
	char	*command;
{
	char	*key,
		*ptr,
		*free_nick,
		*nick = NULL;
	char    *timestr;
	char    *junk;
	int	lastlog_level,
		list_type,
		old_server;
	int	check_away;
	char	the_thing, the_sthing[128];
	char	*query_command = NULL;
	char	nick_list[IRCD_BUFFER_SIZE];
	char	comtemp[3];
	int	do_final_send = 0;

	*nick_list = '\0';
	malloc_strcpy(&nick, org_nick);
	free_nick = ptr = nick;
	timestr = TimeStamp();
	while ((nick = ptr) != NULL)
	{
		if ((ptr = index(nick, ',')) != NULL)
			*(ptr++) = (char) 0;
		if (!*nick)
			continue;
		set_oops(nick, line, command);
		if (is_process(nick))
		{
			int	i;

			if ((i = get_process_index(&nick)) == -1)
				say("Invalid process specification");
			else
				text_to_process(i, line, 1);
			continue;
		}
		if (!*line)
			continue; /* lynx */
		if (in_on_who)
		{
			say("You may not send messages from ON WHO, ON WHOIS, or ON JOIN");
			continue;
		}
		if (doing_privmsg)
			command	= "NOTICE";
		/* Query quote -lynx */
		if (strcmp(nick, "\"") == 0) /* quote */
		{
			send_to_server("%s", line);
			continue;
		}
		if (*nick == '=') /* DCC chat */
		{
			addnick(nick);
			old_server = from_server;
			from_server = -1;
			dcc_chat_transmit(nick + 1, line);
			from_server = old_server;
			continue;
		}
		if (*nick == '@') /* DCC talk */
		{
			old_server = from_server;
			from_server = -1;
			dcc_message_transmit(nick + 1, line, DCC_TALK, 1);
			from_server = old_server;
			continue;
		}
		if (*nick == '/') /* Command */
		{
			malloc_strcpy(&query_command, nick);
			malloc_strcat(&query_command, " ");
			malloc_strcat(&query_command, line);
			parse_command(query_command, 0, empty_string);
			new_free(&query_command);
			continue;
		}
		switch (send_text_flag)
		{
		case MSG_LIST:
			command = "NOTICE";
			break;
		case NOTICE_LIST:
			say("You cannot send a message from a NOTICE");
			new_free(&free_nick);
			return;
		}
		if (is_channel(nick))
		{
			int	current;

			current = is_current_channel(nick, 0);
			if (!command || strcmp(command, "NOTICE"))
			{
				command = "PRIVMSG";
				lastlog_level = set_lastlog_msg_level(LOG_PUBLIC);
				message_from(nick, LOG_PUBLIC);
				list_type = SEND_PUBLIC_LIST;
				the_thing = '>';
			}
			else
			{
				lastlog_level = set_lastlog_msg_level(LOG_NOTICE);
				message_from(nick, LOG_NOTICE);
				list_type = SEND_NOTICE_LIST;
				the_thing = '-';
			}
			if (do_hook(list_type, "%s %s", nick, line))
			{
			    if (list_type == SEND_PUBLIC_LIST) {
				if (current) {
					int rn = get_refnum_by_channel(nick);
					if (!is_last_current_channel(rn, nick)) {
					  set_last_current_channel(rn, nick);
					  say("You are now talking to channel %s", nick);
					}
					put_it("%s", parseformat(SEND_PUBLIC_FMT,
						get_server_nickname(get_window_server(0)),
						nick, line));
				} else
					if (is_on_channel(nick, get_server_nickname(get_window_server(0))))
					  put_it("%s", parseformat(SEND_PUBLIC_OTHER_FMT,
					    get_server_nickname(get_window_server(0)),
					    nick, line));
					else
					  put_it("%s", parseformat(SEND_PUBLIC_MSG_FMT,
					    get_server_nickname(get_window_server(0)),
					    nick, line));
			    } else
				put_it("%s", parseformat(SEND_NOTICE_FMT,
					get_server_nickname(get_window_server(0)), nick,
					line));
			}
			set_lastlog_msg_level(lastlog_level);
			if ((key = is_crypted(nick)) != 0)
			{
				char	*crypt_line;

				if ((crypt_line = crypt_msg(line, key, 1)))
					send_to_server("%s %s :%s", command, nick, crypt_line);
				new_free(&crypt_line);
				continue;
			}
			if (!in_on_who)
			{
				if (*nick_list)
				{
					strcat(nick_list, ",");
					strcat(nick_list, nick);
				}
				else
					strcpy(nick_list, nick);
			}
			do_final_send = 1;
		}
		else
		{
			if (!command || strcmp(command, "NOTICE"))
			{
				lastlog_level = set_lastlog_msg_level(LOG_MSG);
				command = "PRIVMSG";
				message_from(nick, LOG_MSG);
				list_type = SEND_MSG_LIST;
				the_thing = '*';
				addnick(nick);
			}
			else
			{
				lastlog_level = set_lastlog_msg_level(LOG_NOTICE);
				message_from(nick, LOG_NOTICE);
				list_type = SEND_NOTICE_LIST;
				the_thing = '-';
			}
			comtemp[0] = the_thing;
			comtemp[1] = '\0';
			logmsg(1, comtemp, nick, line, NULL, 0);
			if (window_display && do_hook(list_type, "%s %s", nick, line)) {
				if (list_type == SEND_MSG_LIST)
					junk = parseformat(SEND_MSG_FMT,
						get_server_nickname(get_window_server(0)),
						nick, line);
				else
					junk = parseformat(SEND_NOTICE_FMT,
						get_server_nickname(get_window_server(0)),
						nick, line);
				put_it("%s", junk);
				if (list_type == SEND_MSG_LIST)
				  	set_string_var(LAST_SENDMSG_VAR, junk);
				else
					set_string_var(LAST_SENDNOTICE_VAR, junk);
				  
			}
			if ((key = is_crypted(nick)) != NULL)
			{
				char	*crypt_line;

				if ((crypt_line = crypt_msg(line, key, 1)))
					send_to_server("%s %s :%s", command ? command : "PRIVMSG", nick, crypt_line);
				new_free(&crypt_line);
				continue;
			}
			set_lastlog_msg_level(lastlog_level);

			if (!in_on_who)
			{
				if (*nick_list)
				{
					strcat(nick_list, ",");
					strcat(nick_list, nick);
				}
				else
					strcpy(nick_list, nick);
			}

			if (get_int_var(WARN_OF_IGNORES_VAR) && (is_ignored(nick, IGNORE_MSGS) == IGNORED))
				say("Warning: You are ignoring private messages from %s", nick);

			malloc_strcpy(&sent_nick, nick);
			if (check_away && server_list[curr_scr_win->server].away && get_int_var(AUTO_UNMARK_AWAY_VAR))
			{
				check_away = 1;
				away("AWAY", empty_string, empty_string);
			}
			do_final_send = 1;
		}
	}

	malloc_strcpy(&sent_body, line);
	if (do_final_send)
		send_to_server("%s %s :%s", command ? command : "PRIVMSG", nick_list, line);
	message_from(NULL, LOG_CRAP);
	new_free(&free_nick);
}

static void
do_send_text(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*tmp;

	if (command)
		tmp = get_channel_by_refnum(0);
	else
		tmp = get_target_by_refnum(0);
	send_text(tmp, args, NULL);
}

/*
 * command_completion: builds lists of commands and aliases that match the
 * given command and displays them for the user's lookseeing 
 */
void
command_completion()
{
	int	do_aliases;
	int	cmd_cnt,
		alias_cnt,
		i,
		c,
		len;
	char	**aliases = NULL;
	char	*line = NULL,
		*com,
		*cmdchars,
		*rest,
		firstcmdchar = '/';
	IrcCommand	*command;

	malloc_strcpy(&line, get_input());
	if ((com = next_arg(line, &rest)) != NULL)
	{
		if (!(cmdchars = get_string_var(CMDCHARS_VAR)))
			cmdchars = DEFAULT_CMDCHARS;
		if (index(cmdchars, *com))
		{
			firstcmdchar = *cmdchars;
			com++;
			if (*com && index(cmdchars, *com))
			{
				do_aliases = 0;
				alias_cnt = 0;
				com++;
			}
			else
				do_aliases = 1;
			upper(com);
			if (do_aliases)
				aliases = match_alias(com, &alias_cnt,
					COMMAND_ALIAS);
			if ((command = find_command(com, &cmd_cnt)) != NULL)
			{
				if (cmd_cnt < 0)
					cmd_cnt *= -1;
				/* special case for the empty string */

				if (*(command[0].name) == (char) 0)
				{
					command++;
					cmd_cnt = NUMBER_OF_COMMANDS;
				}
			}
			if ((alias_cnt == 1) && (cmd_cnt == 0))
			{
				sprintf(buffer, "%c%s %s", firstcmdchar,
					aliases[0], rest);
				set_input(buffer);
				new_free(&(aliases[0]));
				new_free(&aliases);
				update_input(UPDATE_ALL);
			}
			else if (((cmd_cnt == 1) && (alias_cnt == 0)) ||
			    ((cmd_cnt == 1) && (alias_cnt == 1) &&
			    (strcmp(aliases[0], command[0].name) == 0)))
			{
				sprintf(buffer, "%c%s%s %s", firstcmdchar,
					do_aliases ? "" : &firstcmdchar,
					command[0].name, rest);
				set_input(buffer);
				update_input(UPDATE_ALL);
			}
			else
			{
				*buffer = (char) 0;
				if (command)
				{
					say("Commands:");
					strmcpy(buffer, "\t", BIG_BUFFER_SIZE);
					c = 0;
					for (i = 0; i < cmd_cnt; i++)
					{
						strmcat(buffer, command[i].name,
							BIG_BUFFER_SIZE);
						for (len =
						    strlen(command[i].name);
						    len < 15; len++)
							strmcat(buffer, " ",
							    BIG_BUFFER_SIZE);
						if (++c == 4)
						{
							say("%s", buffer);
							strmcpy(buffer, "\t",
							    BIG_BUFFER_SIZE);
							c = 0;
						}
					}
					if (c)
						say("%s", buffer);
				}
				if (aliases)
				{
					say("Aliases:");
					strmcpy(buffer, "\t", BIG_BUFFER_SIZE);
					c = 0;
					for (i = 0; i < alias_cnt; i++)
					{
						strmcat(buffer, aliases[i],
							BIG_BUFFER_SIZE);
						for (len = strlen(aliases[i]);
								len < 15; len++)
							strmcat(buffer, " ",
							    BIG_BUFFER_SIZE);
						if (++c == 4)
						{
							say("%s", buffer);
							strmcpy(buffer, "\t",
							    BIG_BUFFER_SIZE);
							c = 0;
						}
						new_free(&(aliases[i]));
					}
					if (strlen(buffer) > 1)
						say("%s", buffer);
					new_free(&aliases);
				}
				if (!*buffer)
					term_beep();
			}
		}
		else
			term_beep();
	}
	else
		term_beep();
	new_free(&line);
}

/*
 * parse_line: This is the main parsing routine.  It should be called in
 * almost all circumstances over parse_command().
 *
 * parse_line breaks up the line into commands separated by unescaped
 * semicolons if we are in non interactive mode. Otherwise it tries to leave
 * the line untouched.
 *
 * Currently, a carriage return or newline breaks the line into multiple
 * commands too. This is expected to stop at some point when parse_command
 * will check for such things and escape them using the ^P convention.
 * We'll also try to check before we get to this stage and escape them before
 * they become a problem.
 *
 * Other than these two conventions the line is left basically untouched.
 */
void
parse_line(name, org_line, args, hist_flag, append_flag)
	char	*name,
		*org_line,
		*args;
	int	hist_flag,
		append_flag;
{
	char	*line = NULL,
		*free_line,
		*stuff,
		*buffer,
		*s,
		*t;
	int	args_flag;

	malloc_strcpy(&line, org_line);
	free_line = line;
	args_flag = 0;
	if (!*line)
		do_send_text(NULL, empty_string, empty_string);
	else if (args)
		do
		{
			stuff = expand_alias(name, line, args, &args_flag,
					&line);
			if (!line && append_flag && !args_flag && args && *args)
			{
				buffer = (char *) new_malloc(BIG_BUFFER_SIZE+1);
				strmcpy(buffer, stuff, BIG_BUFFER_SIZE);
				strmcat(buffer, " ", BIG_BUFFER_SIZE);
				strmcat(buffer, args, BIG_BUFFER_SIZE);
				new_free(&stuff);
				stuff = buffer;
			}
			parse_command(stuff, hist_flag, args);
			new_free(&stuff);
		}
		while(line);
	else
	{
		if (load_depth)
			parse_command(line, hist_flag, args);
		else
			for (;(s = line);)
			{
				if ((t = sindex(line, "\r\n")) != NULL)
				{
					*t++ = '\0';
					line = t;
				}
				else
					line = NULL;
				parse_command(s, hist_flag, args);
			}
	}
	new_free(&free_line);
	return;
}

/*
 * parse_command: parses a line of input from the user.  If the first
 * character of the line is equal to irc_variable[CMDCHAR_VAR].value, the
 * line is used as an irc command and parsed appropriately.  If the first
 * character is anything else, the line is sent to the current channel or to
 * the current query user.  If hist_flag is true, commands will be added to
 * the command history as appropriate.  Otherwise, parsed commands will not
 * be added. 
 *
 * Parse_command() only parses a single command.  In general, you will want
 * to use parse_line() to execute things.Parse command recognized no quoted
 * characters or anything (beyond those specific for a given command being
 * executed). 
 */
void
parse_command(line, hist_flag, sub_args)
	char	*line;
	int	hist_flag;
	char	*sub_args;
{
	static	unsigned int	 level = 0;
	unsigned int	display,
			old_display_var;
	char	*cmdchars,
		*com,
		*this_cmd = NULL;
	int	args_flag,
		add_to_hist,
		cmdchar_used;

	if (!line || !*line)
		return;
	if (get_int_var(DEBUG_VAR) & DEBUG_COMMANDS)
		yell("Executing [%d] %s", level, line);
	level++;
	if (!(cmdchars = get_string_var(CMDCHARS_VAR)))
		cmdchars = DEFAULT_CMDCHARS;
	malloc_strcpy(&this_cmd, line);
	add_to_hist = 1;
	if (index(cmdchars, *line))
	{
		cmdchar_used = 1;
		com = line + 1;
	}
	else
	{
		cmdchar_used = 0;
		com = line;
	}
	/*
	 * always consider input a command unless we are in interactive mode
	 * and command_mode is off.   -lynx
	 */
	if (hist_flag && !cmdchar_used && !get_int_var(COMMAND_MODE_VAR))
	{
		do_send_text(NULL, line, empty_string);
		if (hist_flag && add_to_hist)
		{
			add_to_history(this_cmd);
			set_input(empty_string);
		}
		/* Special handling for ' and : */
	}
	else if (*com == '\'' && get_int_var(COMMAND_MODE_VAR))
	{
		do_send_text(NULL, line+1, empty_string);
		if (hist_flag && add_to_hist)
		{
			add_to_history(this_cmd);
			set_input(empty_string);
		}
	}
	else if (*com == '@')
	{
		/* This kludge fixes a memory leak */
		char	*tmp;

		tmp = parse_inline(line + 1, sub_args, &args_flag);
		if (tmp)
			new_free(&tmp);
		if (hist_flag && add_to_hist)
		{
			add_to_history(this_cmd);
			set_input(empty_string);
		}
	}
	else
	{
		char	*rest,
			*alias = NULL,
			*alias_name;
		int	cmd_cnt,
			alias_cnt;
		IrcCommand	*command; /* = (IrcCommand *) 0 */

		display = window_display;
		old_display_var = (unsigned) get_int_var(DISPLAY_VAR);
		if ((rest = (char *) index(com, ' ')) != NULL)
			*(rest++) = (char) 0;
		else
			rest = empty_string;
		upper(com);

		/* first, check aliases */
		if (*com && index(cmdchars, *com))
		{
			alias_cnt = 0;
			com++;
			if (*com == '^')
			{
				com++;
				window_display = 0;
			}
		}
		else
		{
			if (*com == '^')
			{
				com++;
				window_display = 0;
			}
			alias = get_alias(COMMAND_ALIAS, com, &alias_cnt,
				&alias_name);
		}
		if (alias && (alias_cnt == 0))
		{
			if (hist_flag && add_to_hist)
			{
				add_to_history(this_cmd);
				set_input(empty_string);
			}
			execute_alias(alias_name, alias, rest);
			new_free(&alias_name);
		}
		else
		{
			/* History */
			if (*com == '!')
			{
				if ((com = do_history(com + 1, rest)) != NULL)
				{
					if (level == 1)
					{
						set_input(com);
						update_input(UPDATE_ALL);
					}
					else
						parse_command(com, 0, sub_args);
					new_free(&com);
				}
				else
					set_input(empty_string);
			}
			else
			{
				if (hist_flag && add_to_hist)
				{
					add_to_history(this_cmd);
					set_input(empty_string);
				}
				command = find_command(com, &cmd_cnt);
				if ((command && cmd_cnt < 0) || (0 == alias_cnt && 1 == cmd_cnt == 1))
				{
					if (command->func)
						command->func(command->server_func, rest, sub_args);
					else
						say("%s: command disabled", command->name);
				}
				else if (alias && 1 == alias_cnt && cmd_cnt == 1 && !strcmp(alias_name, command[0].name))
					execute_alias(alias_name, alias, rest);
				else if ((alias_cnt + cmd_cnt) > 1)
					say("Ambiguous command: %s", com);
				else if (alias && 1 == alias_cnt)
					execute_alias(alias_name, alias, rest);
				else if (!my_stricmp(com, nickname))
						/* nick = /me  -lynx */
					me(NULL, rest, empty_string);
				else
					say("Unknown command: %s", com);
			}
			if (alias)
				new_free(&alias_name);
		}
		if (old_display_var != get_int_var(DISPLAY_VAR))
			window_display = get_int_var(DISPLAY_VAR);
		else
			window_display = display;
	}
	new_free(&this_cmd);
	level--;
}

/*
 * load: the /LOAD command.  Reads the named file, parsing each line as
 * though it were typed in (passes each line to parse_command). 
 */
/*ARGSUSED*/
void
load(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	FILE	*fp;
	char	*filename,
		*expanded = NULL;
	int	flag = 0;
	struct	stat	stat_buf;
	int	paste_level = 0;
	char	*start,
		*current_row = NULL,
		buffer[BIG_BUFFER_SIZE + 1];
	int	no_semicolon = 1;
	char	*irc_path;
	int	display;
#ifdef ZCAT
	char	*temp;
	char	*expand_z = NULL;
	int	exists;
#endif

	irc_path = get_string_var(LOAD_PATH_VAR);
	if (!irc_path)
	{
		say("LOAD_PATH has not been set");
		return;
	}

	if (load_depth == MAX_LOAD_DEPTH)
	{
		say("No more than %d levels of LOADs allowed", MAX_LOAD_DEPTH);
		return;
	}
	load_depth++;
	status_update(0);
#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
	{
		say("You may only load your SAVED file");
		filename = ircrc_file;
	}
	else
#endif /* DAEMON_UID */
		while ((filename = next_arg(args, &args)) != NULL)
		{
			if (my_strnicmp(filename, "-args", strlen(filename)) == 0)
				flag = 1;
			else
				break;
		}
	if (filename)
	{
		if ((expanded = expand_twiddle(filename)) != NULL)
		{
#ifdef ZCAT
			/* Handle both <expanded> and <expanded>.Z */
			temp = &(expanded[strlen(expanded) - strlen(ZSUFFIX)]);
			if (strcmp(temp, ZSUFFIX))
			{
				malloc_strcpy(&expand_z, expanded);
				malloc_strcat(&expand_z, ZSUFFIX);
			}
#endif /*ZCAT*/
			if (*expanded != '/')
			{
				filename = path_search(expanded, irc_path);
#ifdef ZCAT
				if (!filename && expand_z)
					filename = path_search(expand_z, irc_path);
#endif /*ZCAT*/
				if (!filename)
				{
					say("%s: File not found", expanded);
					status_update(1);
					load_depth--;
#ifdef ZCAT
					new_free(&expand_z);
#endif /* ZCAT */
					new_free(&expanded);
					return;
				}
				else
					malloc_strcpy(&expanded, filename);
			}
#ifdef ZCAT
			if ((exists = stat_file(expanded, &stat_buf)) == -1)
				if (!(exists = stat_file(expand_z, &stat_buf)))
					expanded = expand_z;
				else
					new_free(&expand_z);
			if (exists == 0)
#else
				if (!stat_file(expanded, &stat_buf))
#endif /*ZCAT*/
				{
					if (stat_buf.st_mode & S_IFDIR)
					{
						say("%s is a directory",
							expanded);
						status_update(1);
						load_depth--;
#ifdef ZCAT
						new_free(&expand_z);
#endif /* ZCAT */
						new_free(&expanded);
						return;
					}
					if (stat_buf.st_mode & 0111)
					{
						say("%s is executable and may \
not be loaded", expanded);
						status_update(1);
						load_depth--;
#ifdef ZCAT
						new_free(&expand_z);
#endif /* ZCAT */
						new_free(&expanded);
						return;
					}
				}
			if (command && *command == 'W')
			{
				say("%s", expanded);
				status_update(1);
				load_depth--;
				new_free(&expanded);
				return;
			}
#ifdef ZCAT
			/* Open if uncompressed, zcat if compressed */
			temp = &(expanded[strlen(expanded) - strlen(ZSUFFIX)]);
			if (!strcmp(temp, ZSUFFIX))
				fp = zcat(expanded);
			else
				fp = fopen(expanded, "r");
			if (fp != NULL)
#else
				if (fp = fopen(expanded, "r"))
#endif /*ZCAT*/
				{
					display = window_display;
					window_display = 0;
					current_row = NULL;
					if (!flag)
						args = NULL;
					for (;;)
					{
						if (fgets(buffer, BIG_BUFFER_SIZE / 2, fp))
	{
		int	len;
		char	*ptr;

		for (start = buffer; isspace(*start); start++)
			;
		if (!*start || *start == '#')
			continue;

		len = strlen(start);
	/*
	 * this line from stargazer to allow \'s in scripts for continued
	 * lines <spz@specklec.mpifr-bonn.mpg.de>
	 */
		while (start[len-1] == '\n' && start[len-2] == '\\' &&
		    len < BIG_BUFFER_SIZE / 2 && fgets(&(start[len-2]),
		    BIG_BUFFER_SIZE / 2 - len, fp))
			len = strlen(start);

		if (start[len - 1] == '\n')
		    start[--len] = '\0';

		while (start && *start)
		{
			char	*optr = start;
			while ((ptr = sindex(optr, "{};")) &&
					ptr != optr &&
					ptr[-1] == '\\')
				optr = ptr+1;

			if (no_semicolon)
				no_semicolon = 0;
			else if ((!ptr || ptr != start) && current_row)
			{
				if (!paste_level)
				{
					parse_line(NULL, current_row,
						args, 0, 0);
					new_free(&current_row);
				}
				else
					malloc_strcat(&current_row, ";");
			}

			if (ptr)
			{
				char	c = *ptr;

				*ptr = '\0';
				malloc_strcat(&current_row, start);
				*ptr = c;

				switch (c)
				{
				case '{' :
					paste_level++;
					if (ptr == start)
						malloc_strcat(&current_row, " {");
					else
						malloc_strcat(&current_row, "{");
					no_semicolon = 1;
					break;

				case '}' :
					if (!paste_level)
						yell("Unexpected }");
					else
					{
						--paste_level;
						malloc_strcat(&current_row, "}");
						no_semicolon = ptr[1] ? 1 : 0;
					}
					break;

				case ';' :
					malloc_strcat(&current_row, ";");
					no_semicolon = 1;
					break;
				}

				start = ptr+1;
			}
			else
			{
				malloc_strcat(&current_row, start);
				start = NULL;
			}
		}
	}
						else
							break;
					}
					if (current_row)
					{
						if (paste_level)
							yell("Unexpected EOF");
						else
							parse_line(NULL,
								current_row, 
								args, 0, 0);
						new_free(&current_row);
					}
					window_display = display;
					fclose(fp);
				}
				else
					say("Couldn't open %s: %s", expanded,
						strerror(errno));
			new_free(&expanded);
		}
		else
			say("Unknown user");
	}
	else
		say("No filename specified");
	status_update(1);
	load_depth--;
}

/*
 * get_history: gets the next history entry, either the PREV entry or the
 * NEXT entry, and sets it to the current input string 
 */
void	
get_history(which)
	int	which;
{
	char	*ptr;

	if ((ptr = get_from_history(which)) != NULL)
	{
		set_input(ptr);
		update_input(UPDATE_ALL);
	}
}

/* BIND function: */
void
forward_character()
{
	input_move_cursor(RIGHT);
}

void
backward_character()
{
	input_move_cursor(LEFT);
}

void
backward_history()
{
	get_history(PREV);
}

void
forward_history()
{
	get_history(NEXT);
}

void
toggle_insert_mode()
{
	set_var_value(INSERT_MODE_VAR, "TOGGLE");
}

/*ARGSUSED*/
void
send_line(key, ptr)
	char	*key;
	void	(*ptr)();
{
	int	server;
	WaitPrompt	*OldPrompt;

	server = from_server;
	from_server = get_window_server(0);
	reset_hold((Window *) 0);
	hold_mode((Window *) 0, OFF, 1);
	if (current_screen->promptlist && current_screen->promptlist->type == WAIT_PROMPT_LINE)
	{
		OldPrompt = current_screen->promptlist;
		(*OldPrompt->func)(OldPrompt->data, get_input());
		set_input(empty_string);
		current_screen->promptlist = OldPrompt->next;
		new_free(&OldPrompt->data);
		new_free(&OldPrompt->prompt);
		new_free(&OldPrompt);
		change_input_prompt(-1);
	}
	else
	{
		char	*line,
			*tmp = NULL;

		line = get_input();
		malloc_strcpy(&tmp, line);

		if (do_hook(INPUT_LIST, "%s", tmp))
		{
			if (get_int_var(INPUT_ALIASES_VAR))
				parse_line(NULL, tmp, empty_string,
					1, 0);
			else
				parse_line(NULL, tmp, NULL,
					1, 0);
		}
		update_input(UPDATE_ALL);
		new_free(&tmp);
	}
	from_server = server;
}

/* The SENDLINE command.. */
static	void
sendlinecmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	int	server;
	int	display;

	server = from_server;
	display = window_display;
	window_display = 1;
	if (get_int_var(INPUT_ALIASES_VAR))
		parse_line(NULL, args, empty_string, 1, 0);
	else
		parse_line(NULL, args, NULL, 1, 0);
	update_input(UPDATE_ALL);
	window_display = display;
	from_server = server;
}

/*ARGSUSED*/
void
meta4_char(key, ptr)
	char	*key;
	void	(*ptr)();
{
	current_screen->meta4_hit = 1 - current_screen->meta4_hit;
}

/*ARGSUSED*/
void
meta3_char(key, ptr)
	char	*key;
	void	(*ptr)();
{
	current_screen->meta3_hit = 1;
}

/*ARGSUSED*/
void
meta2_char(key, ptr)
	char	*key;
	void	(*ptr)();
{
	current_screen->meta2_hit = 1;
}

/*ARGSUSED*/
void
meta1_char(key, ptr)
	char	*key;
	void	(*ptr)();
{
	current_screen->meta1_hit = 1;
}

void
quote_char(c)
	char	c;
{
	current_screen->quote_hit = 1;
}

/* type_text: the BIND function TYPE_TEXT */
/*ARGSUSED*/
void
type_text(c, str)
	char	c,
		*str;
{
	for (; *str; str++)
		input_add_character(*str, NULL);
}

/*
 * clear_screen: the CLEAR_SCREEN function for BIND.  Clears the screen and
 * starts it if it is held 
 */
/*ARGSUSED*/
void
clear_screen(c, str)
	char	c,
		*str;
{
	hold_mode((Window *) 0, OFF, 1);
	clear(NULL, empty_string, empty_string);
}

/* parse_text: the bindable function that executes its string */
void
parse_text(c, str)
	char	c,
		*str;
{
	parse_line(NULL, str, empty_string, 0, 0);
}

/*
 * edit_char: handles each character for an input stream.  Not too difficult
 * to work out.
 */
void
edit_char(key)
	u_char	key;
{
	void	(*func) ();
	char	*ptr;
	u_char	extended_key;
	WaitPrompt *oldprompt;

	if (page_updown_key) {
		page_updown_key = 0;  
		return;
	}
	page_updown_key = 0;

	if (current_screen->promptlist &&
			current_screen->promptlist->type == WAIT_PROMPT_KEY)
	{
		oldprompt = current_screen->promptlist;
		(*oldprompt->func)(oldprompt->data, &key);
		set_input(empty_string);
		current_screen->promptlist = oldprompt->next;
		new_free(&oldprompt->data);
		new_free(&oldprompt->prompt);
		new_free(&oldprompt);
		change_input_prompt(-1);
		return;
	}
	if (!get_int_var(EIGHT_BIT_CHARACTERS_VAR))
		key &= 0x7f;			/* mask out non-ascii crap */

	if (translation)
		extended_key = transFromClient[key];
	else
		extended_key = key;

	if (current_screen->meta1_hit)
	{
		func = key_names[meta1_keys[key].index].func;
		ptr = meta1_keys[key].stuff;
		current_screen->meta1_hit = 0;
	}
	else if (current_screen->meta2_hit)
	{
		func = key_names[meta2_keys[key].index].func;
		ptr = meta2_keys[key].stuff;
		current_screen->meta2_hit = 0;
	}
	else if (current_screen->meta3_hit)
	{
		func = key_names[meta3_keys[key].index].func;
		ptr = meta3_keys[key].stuff;
		current_screen->meta3_hit = 0;
	}
	else if (current_screen->meta4_hit)
	{
		func = key_names[meta4_keys[key].index].func;
		ptr = meta4_keys[key].stuff;
	}
	else
	{
		func = key_names[keys[key].index].func;
		ptr = keys[key].stuff;
	}
	if (!current_screen->meta1_hit && !current_screen->meta2_hit &&
			!current_screen->meta3_hit)
	{
		if (current_screen->inside_menu == 1)
			menu_key(key);
		else if (current_screen->digraph_hit)
		{
			if (extended_key == 0x08 || extended_key == 0x7f)
				current_screen->digraph_hit = 0;
			else if (current_screen->digraph_hit == 1)
			{
				current_screen->digraph_first = extended_key;
				current_screen->digraph_hit = 2;
			}
			else if (current_screen->digraph_hit == 2)
			{
				if ((extended_key =
				    get_digraph(extended_key)) != '\0')
					input_add_character(extended_key, NULL);
				else
					term_beep();
			}
		}
		else if (current_screen->quote_hit)
		{
			current_screen->quote_hit = 0;
			input_add_character(extended_key, NULL);
		}
		else if (func)
			func(extended_key, ptr ? ptr : empty_string);
	}
	else
		term_beep();
}

/*ARGSUSED*/
static	void
cd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg,
		*expand;

#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID)
	{
		say("You are not permitted to use this command");
		return;
	}
#endif /* DAEMON_UID */
	if ((arg = next_arg(args, &args)) != NULL)
	{
		if ((expand = expand_twiddle(arg)) != NULL)
		{
			if (chdir(expand))
				say("CD: %s", strerror(errno));
			new_free(&expand);
		}
		else
			say("CD: No such user");
	}
	if (getcwd(buffer, BIG_BUFFER_SIZE+1) != NULL) {
		say("Current directory: %s", buffer);
	}
	else {
		perror("getcwd");
	}
}

static	void
send_action(target, text)
	char	*target, *text;
{
	send_ctcp(ctcp_type[CTCP_PRIVMSG], target, "ACTION", "%s", text);
}

#if LYNX_STUFF
static	char	*
prepare_action(string)
	char	*string;
{
	short	last;
	char	*message;

	last = strlen(string) - 1;
	while(string[last] == ' ')
		if (--last < 0) return NULL;

	if ((string[last] > 'a' && string[last] < 'z') ||
			(string[last] > 'A' && string[last] < 'Z'))
	{
		message = new_malloc(last + 2);
		strmcpy (message, string, last+1);
		message[last + 1] = '.';
		message[last + 2] = '\0';
		return message;
	}
	else
		return NULL;
}
#endif

static	void
describe(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*target, *timestr;

	timestr = TimeStamp();
	target = next_arg(args, &args);
	if (target && args && *args)
	{
		int	old;
		int	from_level;
#if LYNX_STUFF
		char	*result;
#endif
		char	*message;

#if LYNX_STUFF
		if (result = prepare_action(args))
			message = result;
		else
#endif
			message = args;
		if (*target == '=')
                {
			char buffer[1024];
			int old_server;

			sprintf(buffer, "\001ACTION %s\001", message);
                        old_server = from_server;
                        from_server = -1;
                        dcc_chat_transmit(target + 1, buffer);
                        from_server = old_server;
                }
		else
		send_action(target, message);

		old = set_lastlog_msg_level(LOG_ACTION);
		from_level = message_from_level(LOG_ACTION);
		if (do_hook(SEND_ACTION_LIST, "%s %s", target, message))
			put_it("%s", parseformat(SEND_ACTION_FMT,
				get_server_nickname(from_server), target,
				message));
		set_lastlog_msg_level(old);
		message_from_level(from_level);

#if LYNX_STUFF
		if (result)
			new_free(&result);
#endif
	}
	else
		say("Usage: /DESCRIBE <target> <action description>");
}

/*
 * New 'me' command - now automatically appends period.
 * Necessary for new 'action' script.   -lynx'92
 * Hardly, removed this as it was generally considered offensive
 */
static	void
me(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
        char    *timestr;
	timestr = TimeStamp();
	if (args && *args)
	{
		char	*target;

		if ((target = get_target_by_refnum(0)) != NULL)
		{
			int	old;
			int	from_level;
#if LYNX_STUFF
			char	*result;
#endif
			char	*message;

			if (/* *target == '=' || */ !strncmp(target,
					get_string_var(CMDCHARS_VAR), 1))
				if (!(target = get_channel_by_refnum(0)))
				{
					say("No target, neither channel nor \
query");
					return;
				}
#if LYNX_STUFF
			if (result = prepare_action(args))
				message = result;
			else
#endif
				message = args;
                if (*target == '=')
                {
                        char buffer[1024];
			int old_server;

                        sprintf(buffer, "\001ACTION %s\001", message);
                        old_server = from_server;
                        from_server = -1;
                        dcc_chat_transmit(target + 1, buffer);
                        from_server = old_server;
                }
                else
			send_action(target, message);

			old = set_lastlog_msg_level(LOG_ACTION);
			message_from(target, LOG_ACTION);
			if (do_hook(SEND_ACTION_LIST, "%s %s", target, message))
			{
				if (strchr("&#", *target)) {
				 int rn = get_refnum_by_channel(target);
                                 if (!is_last_current_channel(rn, target)) {
                                   set_last_current_channel(rn, target);
                                   say("You are now talking to channel %s", target);
                                 }
				 put_it("%s", parseformat(SEND_PUBLIC_ACTION_FMT,
					get_server_nickname(from_server), target,
					message));
				} else
				 put_it("%s", parseformat(SEND_ACTION_FMT,
					get_server_nickname(from_server), target,
					message));
			}
			set_lastlog_msg_level(old);

#if LYNX_STUFF
			if (result)
				new_free(&result);
#endif
		}
		else
			say("No target, neither channel nor query");
	}
	else
		say("Usage: /ME <action description>");
}

static	void
mload(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*file;

	while ((file = next_arg(args, &args)) != NULL)
		load_menu(file);
}

static	void
mlist(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*menu;

	while ((menu = new_next_arg(args, &args)) != NULL)
		(void) ShowMenu(menu, 0);
}

static	void
evalcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	parse_line(NULL, args, subargs ? subargs : empty_string, 0, 0);
}

/*
 * ExecuteTimers:  checks to see if any currently pending timers have
 * gone off, and if so, execute them, delete them, etc, setting the
 * current_exec_timer, so that we can't remove the timer while its
 * still executing.
 */
extern	void
ExecuteTimers()
{
	time_t	current;
	TimerList *next;
	int	old_in_on_who;

	time(&current);
	while (PendingTimers && PendingTimers->time <= current)
	{
		old_in_on_who = in_on_who;
		in_on_who = PendingTimers->in_on_who;
		current_exec_timer = PendingTimers->ref;
		parse_command(PendingTimers->command, 0, empty_string);
		current_exec_timer = -1;
		new_free(&PendingTimers->command);
		next = PendingTimers->next;
		new_free(&PendingTimers);
		PendingTimers = next;
		in_on_who = old_in_on_who;
	}
}

/*
 * timercmd: the bit that handles the TIMER command.  If there are no
 * arguements, then just list the currently pending timers, if we are
 * give a -DELETE flag, attempt to delete the timer from the list.  Else
 * consider it to be a timer to add, and add it.
 */
static	void
timercmd(command, args, subargs)
	char	*command;
	char	*args,
	*subargs;
{
	char	*waittime,
		*flag;
	time_t	current;
	TimerList	**slot,
			*ntimer;
	int	want = -1,
		refnum;

	if (*args == '-')
	{
		int	len;

		flag = next_arg(args, &args);
		len = strlen(flag);

		/* first check for the -DELETE flag */

		if (!my_strnicmp(flag, "-DELETE", len))
		{
			char	*ptr;
			int	ref;
			TimerList	*tmp,
					*prev;

			if (current_exec_timer != -1)
			{
				say("You may not remove a TIMER from itself");
				return;
			}
			if (!(ptr = next_arg(args, &args)))
			{
				say("%s: Need a timer reference number for -DELETE", command);
				return;
			}
			ref = atoi(ptr);
			for (prev = tmp = PendingTimers; tmp; prev = tmp,
					tmp = tmp->next)
			{
				if (tmp->ref == ref)
				{
					if (tmp == prev)
						PendingTimers =
							PendingTimers->next;
					else
						prev->next = tmp->next;
					new_free(&tmp->command);
					new_free(&tmp);
					return;
				}
			}
			say("%s: Can't delete %d, no such refnum",
				command, ref);
			return;
		}
		else if (!my_strnicmp(flag, "-REFNUM", len))
		{
			char	*ptr;

			ptr = next_arg(args, &args);
			want = atoi(ptr);
			if (want < 0)
			{
				say("%s: Illegal refnum %d", command, want);
				return;
			}
		}
		else
		{
			say("%s: %s no such flag", command, flag);
			return;
		}
	}

	/* else check to see if we have no args -> list */

	if (!(waittime = next_arg(args, &args)))
	{
		show_timer(command);
		return;
	}

	/* must be something to add */

	if ((refnum = create_timer_ref(want)) == -1)
	{
		say("%s: Refnum %d already exists", command, want);
		return;
	}
	time(&current);
	ntimer = (TimerList *) new_malloc(sizeof(TimerList));
	ntimer->in_on_who = in_on_who;
	ntimer->time = current + atol(waittime);
	ntimer->ref = refnum;
	ntimer->command = NULL;
	malloc_strcpy(&ntimer->command, args);

	/* we've created it, now put it in order */

	for (slot = &PendingTimers; *slot; slot = &(*slot)->next)
	{
		if ((*slot)->time > ntimer->time)
			break;
	}
	ntimer->next = *slot;
	*slot = ntimer;
}

/*
 * show_timer:  Display a list of all the TIMER commands that are
 * pending to be executed.
 */
static	void
show_timer(command)
	char	*command;
{
	TimerList	*tmp;
	time_t	current,
		time_left;

	if (!PendingTimers)
	{
		say("%s: No commands pending to be executed", command);
		return;
	}

	time(&current);
	say("Timer Seconds   Command");
	for (tmp = PendingTimers; tmp; tmp = tmp->next)
	{
		time_left = tmp->time - current;
		if (time_left < 0)
			time_left = 0;
		say("%-5d %-10d %s", tmp->ref, time_left, tmp->command);
	}
}

/*
 * create_timer_ref:  returns the lowest unused reference number for
 * a timer
 */
static	int
create_timer_ref(want)
	int	want;
{
	TimerList	*tmp;
	int	ref = 0;
	int	done = 0;

	if (want == -1)
		while (!done)
		{
			done = 1;
			for (tmp = PendingTimers; tmp; tmp = tmp->next)
				if (ref == tmp->ref)
				{
					ref++;
					done = 0;
					break;
				}
		}
	else
	{
		ref = want;
		for (tmp = PendingTimers; tmp; tmp = tmp->next)
			if (ref == tmp->ref)
			{
				ref = -1;
				break;
			}
	}

	return (ref);
}

/*
 * inputcmd:  the INPUT command.   Takes a couple of arguements...
 * the first surrounded in double quotes, and the rest makes up
 * a normal ircII command.  The command is evalutated, with $*
 * being the line that you input.  Used add_wait_prompt() to prompt
 * the user...  -phone, jan 1993.
 */

static	void
inputcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*prompt;

	if (!args || !*args)
		return;
	
	if (*args++ != '"')
	{
		say("Need \" to begin prompt for INPUT");
		return;
	}

	prompt = args;
	if ((args = index(prompt, '"')) != NULL)
		*args++ = '\0';
	else
	{
		say("Missing \" in INPUT");
		return;
	}

	for (; *args == ' '; args++)
		;

	add_wait_prompt(prompt, eval_inputlist, args, WAIT_PROMPT_LINE);
}

/*
 * eval_inputlist:  Cute little wrapper that calls parse_line() when we
 * get an input prompt ..
 */

void
eval_inputlist(args, line)
	char	*args,
		*line;
{
	parse_line(NULL, args, line ? line : empty_string, 0, 0);
}

/* pingcmd: ctcp ping, duh - phone, jan 1993. */
static	void
pingcmd(command, args, subargs)
	char    *command,
		*args,
		*subargs;
{
	int temp, result;
	struct  timeval         tp;
	struct  timezone        tzp;

	if (!args || !*args)
		args = "*";

	result = gettimeofday(&tp,&tzp);
	temp = get_int_var(PING_TYPE_VAR);
	sprintf(buffer, "%s PING %ld", args, time(NULL));
	if (temp == 1 && !result)
		sprintf(buffer, "%s PING %ld %ld", args, tp.tv_sec,tp.tv_usec);
	if (temp == 2 && !result)
		sprintf(buffer, "%s ECHO %ld %ld", args, tp.tv_sec,tp.tv_usec);
	ctcp(command, buffer, empty_string);
}

static	void
xtypecmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*arg;
	int	len;

	if (args && *args == '-')
	{
		args++;
		if ((arg = next_arg(args, &args)) != NULL)
		{
			len = strlen(arg);
			if (!my_strnicmp(arg, "LITERAL", len))
			{
				for (; *args; args++)
					input_add_character(*args, NULL);
			}
			else
				say ("Unknown flag -%s to XTYPE", arg);
			return;
		}
		input_add_character('-', NULL);
	}
	else
		type(command, args, empty_string);
	return;
}

static	void
beepcmd(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	term_beep();
}
