/*
 * vars.h: header for vars.c
 *
 */

#ifndef _VARS_H_
#define _VARS_H_


/* IrcVariable: structure for each variable in the variable table */
typedef struct
{
	char	*name;			/* what the user types */
	int	type;			/* variable types, see below */
	int	integer;		/* int value of variable */
	char	*string;		/* string value of variable */
	void	(*func)();		/* function to do every time variable is set */
	char	int_flags;		/* internal flags to the variable */
	unsigned short	flags;		/* flags for this variable */
}	IrcVariable;

/* the types of IrcVariables */
#define BOOL_TYPE_VAR 0
#define CHAR_TYPE_VAR 1
#define INT_TYPE_VAR 2
#define STR_TYPE_VAR 3


extern	int	do_boolean();
extern	void	set_variable _((char *, char *, char *));
extern	int	get_int_var();
extern	char	*get_string_var();
extern	void	set_int_var();
extern	void	set_string_var();
extern	char	*get_string_var();
extern	void	init_variables();
extern	char	*var_settings[];
extern	char	*make_string_var();
extern	int	variable_get _((char *, IrcVariable *));
extern	int	variable_set _((char *, IrcVariable *));
extern	void	set_highlight_char();
extern	int	charset_size();
extern	void	save_variables();
extern	void	set_var_value();

extern	int	loading_global;

/* var_settings indexes ... also used in display.c for highlights */
#define OFF 0
#define ON 1
#define TOGGLE 2

#define	DEBUG_COMMANDS		0x0001
#define	DEBUG_EXPANSIONS	0x0002
#define DEBUG_FUNCTIONS		0x0004

/* indexes for the irc_variable array */
enum VAR_TYPES {
	ALWAYS_SPLIT_BIGGEST_VAR,
	AUTO_ACCEPT_DCC_CHAT_VAR,
	AUTO_ACCEPT_DCC_SEND_VAR,
	AUTO_NSLOOKUP_VAR,
	AUTO_REJOIN_VAR,
	AUTO_REOPER_VAR,
	AUTO_UNMARK_AWAY_VAR,
	AUTO_WHOWAS_VAR,
	AUTOOP_DELAY_VAR,
	AUTOOP_ENABLE_VAR,
	AWAY_RECORDER_VAR,
	BAN_TIMEOUT_VAR,
	BANNER_VAR,
	BEEP_VAR,
	BEEP_MAX_VAR,
	BEEP_ON_MSG_VAR,
	BEEP_WHEN_AWAY_VAR,
	BLINK_VIDEO_VAR,
	BOLD_VIDEO_VAR,
	CHANNEL_NAME_WIDTH_VAR,
	CLIENTINFO_VAR,
	CLOCK_VAR,
	CLOCK_24HOUR_VAR,
	CLOCK_ALARM_VAR,
	CMDCHARS_VAR,
	COMMAND_MODE_VAR,
	CONTINUED_LINE_VAR,
	DCC_BLOCK_SIZE_VAR,
	DEBUG_VAR,
	DEFAULT_REASON_VAR,
	DISPLAY_VAR,
	EIGHT_BIT_CHARACTERS_VAR,
	ENCRYPT_PROGRAM_VAR,
	EXEC_PROTECTION_VAR,
	FAKE_HOST_VAR,
	FILTER_LOG_VAR,
	FLOOD_AFTER_VAR,
	FLOOD_RATE_VAR,
	FLOOD_USERS_VAR,
	FLOOD_WARNING_VAR,
	FULL_STATUS_LINE_VAR,
	HELP_FILE_VAR,
	HELP_PAGER_VAR,
	HELP_PATH_VAR,
	HELP_PROMPT_VAR,
	HELP_WINDOW_VAR,
	HIDE_PRIVATE_CHANNELS_VAR,
	HIGHLIGHT_CHAR_VAR,
	HISTORY_VAR,
	HISTORY_FILE_VAR,
	HOLD_MODE_VAR,
	HOLD_MODE_MAX_VAR,
	IGN_PARAM_VAR,
	INDENT_VAR,
	INPUT_ALIASES_VAR,
	INPUT_PROMPT_VAR,
	INPUT_PROTECTION_VAR,
	INSERT_MODE_VAR,
	INVERSE_VIDEO_VAR,
	KILLLOGFILE_VAR,
	LAST_MSG_VAR,
	LAST_NOTICE_VAR,
	LAST_SENDMSG_VAR,
	LAST_SENDNOTICE_VAR,
	LASTLOG_VAR,
	LASTLOG_LEVEL_VAR,
	LOAD_PATH_VAR,
	LOG_VAR,
	LOGFILE_VAR,
	LOOP_RECONNECT_VAR,
	MAIL_VAR,
	MAX_RECURSIONS_VAR,
	MAX_WALL_NICKS_VAR,
	MENU_VAR,
	MINIMUM_SERVERS_VAR,
	MINIMUM_USERS_VAR,
	MSGLOG_VAR,
	MSGLOGFILE_VAR,
	NO_CTCP_FLOOD_VAR,
	NO_LOG_VAR,
	NOTIFY_HANDLER_VAR,
	NOTIFY_LEVEL_VAR,
	NOTIFY_ON_TERMINATION_VAR,
	NOVICE_VAR,
	NUM_BANMODES_VAR,
	NUM_OPMODES_VAR,
	OLD_WHOIS_FORMAT_VAR,
	OPS_METHOD_VAR,
	PING_TYPE_VAR,
	PUBLOGSTR_VAR,
	REALNAME_VAR,
	REASON_TYPE_VAR,
	REJOIN_BADKEY_VAR,
	REJOIN_BANNED_VAR,
	REJOIN_FULL_VAR,
	REJOIN_INTERVAL_VAR,
	REJOIN_INVITEONLY_VAR,
	REJOIN_UNAVAILABLE_VAR,
	SCREEN_OPTIONS_VAR,
	SCROLL_VAR,
	SCROLL_LINES_VAR,
	SEND_IGNORE_MSG_VAR,
	SERVER_NOTICE_WINDOW_VAR,
	SHELL_VAR,
	SHELL_FLAGS_VAR,
	SHELL_LIMIT_VAR,
	SHOW_ADMIN_REQUESTS_VAR,
	SHOW_AWAY_ONCE_VAR,
	SHOW_CHANNEL_NAMES_VAR,
	SHOW_CTCP_IDLE_VAR,
	SHOW_END_OF_MSGS_VAR,
	SHOW_FAKES_VAR,
	SHOW_ILINE_FULL_VAR,
	SHOW_INFO_REQUESTS_VAR,
	SHOW_LINKS_REQUESTS_VAR,
	SHOW_MOTD_REQUESTS_VAR,
	SHOW_MY_WALLOPS_VAR,
	SHOW_NUMERICS_VAR,
	SHOW_SERVER_KILLS_VAR,
	SHOW_STATS_REQUESTS_VAR,
	SHOW_STATUS_ALL_VAR,
	SHOW_TOOMANY_VAR,
	SHOW_TRACE_REQUESTS_VAR,
	SHOW_UNAUTHS_VAR,
	SHOW_WHO_HOPCOUNT_VAR,
	STATUS_AWAY_VAR,
	STATUS_CHANNEL_VAR,
	STATUS_CHANOP_VAR,
	STATUS_CHANVOICE_VAR,
	STATUS_CLOCK_VAR,
	STATUS_FORMAT_VAR,
	STATUS_HOLD_VAR,
	STATUS_HOLD_LINES_VAR,
	STATUS_INSERT_VAR,
	STATUS_MAIL_VAR,
	STATUS_MODE_VAR,
	STATUS_MSGS_VAR,
	STATUS_NOTIFY_VAR,
	STATUS_OPER_VAR,
	STATUS_OVERWRITE_VAR,
	STATUS_QUERY_VAR,
	STATUS_SERVER_VAR,
	STATUS_UMODE_VAR,
	STATUS_USER_VAR,
	STATUS_USER1_VAR,
	STATUS_USER2_VAR,
	STATUS_USER3_VAR,
	STATUS_WINDOW_VAR,
	SUPPRESS_SERVER_MOTD_VAR,
	TAB_VAR,
	TAB_MAX_VAR,
	TIMESTAMP_VAR,
	TRANSLATION_VAR,
	UNDERLINE_VIDEO_VAR,
	URLLOG_FILE_VAR,
	URLLOG_LEVEL_VAR,
	USE_FAKE_HOST_VAR,
	USER_INFO_VAR,
#define	USERINFO_VAR	USER_INFO_VAR
	USER_WALLOPS_VAR,
	USERHOST_NOTIFY_VAR,
	VERBOSE_CTCP_VAR,
	VERBOSE_WHOKILL_VAR,
	VHOST_VAR,
	WARN_OF_IGNORES_VAR,
	XTERM_OPTIONS_VAR,
	NUMBER_OF_VARIABLES
};

#endif /* _VARS_H_ */
