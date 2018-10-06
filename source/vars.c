/*
 * vars.c: All the dealing of the irc variables are handled here. 
 *
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: vars.c,v 1.20.2.2 2002/03/11 19:24:05 toast Exp $";
#endif

#include "irc.h"

#include "status.h"
#include "window.h"
#include "lastlog.h"
#include "log.h"
#include "crypt.h"
#include "history.h"
#include "notify.h"
#include "vars.h"
#include "input.h"
#include "ircaux.h"
#include "whois.h"
#include "translat.h"
#include "term.h"
#include "output.h"
#include "server.h"
#include "comstud.h"
#include "toast.h"

/* IrcVariable Flags */
#define	VF_NODAEMON	0x0001

/* IrcVariable Internal Flags */
#define VIF_CHANGED	0x01
#define VIF_GLOBAL	0x02

char	*var_settings[] =
{
	"OFF", "ON", "TOGGLE"
};

/* For the NOVICE variable. Complain loudlly if turned off manually.  */
extern	int	load_depth;

	int	loading_global = 0;

static	void	exec_warning();
static	void	input_warning();
static	void	eight_bit_characters();
static	void	set_realname _((char *));
static	void	set_auto_reoper _((int));
static	void	set_ops_method_var _((char *));

/*
 * irc_variable: all the irc variables used.  Note that the integer and
 * boolean defaults are set here, which the string default value are set in
 * the init_variables() procedure 
 */
static	IrcVariable irc_variable[] =
{
	{ "ALWAYS_SPLIT_BIGGEST",	BOOL_TYPE_VAR,	DEFAULT_ALWAYS_SPLIT_BIGGEST, NULL, NULL, 0, 0 },
	{ "AUTO_ACCEPT_DCC_CHAT",	BOOL_TYPE_VAR,	DEFAULT_AUTO_ACCEPT_DCC_CHAT, NULL, NULL, 0, 0 },
	{ "AUTO_ACCEPT_DCC_SEND",	BOOL_TYPE_VAR,	DEFAULT_AUTO_ACCEPT_DCC_SEND, NULL, NULL, 0, 0 },
	{ "AUTO_NSLOOKUP",		BOOL_TYPE_VAR,	DEFAULT_AUTO_NSLOOKUP, NULL, NULL, 0, 0 },
	{ "AUTO_REJOIN",		INT_TYPE_VAR,	DEFAULT_AUTO_REJOIN, NULL, NULL, 0, 0 },
	{ "AUTO_REOPER",		BOOL_TYPE_VAR,	DEFAULT_AUTO_REOPER, NULL, set_auto_reoper, 0, 0 },
	{ "AUTO_UNMARK_AWAY",		BOOL_TYPE_VAR,	DEFAULT_AUTO_UNMARK_AWAY, NULL, NULL, 0, 0 },
	{ "AUTO_WHOWAS",		BOOL_TYPE_VAR,	DEFAULT_AUTO_WHOWAS, NULL, NULL, 0, 0 },
	{ "AUTOOP_DELAY",		INT_TYPE_VAR,	DEFAULT_AUTOOP_DELAY, NULL, NULL, 0, 0},
	{ "AUTOOP_ENABLE",		BOOL_TYPE_VAR,	DEFAULT_AUTOOP_ENABLE, NULL, NULL, 0, 0},
	{ "AWAY_RECORDER",		BOOL_TYPE_VAR,	DEFAULT_AWAY_RECORDER, NULL, NULL, 0, 0},
	{ "BAN_TIMEOUT",		INT_TYPE_VAR,	DEFAULT_BAN_TIMEOUT, NULL, NULL, 0, 0},
	{ "BANNER",			STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "BEEP",			BOOL_TYPE_VAR,	DEFAULT_BEEP, NULL, NULL, 0, 0 },
	{ "BEEP_MAX",			INT_TYPE_VAR,	DEFAULT_BEEP_MAX, NULL, NULL, 0, 0 },
	{ "BEEP_ON_MSG",		STR_TYPE_VAR,	0, NULL, set_beep_on_msg, 0, 0 },
	{ "BEEP_WHEN_AWAY",		INT_TYPE_VAR,	DEFAULT_BEEP_WHEN_AWAY, NULL, NULL, 0, 0 },
	{ "BLINK_VIDEO",		BOOL_TYPE_VAR,	DEFAULT_BLINK_VIDEO, NULL, NULL, 0, 0 },
	{ "BOLD_VIDEO",			BOOL_TYPE_VAR,	DEFAULT_BOLD_VIDEO, NULL, NULL, 0, 0 },
	{ "CHANNEL_NAME_WIDTH",		INT_TYPE_VAR,	DEFAULT_CHANNEL_NAME_WIDTH, NULL, update_all_status, 0, 0 },
	{ "CLIENT_INFORMATION",		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "CLOCK",			BOOL_TYPE_VAR,	DEFAULT_CLOCK, NULL, update_all_status, 0, 0 },
	{ "CLOCK_24HOUR",		BOOL_TYPE_VAR,	DEFAULT_CLOCK_24HOUR, NULL, reset_clock, 0, 0 },
	{ "CLOCK_ALARM",		STR_TYPE_VAR,	0, NULL, set_alarm, 0, 0 },
	{ "CMDCHARS",			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "COMMAND_MODE",		BOOL_TYPE_VAR,	DEFAULT_COMMAND_MODE, NULL, NULL, 0, 0 },
	{ "CONTINUED_LINE",		STR_TYPE_VAR,	0, NULL, set_continued_line, 0, 0 },
	{ "DCC_BLOCK_SIZE",		INT_TYPE_VAR,	DEFAULT_DCC_BLOCK_SIZE, NULL, NULL, 0, 0 },
	{ "DEBUG",			INT_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "DEFAULT_REASON",		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "DISPLAY",			BOOL_TYPE_VAR,	DEFAULT_DISPLAY, NULL, NULL, 0, 0 },
	{ "EIGHT_BIT_CHARACTERS",	BOOL_TYPE_VAR,	DEFAULT_EIGHT_BIT_CHARACTERS, NULL, eight_bit_characters, 0, 0 },
	{ "ENCRYPT_PROGRAM",		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "EXEC_PROTECTION",		BOOL_TYPE_VAR,	DEFAULT_EXEC_PROTECTION, NULL, exec_warning, 0, VF_NODAEMON },
	{ "FAKE_HOST",			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "FILTER_LOG",			BOOL_TYPE_VAR,	DEFAULT_FILTER_LOG, NULL, NULL, 0, 0 },
	{ "FLOOD_AFTER",		INT_TYPE_VAR,	DEFAULT_FLOOD_AFTER, NULL, NULL, 0, 0 },
	{ "FLOOD_RATE",			INT_TYPE_VAR,	DEFAULT_FLOOD_RATE, NULL, NULL, 0, 0 },
	{ "FLOOD_USERS",		INT_TYPE_VAR,	DEFAULT_FLOOD_USERS, NULL, NULL, 0, 0 },
	{ "FLOOD_WARNING",		BOOL_TYPE_VAR,	DEFAULT_FLOOD_WARNING, NULL, NULL, 0, 0 },
	{ "FULL_STATUS_LINE",		BOOL_TYPE_VAR,	DEFAULT_FULL_STATUS_LINE, NULL, update_all_status, 0, 0 },
	{ "HELP_FILE",			STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "HELP_PAGER",			BOOL_TYPE_VAR,	DEFAULT_HELP_PAGER, NULL, NULL, 0, 0 },
	{ "HELP_PATH",			STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "HELP_PROMPT",		BOOL_TYPE_VAR,	DEFAULT_HELP_PROMPT, NULL, NULL, 0, 0 },
	{ "HELP_WINDOW",		BOOL_TYPE_VAR,	DEFAULT_HELP_WINDOW, NULL, NULL, 0, 0 },
	{ "HIDE_PRIVATE_CHANNELS",	BOOL_TYPE_VAR,	DEFAULT_HIDE_PRIVATE_CHANNELS, NULL, update_all_status, 0, 0 },
	{ "HIGHLIGHT_CHAR",		STR_TYPE_VAR,	0, NULL, set_highlight_char, 0, 0 },
	{ "HISTORY",			INT_TYPE_VAR,	DEFAULT_HISTORY, NULL, set_history_size, 0, VF_NODAEMON },
	{ "HISTORY_FILE",		STR_TYPE_VAR,	0, NULL, set_history_file, 0, 0 },
	{ "HOLD_MODE",			BOOL_TYPE_VAR,	DEFAULT_HOLD_MODE, NULL, reset_line_cnt, 0, 0 },
	{ "HOLD_MODE_MAX",		INT_TYPE_VAR,	DEFAULT_HOLD_MODE_MAX, NULL, NULL, 0, 0 },
	{ "IGN_PARAM",			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "INDENT",			BOOL_TYPE_VAR,	DEFAULT_INDENT, NULL, NULL, 0, 0 },
	{ "INPUT_ALIASES",		BOOL_TYPE_VAR,	DEFAULT_INPUT_ALIASES, NULL, NULL, 0, 0 },
	{ "INPUT_PROMPT",		STR_TYPE_VAR,	0, NULL, set_input_prompt, 0, 0 },
	{ "INPUT_PROTECTION",		BOOL_TYPE_VAR,	DEFAULT_INPUT_PROTECTION, NULL, input_warning, 0, 0 },
	{ "INSERT_MODE",		BOOL_TYPE_VAR,	DEFAULT_INSERT_MODE, NULL, update_all_status, 0, 0 },
	{ "INVERSE_VIDEO",		BOOL_TYPE_VAR,	DEFAULT_INVERSE_VIDEO, NULL, NULL, 0, 0 },
	{ "KILLLOGFILE",		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "LAST_MSG",			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "LAST_NOTICE",		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "LAST_SENDMSG",		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "LAST_SENDNOTICE",		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "LASTLOG",			INT_TYPE_VAR,	DEFAULT_LASTLOG, NULL, set_lastlog_size, 0, 0 },
	{ "LASTLOG_LEVEL",		STR_TYPE_VAR,	0, NULL, set_lastlog_level, 0, 0 },
	{ "LOAD_PATH",			STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "LOG",			BOOL_TYPE_VAR,	DEFAULT_LOG, NULL, logger, 0, 0 },
	{ "LOGFILE",			STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "LOOP_RECONNECT",		BOOL_TYPE_VAR,	DEFAULT_LOOP_RECONNECT, NULL, NULL, 0, 0 },
	{ "MAIL",			INT_TYPE_VAR,	DEFAULT_MAIL, NULL, update_all_status, 0, VF_NODAEMON },
	{ "MAX_RECURSIONS",		INT_TYPE_VAR,	DEFAULT_MAX_RECURSIONS, NULL, NULL, 0, 0 },
	{ "MAX_WALL_NICKS",		INT_TYPE_VAR,	DEFAULT_MAX_WALL_NICKS, NULL, NULL, 0, 0 },
	{ "MENU",			STR_TYPE_VAR,	0, NULL, set_menu, 0, 0 },
	{ "MINIMUM_SERVERS",		INT_TYPE_VAR,	DEFAULT_MINIMUM_SERVERS, NULL, NULL, 0, VF_NODAEMON },
	{ "MINIMUM_USERS",		INT_TYPE_VAR,	DEFAULT_MINIMUM_USERS, NULL, NULL, 0, VF_NODAEMON },
	{ "MSGLOG",			BOOL_TYPE_VAR,	0, NULL, msglogger, 0, 0 },
	{ "MSGLOGFILE",			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "NO_CTCP_FLOOD",		BOOL_TYPE_VAR,	DEFAULT_NO_CTCP_FLOOD, NULL, NULL, 0, 0 },
	{ "NO_LOG",			STR_TYPE_VAR,	0, NULL, set_nolog, 0, 0 },
	{ "NOTIFY_HANDLER",		STR_TYPE_VAR, 	0, 0, set_notify_handler, 0, 0 },
	{ "NOTIFY_LEVEL",		STR_TYPE_VAR,	0, NULL, set_notify_level, 0, 0 },
	{ "NOTIFY_ON_TERMINATION",	BOOL_TYPE_VAR,	DEFAULT_NOTIFY_ON_TERMINATION, NULL, NULL, 0, VF_NODAEMON },
	{ "NOVICE",			BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "NUM_BANMODES",		INT_TYPE_VAR,   4, NULL, NULL, 0, 0 },
	{ "NUM_OPMODES",		INT_TYPE_VAR,   4, NULL, NULL, 0, 0 },
	{ "OLD_WHOIS_FORMAT",		BOOL_TYPE_VAR,	DEFAULT_OLD_WHOIS_FORMAT, NULL, NULL, 0, 0 },
	{ "OPS_METHOD",			STR_TYPE_VAR,	0, NULL, set_ops_method_var, 0, 0 },
	{ "PING_TYPE",			INT_TYPE_VAR,   DEFAULT_PING_TYPE, NULL, NULL, 0, 0 },
	{ "PUB_LOGSTR",			STR_TYPE_VAR,   0, NULL, NULL, 0, 0 },
	{ "REALNAME",			STR_TYPE_VAR,	0, 0, set_realname, 0, VF_NODAEMON },
	{ "REASON_TYPE",		INT_TYPE_VAR,   DEFAULT_REASON_TYPE, NULL, NULL, 0, 0 },
	{ "REJOIN_BADKEY",		BOOL_TYPE_VAR,	DEFAULT_REJOIN_BADKEY, NULL, NULL, 0, 0 },
	{ "REJOIN_BANNED",		BOOL_TYPE_VAR,	DEFAULT_REJOIN_BANNED, NULL, NULL, 0, 0 },
	{ "REJOIN_FULL",		BOOL_TYPE_VAR,	DEFAULT_REJOIN_FULL, NULL, NULL, 0, 0 },
	{ "REJOIN_INTERVAL",		INT_TYPE_VAR,	DEFAULT_REJOIN_INTERVAL, NULL, NULL, 0, 0 },
	{ "REJOIN_INVITEONLY",		BOOL_TYPE_VAR,	DEFAULT_REJOIN_INVITEONLY, NULL, NULL, 0, 0 },
	{ "REJOIN_UNAVAILABLE",		BOOL_TYPE_VAR,	DEFAULT_REJOIN_UNAVAILABLE, NULL, NULL, 0, 0 },
	{ "SCREEN_OPTIONS", 		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SCROLL",			BOOL_TYPE_VAR,	DEFAULT_SCROLL, NULL, set_scroll, 0, 0 },
	{ "SCROLL_LINES",		INT_TYPE_VAR,	DEFAULT_SCROLL_LINES, NULL, set_scroll_lines, 0, 0 },
	{ "SEND_IGNORE_MSG",		BOOL_TYPE_VAR,	DEFAULT_SEND_IGNORE_MSG, NULL, NULL, 0, 0 },
	{ "SERVER_NOTICE_WINDOW",	STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SHELL",			STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SHELL_FLAGS",		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SHELL_LIMIT",		INT_TYPE_VAR,	DEFAULT_SHELL_LIMIT, NULL, NULL, 0, VF_NODAEMON },
	{ "SHOW_ADMIN_REQUESTS",	BOOL_TYPE_VAR,	DEFAULT_SHOW_ADMIN_REQUESTS, NULL, NULL, 0, 0 },
	{ "SHOW_AWAY_ONCE",		BOOL_TYPE_VAR,	DEFAULT_SHOW_AWAY_ONCE, NULL, NULL, 0, 0 },
	{ "SHOW_CHANNEL_NAMES",		BOOL_TYPE_VAR,	DEFAULT_SHOW_CHANNEL_NAMES, NULL, NULL, 0, 0 },
	{ "SHOW_CTCP_IDLE",		BOOL_TYPE_VAR,	DEFAULT_SHOW_CTCP_IDLE, NULL, NULL, 0, 0 },
	{ "SHOW_END_OF_MSGS",		BOOL_TYPE_VAR,	DEFAULT_SHOW_END_OF_MSGS, NULL, NULL, 0, 0 },
	{ "SHOW_FAKES",			BOOL_TYPE_VAR,	DEFAULT_SHOW_FAKES, NULL, NULL, 0, 0 },
	{ "SHOW_ILINE_FULL",		BOOL_TYPE_VAR,	DEFAULT_SHOW_ILINE_FULL, NULL, NULL, 0, 0 },
	{ "SHOW_INFO_REQUESTS",		BOOL_TYPE_VAR,	DEFAULT_SHOW_INFO_REQUESTS, NULL, NULL, 0, 0 },
	{ "SHOW_LINKS_REQUESTS",	BOOL_TYPE_VAR,	DEFAULT_SHOW_LINKS_REQUESTS, NULL, NULL, 0, 0 },
	{ "SHOW_MOTD_REQUESTS",		BOOL_TYPE_VAR,	DEFAULT_SHOW_MOTD_REQUESTS, NULL, NULL, 0, 0 },
	{ "SHOW_MY_WALLOPS",		BOOL_TYPE_VAR,	DEFAULT_SHOW_MY_WALLOPS, NULL, NULL, 0, 0 },
	{ "SHOW_NUMERICS",		BOOL_TYPE_VAR,	DEFAULT_SHOW_NUMERICS, NULL, NULL, 0, 0 },
	{ "SHOW_SERVER_KILLS",		BOOL_TYPE_VAR,	DEFAULT_SHOW_SERVER_KILLS, NULL, NULL, 0, 0 },
	{ "SHOW_STATS_REQUESTS",	BOOL_TYPE_VAR,	DEFAULT_SHOW_STATS_REQUESTS, NULL, NULL, 0, 0 },
	{ "SHOW_STATUS_ALL",		BOOL_TYPE_VAR,	DEFAULT_SHOW_STATUS_ALL, NULL, update_all_status, 0, 0 },
	{ "SHOW_TOOMANY",		BOOL_TYPE_VAR,	DEFAULT_SHOW_TOOMANY, NULL, NULL, 0, 0 },
	{ "SHOW_TRACE_REQUESTS",	BOOL_TYPE_VAR,	DEFAULT_SHOW_TRACE_REQUESTS, NULL, NULL, 0, 0 },
	{ "SHOW_UNAUTHS",		BOOL_TYPE_VAR,	DEFAULT_SHOW_UNAUTHS, NULL, NULL, 0, 0 },
	{ "SHOW_WHO_HOPCOUNT", 		BOOL_TYPE_VAR,	DEFAULT_SHOW_WHO_HOPCOUNT, NULL, NULL, 0, 0 },
	{ "STATUS_AWAY",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_CHANNEL",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_CHANOP",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_CHANVOICE",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_CLOCK",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_FORMAT",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_HOLD",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_HOLD_LINES",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_INSERT",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_MAIL",		STR_TYPE_VAR,	0, NULL, build_status, 0, VF_NODAEMON },
	{ "STATUS_MODE",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_MSGS",		STR_TYPE_VAR,	0, NULL, build_status, 0, VF_NODAEMON },
	{ "STATUS_NOTIFY",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_OPER",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_OVERWRITE",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_QUERY",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_SERVER",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_UMODE",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER1",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER2",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER3",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_WINDOW",		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "SUPPRESS_SERVER_MOTD",	BOOL_TYPE_VAR,	DEFAULT_SUPPRESS_SERVER_MOTD, NULL, NULL, 0, VF_NODAEMON },
	{ "TAB",			BOOL_TYPE_VAR,	DEFAULT_TAB, NULL, NULL, 0, 0 },
	{ "TAB_MAX",			INT_TYPE_VAR,	DEFAULT_TAB_MAX, NULL, NULL, 0, 0 },
	{ "TIMESTAMP",			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "TRANSLATION",		STR_TYPE_VAR,	0, NULL, set_translation, 0, 0 },
	{ "UNDERLINE_VIDEO",		BOOL_TYPE_VAR,	DEFAULT_UNDERLINE_VIDEO, NULL, NULL, 0, 0 },
	{ "URLLOG_FILE",		STR_TYPE_VAR,	0, NULL, set_urllog_file, 0, 0 },
	{ "URLLOG_LEVEL",		STR_TYPE_VAR,	0, NULL, set_urllog_level, 0, 0 },
	{ "USE_FAKE_HOST",		BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "USER_INFORMATION", 		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "USER_WALLOPS",		BOOL_TYPE_VAR,	DEFAULT_USER_WALLOPS, NULL, NULL, 0, 0 },
	{ "USERHOST_NOTIFY",		BOOL_TYPE_VAR,	DEFAULT_USERHOST_NOTIFY, NULL, NULL, 0, 0 },
	{ "VERBOSE_CTCP",		BOOL_TYPE_VAR,	DEFAULT_VERBOSE_CTCP, NULL, NULL, 0, 0 },
	{ "VERBOSE_WHOKILL",		BOOL_TYPE_VAR,	DEFAULT_VERBOSE_WHOKILL, NULL, NULL, 0, 0 },
	{ "VHOST",			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "WARN_OF_IGNORES",		BOOL_TYPE_VAR,	DEFAULT_WARN_OF_IGNORES, NULL, NULL, 0, 0 },
	{ "XTERM_OPTIONS", 		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ (char *) 0, 0, 0, 0, 0, 0, 0 }
};

/*
 * init_variables: initializes the string variables that can't really be
 * initialized properly above 
 */
void
init_variables()
{
	set_string_var(MSGLOGFILE_VAR, DEFAULT_MSGLOGFILE);
	if (DEFAULT_MSGLOG)
	{
		set_int_var(MSGLOG_VAR, DEFAULT_MSGLOG);
		msglogger(1);
	}
	set_string_var(KILLLOGFILE_VAR, DEFAULT_KILLLOGFILE);
	log_kill(NULL, "", "", "", "");
	set_string_var(FAKE_HOST_VAR, DEFAULT_FAKE_HOST);
	set_string_var(PUBLOGSTR_VAR, DEFAULT_PUBLOGSTR);
	set_string_var(DEFAULT_REASON_VAR, DEFAULT_KICK_REASON);
	set_string_var(CMDCHARS_VAR, DEFAULT_CMDCHARS);
	set_string_var(LOGFILE_VAR, DEFAULT_LOGFILE);
	set_string_var(SHELL_VAR, DEFAULT_SHELL);
	set_string_var(SHELL_FLAGS_VAR, DEFAULT_SHELL_FLAGS);
	set_string_var(ENCRYPT_PROGRAM_VAR, DEFAULT_ENCRYPT_PROGRAM);
	set_string_var(CONTINUED_LINE_VAR, DEFAULT_CONTINUED_LINE);
	set_string_var(INPUT_PROMPT_VAR, DEFAULT_INPUT_PROMPT);
	set_string_var(HIGHLIGHT_CHAR_VAR, DEFAULT_HIGHLIGHT_CHAR);
	set_string_var(HISTORY_FILE_VAR, DEFAULT_HISTORY_FILE);
	set_string_var(LASTLOG_LEVEL_VAR, DEFAULT_LASTLOG_LEVEL);
	set_string_var(NOTIFY_HANDLER_VAR, DEFAULT_NOTIFY_HANDLER);
	set_string_var(NOTIFY_LEVEL_VAR, DEFAULT_NOTIFY_LEVEL);
	set_string_var(REALNAME_VAR, realname);
	set_string_var(SERVER_NOTICE_WINDOW_VAR, DEFAULT_SERVER_NOTICE_WINDOW);
	set_string_var(STATUS_FORMAT_VAR, DEFAULT_STATUS_FORMAT);
	set_string_var(STATUS_AWAY_VAR, DEFAULT_STATUS_AWAY);
	set_string_var(STATUS_CHANNEL_VAR, DEFAULT_STATUS_CHANNEL);
	set_string_var(STATUS_CHANOP_VAR, DEFAULT_STATUS_CHANOP);
	set_string_var(STATUS_CHANVOICE_VAR, DEFAULT_STATUS_CHANVOICE);
	set_string_var(STATUS_CLOCK_VAR, DEFAULT_STATUS_CLOCK);
	set_string_var(STATUS_HOLD_VAR, DEFAULT_STATUS_HOLD);
	set_string_var(STATUS_HOLD_LINES_VAR, DEFAULT_STATUS_HOLD_LINES);
	set_string_var(STATUS_INSERT_VAR, DEFAULT_STATUS_INSERT);
	set_string_var(STATUS_MAIL_VAR, DEFAULT_STATUS_MAIL);
	set_string_var(STATUS_MODE_VAR, DEFAULT_STATUS_MODE);
	set_string_var(STATUS_MSGS_VAR, DEFAULT_STATUS_MSGS);
	set_string_var(STATUS_OPER_VAR, DEFAULT_STATUS_OPER);
	set_string_var(STATUS_OVERWRITE_VAR, DEFAULT_STATUS_OVERWRITE);
	set_string_var(STATUS_QUERY_VAR, DEFAULT_STATUS_QUERY);
	set_string_var(STATUS_SERVER_VAR, DEFAULT_STATUS_SERVER);
	set_string_var(STATUS_UMODE_VAR, DEFAULT_STATUS_UMODE);
	set_string_var(STATUS_USER_VAR, DEFAULT_STATUS_USER);
	set_string_var(STATUS_USER1_VAR, DEFAULT_STATUS_USER1);
	set_string_var(STATUS_USER2_VAR, DEFAULT_STATUS_USER2);
	set_string_var(STATUS_USER3_VAR, DEFAULT_STATUS_USER3);
	set_string_var(STATUS_WINDOW_VAR, DEFAULT_STATUS_WINDOW);
	set_string_var(USERINFO_VAR, DEFAULT_USERINFO);
	set_string_var(XTERM_OPTIONS_VAR, DEFAULT_XTERM_OPTIONS);
	set_alarm(DEFAULT_CLOCK_ALARM);
	set_beep_on_msg(DEFAULT_BEEP_ON_MSG);
	set_string_var(STATUS_NOTIFY_VAR, DEFAULT_STATUS_NOTIFY);
	set_string_var(CLIENTINFO_VAR, IRCII_COMMENT);
	/*set_string_var(TRANSLATION_VAR, "ASCII");*/
	/*set_translation("ASCII");*/
	set_string_var(HELP_PATH_VAR, DEFAULT_HELP_PATH);
	set_string_var(HELP_FILE_VAR, DEFAULT_HELP_FILE);
	set_lastlog_size(irc_variable[LASTLOG_VAR].integer);
	set_history_size(irc_variable[HISTORY_VAR].integer);
	set_history_file(irc_variable[HISTORY_FILE_VAR].string);
	set_highlight_char(irc_variable[HIGHLIGHT_CHAR_VAR].string);
	set_lastlog_level(irc_variable[LASTLOG_LEVEL_VAR].string);
	set_notify_level(irc_variable[NOTIFY_LEVEL_VAR].string);
	set_string_var(TIMESTAMP_VAR, DEFAULT_TIMESTAMP);
	set_string_var(IGN_PARAM_VAR, DEFAULT_IGN_PARAM);
	set_string_var(OPS_METHOD_VAR, DEFAULT_OPS_METHOD);
	set_ops_method_var(irc_variable[OPS_METHOD_VAR].string);
	set_string_var(BANNER_VAR, DEFAULT_BANNER);
	set_string_var(URLLOG_FILE_VAR, DEFAULT_URLLOG_FILE);
	set_string_var(URLLOG_LEVEL_VAR, DEFAULT_URLLOG_LEVEL);
	set_urllog_level(irc_variable[URLLOG_LEVEL_VAR].string);
}

/*
 * find_variable: looks up variable name in the variable table and returns
 * the index into the variable array of the match.  If there is no match, cnt
 * is set to 0 and -1 is returned.  If more than one match the string, cnt is
 * set to that number, and it returns the first match.  Index will contain
 * the index into the array of the first found entry 
 */
int
find_variable(org_name, cnt)
	char	*org_name;
	int	*cnt;
{
	IrcVariable *v,
		    *first;
	int	len,
		var_index;
	char	*name = (char *) 0;

	malloc_strcpy(&name,org_name);
	upper(name);
	len = strlen(name);
	var_index = 0;
	for (first = irc_variable; first->name; first++, var_index++)

	{
		if (strncmp(name, first->name, len) == 0)
		{
			*cnt = 1;
			break;
		}
	}
	if (first->name)

	{
		if (strlen(first->name) != len)
		{
			v = first;
			for (v++; v->name; v++, (*cnt)++)

			{
				if (strncmp(name, v->name, len) != 0)
					break;
			}
		}
		new_free(&name);
		return (var_index);
	}
	else

	{
		*cnt = 0;
		new_free(&name);
		return (-1);
	}
}

/*
 * do_boolean: just a handy thing.  Returns 1 if the str is not ON, OFF, or
 * TOGGLE 
 */
int
do_boolean(str, value)
	char	*str;
	int	*value;
{
	upper(str);
	if (strcmp(str, var_settings[ON]) == 0)
		*value = 1;
	else if (strcmp(str, var_settings[OFF]) == 0)
		*value = 0;
	else if (strcmp(str, "TOGGLE") == 0)
	{
		if (*value)
			*value = 0;
		else
			*value = 1;
	}
	else
		return (1);
	return (0);
}

/*
 * set_var_value: Given the variable structure and the string representation
 * of the value, this sets the value in the most verbose and error checking
 * of manors.  It displays the results of the set and executes the function
 * defined in the var structure 
 */
void
set_var_value(var_index, value)
	int	var_index;
	char	*value;
{
	char	*rest;
	IrcVariable *var;
	int	old;


	var = &(irc_variable[var_index]);
#ifdef DAEMON_UID
	if (getuid() == DAEMON_UID && var->flags&VF_NODAEMON && value && *value)
	{
		say("You are not permitted to set that variable");
		return;
	}
#endif
	switch (var->type)
	{
	case BOOL_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest)))
		{
			old = var->integer;
			if (do_boolean(value, &(var->integer)))

			{
				say("Value must be either ON, OFF, or TOGGLE");
				break;
			}
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (old != var->integer)
					var->int_flags |= VIF_CHANGED;
			}
			if (loading_global)
				var->int_flags |= VIF_GLOBAL;
			if (var->func)
				(var->func) (var->integer);
			say("Value of %s set to %s", var->name,
				var->integer ? var_settings[ON]
					     : var_settings[OFF]);
		}
		else
			say("Current value of %s is %s", var->name,
				(var->integer) ?
				var_settings[ON] : var_settings[OFF]);
		break;
	case CHAR_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest)))
		{
			if (strlen(value) > 1)
				say("Value of %s must be a single character",
					var->name);
			else
			{
				if (!(var->int_flags & VIF_CHANGED))
				{
					if (var->integer != *value)
						var->int_flags |= VIF_CHANGED;
				}
				if (loading_global)
					var->int_flags |= VIF_GLOBAL;
				var->integer = *value;
				if (var->func)
					(var->func) (var->integer);
				say("Value of %s set to '%c'", var->name,
					var->integer);
			}
		}
		else
			say("Current value of %s is '%c'", var->name,
				var->integer);
		break;
	case INT_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest)))
		{
			int	val;

			if (!is_number(value))
			{
				say("Value of %s must be numeric!", var->name);
				break;
			}
			if ((val = atoi(value)) < 0)
			{
				say("Value of %s must be greater than 0",
					var->name);
				break;
			}
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (var->integer != val)
					var->int_flags |= VIF_CHANGED;
			}
			if (loading_global)
				var->int_flags |= VIF_GLOBAL;
			var->integer = val;
			if (var->func)
				(var->func) (var->integer);
			say("Value of %s set to %d", var->name, var->integer);
		}
		else
			say("Current value of %s is %d", var->name,
				var->integer);
		break;
	case STR_TYPE_VAR:
		if (value)
		{
			if (*value)
			{
				if ((!var->int_flags & VIF_CHANGED))
				{
					if ((var->string && ! value) ||
					    (! var->string && value) ||
					    my_stricmp(var->string, value))
						var->int_flags |= VIF_CHANGED;
				}
				if (loading_global)
					var->int_flags |= VIF_GLOBAL;
				malloc_strcpy(&(var->string), value);
			}
			else
			{
				if (var->string)
					say("Current value of %s is %s",
						var->name, var->string);
				else
					say("No value for %s has been set",
						var->name);
				return;
			}
		}
		else
			new_free(&(var->string));
		if (var->func)
			(var->func) (var->string);
		say("Value of %s set to %s", var->name, var->string ?
			var->string : "<EMPTY>");
		break;
	}
}

/*
 * set_variable: The SET command sets one of the irc variables.  The args
 * should consist of "variable-name setting", where variable name can be
 * partial, but non-ambbiguous, and setting depends on the variable being set 
 */
/*ARGSUSED*/
void
set_variable(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*var;
	int	no_args = 1,
		cnt,
		var_index;

	if ((var = next_arg(args, &args)) != NULL)
	{
		if (*var == '-')
		{
			var++;
			args = (char *) 0;
		}
		var_index = find_variable(var, &cnt);
		switch (cnt)
		{
		case 0:
			say("No such variable \"%s\"", var);
			return;
		case 1:
			set_var_value(var_index, args);
			return;
		default:
			say("%s is ambiguous", var);
			for (cnt += var_index; var_index < cnt; var_index++)
				set_var_value(var_index, empty_string);
			return;
		}
	/* not supposed to get here anyway
	 * no_args = 0;
	 */
	}
	if (no_args)
	{
		for (var_index = 0; var_index < NUMBER_OF_VARIABLES; var_index++)
			set_var_value(var_index, empty_string);
	}
}

/*
 * get_string_var: returns the value of the string variable given as an index
 * into the variable table.  Does no checking of variable types, etc 
 */
char	*
get_string_var(var)
	int	var;
{
	return (irc_variable[var].string);
}

/*
 * get_int_var: returns the value of the integer string given as an index
 * into the variable table.  Does no checking of variable types, etc 
 */
int
get_int_var(var)
	int	var;
{
	return (irc_variable[var].integer);
}

/*
 * set_string_var: sets the string variable given as an index into the
 * variable table to the given string.  If string is null, the current value
 * of the string variable is freed and set to null 
 */
void
set_string_var(var, string)
	int	var;
	char	*string;
{
	if (string)
		malloc_strcpy(&(irc_variable[var].string), string);
	else
		new_free(&(irc_variable[var].string));
}

/*
 * set_int_var: sets the integer value of the variable given as an index into
 * the variable table to the given value 
 */
void
set_int_var(var, value)
	int	var;
	unsigned int	value;
{
	if (var == NOVICE_VAR && !load_depth && !value)
	{
say("WARNING: Setting NOVICE to OFF enables commands in your client which");
say("         could be used by others on IRC to control your IRC session");
say("         or compromise security on your machine. If somebody has");
say("         asked you to do this, and you do not know EXACTLY why, or if");
say("         you are not ABSOLUTELY sure what you are doing, you should");
say("         immediately /SET NOVICE ON and ask the IRC operators about");
say("         the commands you have been asked to enter on channel");
say("         #Twilight_Zone.");
	}
	irc_variable[var].integer = value;
}

/*
 * save_variables: this writes all of the IRCII variables to the given FILE
 * pointer in such a way that they can be loaded in using LOAD or the -l switch 
 */
void
save_variables(fp, do_all)
	FILE	*fp;
	int	do_all;
{
	IrcVariable *var;

	for (var = irc_variable; var->name; var++)
	{
		if (!(var->int_flags & VIF_CHANGED))
			continue;
		if (do_all || !(var->int_flags & VIF_GLOBAL))
		{
			if (strcmp(var->name, "DISPLAY") == 0 || strcmp(var->name, "CLIENT_INFORMATION") == 0)
				continue;
			fprintf(fp, "SET ");
			switch (var->type)
			{
			case BOOL_TYPE_VAR:
				fprintf(fp, "%s %s\n", var->name, var->integer ?
					var_settings[ON] : var_settings[OFF]);
				break;
			case CHAR_TYPE_VAR:
				fprintf(fp, "%s %c\n", var->name, var->integer);
				break;
			case INT_TYPE_VAR:
				fprintf(fp, "%s %u\n", var->name, var->integer);
				break;
			case STR_TYPE_VAR:
				if (var->string)
					fprintf(fp, "%s %s\n", var->name,
						var->string);
				else
					fprintf(fp, "-%s\n", var->name);
				break;
			}
		}
	}
}


int
variable_get(var_name, set_var)
	char		*var_name;
	IrcVariable	*set_var;
{
	IrcVariable	*var;
	int		i = 0;

	for (var = irc_variable; var->name; var++)
	{
		if (!my_stricmp(var_name, var->name))
		{
			if (set_var)
			{
				set_var->name = var->name;
				set_var->type = var->type;
				set_var->integer = var->integer;
				malloc_strcpy(&set_var->string, var->string);
				set_var->func = var->func;
				set_var->int_flags = var->int_flags;
				set_var->flags = var->flags;
			}
			return i;
		}
		i++;
	}
	return -1;
}


int
variable_set(var_name, set_var)
	char		*var_name;
	IrcVariable	*set_var;
{
	IrcVariable	*var;
	int		i = 0;

	for (var = irc_variable; var->name; var++)
	{
		if (!my_stricmp(var_name, var->name))
		{
			if (set_var)
			{
				var->type = set_var->type;
				var->integer = set_var->integer;
				malloc_strcpy(&var->string, set_var->string);
				var->func = set_var->func;
				if (var->func)
				{
					var->func();
				}
				var->int_flags = set_var->int_flags;
				var->flags = set_var->flags;
			}
			return i;
		}
		i++;
	}
	return -1;
}


char	*
make_string_var(var_name)
	char	*var_name;
{
	int	cnt,
		var_index;
	char	buffer[BIG_BUFFER_SIZE + 1],
		*ret = (char *) 0;

	if (((var_index = find_variable(var_name, &cnt)) == -1) ||
	    (cnt > 1) ||
	    my_stricmp(var_name,irc_variable[var_index].name))
		return ((char *) 0);
	switch (irc_variable[var_index].type)
	{
	case STR_TYPE_VAR:
		malloc_strcpy(&ret, irc_variable[var_index].string);
		break;
	case INT_TYPE_VAR:
		sprintf(buffer, "%u", irc_variable[var_index].integer);
		malloc_strcpy(&ret, buffer);
		break;
	case BOOL_TYPE_VAR:
		malloc_strcpy(&ret, var_settings[irc_variable[var_index].integer]);
		break;
	case CHAR_TYPE_VAR:
		sprintf(buffer, "%c", irc_variable[var_index].integer);
		malloc_strcpy(&ret, buffer);
		break;
	}
	return (ret);

}

/* exec_warning: a warning message displayed whenever EXEC_PROTECTION is turned off.  */
static	void
exec_warning(value)
	int	value;
{
	if (value == OFF)
	{
		say("Warning!  You have turned EXEC_PROTECTION off");
		say("Please read the /HELP SET EXEC_PROTECTION documentation");
	}
}

static	void
input_warning(value)
	int	value;
{
	if (value == OFF)
	{
		say("Warning!  You have turned INPUT_PROTECTION off");
		say("Please read the /HELP ON INPUT, and /HELP SET INPUT_PROTECTION documentation");
	}
}

/* returns the size of the character set */
int
charset_size()
{
	return get_int_var(EIGHT_BIT_CHARACTERS_VAR) ? 256 : 128;
}

static	void
eight_bit_characters(value)
	int	value;
{
	if (value == ON && !term_eight_bit())
		say("Warning!  Your terminal says it does not support eight bit characters");
	set_term_eight_bit(value);
}

static	void
set_realname(value)
	char	*value;
{
	strmcpy(realname, value, REALNAME_LEN);
}

static	void
set_auto_reoper(value)
	int	value;
{
	if (value == OFF)
	{
		clear_all_server_operator_pwd();
	}
}

static	void
set_ops_method_var(value)
	char	*value;
{
	int len;

	if (value)
	{
		len = strlen(value);
	}

	if (!value || !my_strnicmp(value, "DEFAULT", len))
	{
		set_ops_method(OPS_METHOD_DEFAULT);
		set_string_var(OPS_METHOD_VAR, "DEFAULT");
	}
	else if (!my_strnicmp(value, "HYBRID", len))
	{
		set_ops_method(OPS_METHOD_HYBRID);
		set_string_var(OPS_METHOD_VAR, "HYBRID");
	}
	else if (!my_strnicmp(value, "WALLCHOPS", len))
	{
		set_ops_method(OPS_METHOD_WALLCHOPS);
		set_string_var(OPS_METHOD_VAR, "WALLCHOPS");
	}
	else if (!my_strnicmp(value, "AUTO", len))
	{
		set_ops_method(OPS_METHOD_AUTO);
		set_string_var(OPS_METHOD_VAR, "AUTO");
	}
	else
	{
		say("Unknown /OPS method: %s", value);
		set_ops_method(OPS_METHOD_DEFAULT);
		set_string_var(OPS_METHOD_VAR, "DEFAULT");
	}
}

