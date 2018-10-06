/* 'new' config.h:
 *	A configuration file designed to make best use of the abilities
 *	of ircII, and trying to make things more intuitively understandable.
 *
 * Done by Carl v. Loesch <lynx@dm.unirm1.it>
 * Based on the 'classic' config.h by Michael Sandrof.
 * Copyright(c) 1991 - See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * Warning!  You will most likely have to make changes to your .ircrc file to
 * use this version of IRCII!  Please read the INSTALL and New2.2 files
 * supplied with the distribution for details!
 *
 */

#ifndef __config_h_
#define __config_h_

/*
 * Set your favorite default server list here.  This list should be a
 * whitespace separated hostname:portnum:password list (with portnums and
 * passwords optional).  This IS NOT an optional definition. Please set this
 * to your nearest servers.  However if you use a seperate 'ircII.servers'
 * file and the ircII can find it, this setting is overridden.
 */
#define DEFAULT_SERVER	      "irc.blackened.com"

/*
 * Uncomment the following if the gecos field of your /etc/passwd has other
 * information in it that you don't want as the user name (such as office
 * locations, phone numbers, etc).  The default delimiter is a comma, change
 * it if you need to. If commented out, the entire gecos field is used. 
 */
#define GECOS_DELIMITER ','

/*
 * Control the number of nicks to keep track of for the tab key auto msg
 * feature. (Toast 9/26/97)
 */
#define AUTOMSGLISTMAX 10

/*
 * Define the following if you want to disable all DCC features.
 *    (Toast 1/10/99)
 */
#undef DISABLE_DCC

/*
 * File I/O functions keep an internal array to translate the "fd" to
 * a FILE* structure.  This sets how many files you can have open at
 * the same time.
 */
#define MAX_FILES 16

/*
 * Set the following to 1 if you wish for IRCII not to disturb the tty's flow
 * control characters as the default.  Normally, these are ^Q and ^S.  You
 * may have to rebind them in IRCII.  Set it to 0 for IRCII to take over the
 * tty's flow control.
 */
#define USE_FLOW_CONTROL 1

/*
 * Below you can set what type of mail your system uses and the path to the
 * appropriate mail file/directory.  Only one may be selected. 
 * You may also undefine both if you think mail checking in irc clients is
 * silly.
 */
/* AMS_MAIL is the Andrew Mail System mail format. */

#undef AMS_MAIL

/* UNIX_MAIL is the normal unix mail format.  */
#ifndef UNIX_MAIL
# define UNIX_MAIL "/usr/spool/mail"	/* */
#endif

/*
 * MAIL_DELIMITER specifies the unique text that separates one mail message
 * from another in the mail spool file when using UNIX_MAIL.
 */
#define MAIL_DELIMITER "From "

#ifdef AMS_MAIL
# define AMS_MAIL "Mailbox"
#endif /* AMS_MAIL */
/* Thanks to Allanon a very useful feature, when this is defined, ircII will
 * be able to read help files in compressed format (it will recognize the .Z)
 * If you undefine this you will spare some code, though, so better only
 * set if you are sure you are going to keep your help-files compressed.
 */
#define ZCAT "/usr/ucb/zcat"

/* Define ZSUFFIX in case we are using ZCAT */
#ifdef ZCAT
# define ZSUFFIX ".Z"
#endif

/* Make ^Z stop the irc process by default,
 * if undefined, ^Z will self-insert by default
 */
#define ALLOW_STOP_IRC /**/

/* And here is the port number for default client connections.  */
#define IRC_PORT 6667

/*
 * Uncomment the following to make ircII read a list of irc servers from
 * the ircII.servers file in the ircII library. This file should be
 * whitespace separated hostname:portnum:password (with the portnum and
 * password being optional). This server list will supercede the
 * DEFAULT_SERVER. 
*/

#define SERVERS_FILE "ircII.servers"

/* Uncomment the following if you want ircII to display the file
 * ircII.motd in the ircII library at startup.
 */
#define MOTD_FILE "ircII.motd"
#define PAUSE_AFTER_MOTD 1

/*
 * We don't want to catch signals that cause coredumps.
 */
#undef CORECATCH

/*
 * set this is you want have debugging in the client... really pretty
 * boring..and mostly does nothing.
 */
#undef DEBUG

/*
 * define this if you are on a machine that dynamically changes ip
 * address, such as a floating slip machine.
 */

#undef DYNAMIC_SLIP

/*
 * Below are the IRCII variable defaults.  For boolean variables, use 1 for
 * ON and 0 for OFF.  You may set string variable to NULL if you wish them to
 * have no value.  None of these are optional.  You may *not* comment out or
 * remove them.  They are default values for variables and are required for
 * proper compilation.
 */

#define DEFAULT_SHOW_CTCP_IDLE 1
#define DEFAULT_PING_TYPE 1
#define DEFAULT_FAKE_HOST ""
#define DEFAULT_HACKPASS ""
#define DEFAULT_MSGLOG 1
#define DEFAULT_MSGLOGFILE ".MsgLog"
#define DEFAULT_KILLLOGFILE ".KillLog"
#define DEFAULT_PUBLOGSTR ""
#define DEFAULT_AUTO_NSLOOKUP 0
#define DEFAULT_KICK_REASON ""
#define DEFAULT_REASON_TYPE 1
#define DEFAULT_AUTO_REJOIN 1
#define DEFAULT_SHOW_SERVER_KILLS 1
#define DEFAULT_SHOW_UNAUTHS 1
#define DEFAULT_SHOW_FAKES 1
#define DEFAULT_SHOW_TOOMANY 1
#define DEFAULT_LONG_MSG 1
#define DEFAULT_MAX_WALL_NICKS 20
#define DEFAULT_OPS_METHOD "AUTO"

#define DEFAULT_AUTOOP_ENABLE 1
#define DEFAULT_AUTOOP_DELAY 3
#define DEFAULT_OLD_WHOIS_FORMAT 0
#define DEFAULT_SERVER_NOTICE_WINDOW "OV"
#define DEFAULT_USERHOST_NOTIFY 1
#define DEFAULT_VERBOSE_WHOKILL 1
#define DEFAULT_SHOW_ILINE_FULL 1
#define DEFAULT_SHOW_MY_WALLOPS 0
#define DEFAULT_IGN_PARAM "MSGS NOTICES CTCPS INVITES 60"
#define DEFAULT_BAN_TIMEOUT 1440
#define DEFAULT_SHOW_ADMIN_REQUESTS 1
#define DEFAULT_SHOW_INFO_REQUESTS 1
#define DEFAULT_SHOW_LINKS_REQUESTS 1
#define DEFAULT_SHOW_MOTD_REQUESTS 1
#define DEFAULT_SHOW_STATS_REQUESTS 1
#define DEFAULT_SHOW_TRACE_REQUESTS 1
#define DEFAULT_AWAY_RECORDER 0
#define DEFAULT_LOOP_RECONNECT 1
#define DEFAULT_AUTO_ACCEPT_DCC_CHAT 0
#define DEFAULT_AUTO_ACCEPT_DCC_SEND 0
#define DEFAULT_AUTO_REOPER 1
#define DEFAULT_REJOIN_BADKEY 0
#define DEFAULT_REJOIN_BANNED 0
#define DEFAULT_REJOIN_FULL 0
#define DEFAULT_REJOIN_INTERVAL 60
#define DEFAULT_REJOIN_INVITEONLY 0
#define DEFAULT_REJOIN_UNAVAILABLE 1
#define DEFAULT_BANNER "*** "
#define DEFAULT_URLLOG_FILE ".URLLog"
#define DEFAULT_URLLOG_LEVEL "PUBLIC MSGS NOTICES OPNOTES ACTIONS DCC"

#define DEFAULT_ALWAYS_SPLIT_BIGGEST 1
#define DEFAULT_AUTO_UNMARK_AWAY 0
#define DEFAULT_AUTO_WHOWAS 1
#define DEFAULT_BEEP 1
#define DEFAULT_BEEP_MAX 3
#define DEFAULT_BEEP_ON_MSG "NONE"
#define DEFAULT_BEEP_WHEN_AWAY 0
#define DEFAULT_BLINK_VIDEO 1
#define DEFAULT_BOLD_VIDEO 1
#define DEFAULT_CHANNEL_NAME_WIDTH 10
#define DEFAULT_CLOCK 1
#define DEFAULT_CLOCK_24HOUR 1
#define DEFAULT_CLOCK_ALARM NULL
#define DEFAULT_CMDCHARS "/"
#define DEFAULT_COMMAND_MODE 0
#define DEFAULT_CONTINUED_LINE " "
#define DEFAULT_DCC_BLOCK_SIZE 512
#define DEFAULT_DISPLAY 1
#define DEFAULT_EIGHT_BIT_CHARACTERS 1
#define DEFAULT_ENCRYPT_PROGRAM NULL
#define DEFAULT_EXEC_PROTECTION 1
#define DEFAULT_FILTER_LOG 0
#define DEFAULT_FLOOD_AFTER 3
#define DEFAULT_FLOOD_RATE 3
#define DEFAULT_FLOOD_USERS 3
#define DEFAULT_FLOOD_WARNING 0
#define DEFAULT_FULL_STATUS_LINE 1
#define DEFAULT_HELP_PAGER 1
#define DEFAULT_HELP_PROMPT 1
#define DEFAULT_HELP_WINDOW 0
#define DEFAULT_HIDE_PRIVATE_CHANNELS 0
#define DEFAULT_HIGHLIGHT_CHAR "BOLD"
#define DEFAULT_HISTORY 30
#define DEFAULT_HISTORY_FILE NULL
#define DEFAULT_HOLD_MODE 0
#define DEFAULT_HOLD_MODE_MAX 0
#define DEFAULT_INDENT 1
#define DEFAULT_INPUT_ALIASES 0
#define DEFAULT_INPUT_PROMPT NULL
#define DEFAULT_INPUT_PROTECTION 1
#define DEFAULT_INSERT_MODE 1
#define DEFAULT_INVERSE_VIDEO 1
#define DEFAULT_LASTLOG 1000
#define DEFAULT_LASTLOG_LEVEL "ALL"
#define DEFAULT_LOG 0
#define DEFAULT_LOGFILE "IrcLog"
#define DEFAULT_MAIL 2
#define DEFAULT_MAX_RECURSIONS 100
#define DEFAULT_MINIMUM_SERVERS 0
#define DEFAULT_MINIMUM_USERS 0
#define DEFAULT_NO_CTCP_FLOOD 1
#define DEFAULT_NOTIFY_HANDLER "QUIET"
#define DEFAULT_NOTIFY_LEVEL "ALL DCC"
#define DEFAULT_NOTIFY_ON_TERMINATION 0
#define DEFAULT_SCROLL 1
#define DEFAULT_SCROLL_LINES 1
#define DEFAULT_SEND_IGNORE_MSG 1
#define DEFAULT_SHELL "/bin/sh"
#define DEFAULT_SHELL_FLAGS "-c"
#define DEFAULT_SHELL_LIMIT 0
#define DEFAULT_SHOW_AWAY_ONCE 1
#define DEFAULT_SHOW_CHANNEL_NAMES 1
#define DEFAULT_SHOW_END_OF_MSGS 0
#define DEFAULT_SHOW_NUMERICS 0
#define DEFAULT_SHOW_STATUS_ALL 0
#define DEFAULT_SHOW_WHO_HOPCOUNT 1
#define DEFAULT_STATUS_AWAY " (away)"
#define DEFAULT_STATUS_CHANNEL " on %C"
#define DEFAULT_STATUS_CHANOP "@"
#define DEFAULT_STATUS_CHANVOICE "+"
#define DEFAULT_STATUS_CLOCK " %T"
#define DEFAULT_STATUS_FORMAT "[%R]%T %*%-%@%N%#%S%H%B%Q%A%G%C%+%I%O%M%F%U %W"
#define DEFAULT_STATUS_HOLD " --- more ---"
#define DEFAULT_STATUS_HOLD_LINES " (%B)"
#define DEFAULT_STATUS_INSERT ""
#define DEFAULT_STATUS_MAIL " [Mail: %M]"
#define DEFAULT_STATUS_MODE " (+%+)"
#define DEFAULT_STATUS_MSGS " [Msgs: %G]"
#define DEFAULT_STATUS_NOTIFY " [Activity: %F]"
#define DEFAULT_STATUS_OPER "*"
#define DEFAULT_STATUS_OVERWRITE "(overtype) "
#define DEFAULT_STATUS_QUERY " [Query: %Q]"
#define DEFAULT_STATUS_SERVER " via %S"
#define DEFAULT_STATUS_UMODE " (+%#)"
#define DEFAULT_STATUS_USER " * Blackened "
#define DEFAULT_STATUS_USER1 ""
#define DEFAULT_STATUS_USER2 ""
#define DEFAULT_STATUS_USER3 ""
#define DEFAULT_STATUS_WINDOW "^^^^^^^^"
#define DEFAULT_SUPPRESS_SERVER_MOTD 0
#define DEFAULT_TAB 1
#define DEFAULT_TAB_MAX 8
#define DEFAULT_TIMESTAMP "%d-%b:%H:%M"
#define DEFAULT_UNDERLINE_VIDEO 1
#define DEFAULT_USERINFO ""
#define DEFAULT_USER_WALLOPS 1
#define DEFAULT_VERBOSE_CTCP 1
#define DEFAULT_WARN_OF_IGNORES 0
#define DEFAULT_XTERM_OPTIONS NULL

#define FDEF_ACTION			"[%!%@%!] <*> %F %M"
#define FDEF_ACTION_OTHER		"[%!%@%!] (%T) * %F %M"
#define FDEF_CTCP			"%B[%!%@%!] CTCP %C from %F to %T: %M"
#define FDEF_CTCP_PRIVATE		"%B[%!%@%!] CTCP %C from %F (%U): %M"
#define FDEF_CTCP_REPLY			"%B[%!%@%!] CTCP %C reply from %F: %M"
#define FDEF_CTCP_UNKNOWN		"%B[%!%@%!] Unknown CTCP %C from %F to %T: %M"
#define FDEF_DCC_CHAT			"[%@] =%F= %M"
#define FDEF_DCC_CHAT_ACTION		"[%@] => %F %M"
#define FDEF_DCC_TALK			"[%@] +%F+ %M"
#define FDEF_INVITE			"%B[%!%@%!] %F (%*) invites you to channel %C"
#define FDEF_GONE			"%M [%@]"
#define FDEF_JOIN			"%B[%@] Joined %C: %F (%U)"
#define FDEF_JOIN_SELF			"%B[%@] Joined %C: %F (%U)"
#define FDEF_KICK			"%B[%@] %T kicked from %C by %F (%M)"
#define FDEF_KICK_SELF			"%B[%@] Kicked from %C by %F (%M)"
#define FDEF_KILL			"%B[%@] -- \002KILL\002 for %T by %F on %S %M"
#define FDEF_MODE			"%B[%@] Mode on %C by %F: %M"
#define FDEF_MSG			"[%!%@%!] \026*%F!%U*\026 %M"
#define FDEF_MSG_GROUP			"[%!%@%!] \026*%F:%T*\026 %M"
#define FDEF_NICK			"%B[%@] %F is now known as %N"
#define FDEF_NOTICE			"[%!%@%!] -%F!%U- %M"
#define FDEF_NOTIFY_SIGNOFF		"%B[%@] Signoff by %N detected"
#define FDEF_NOTIFY_SIGNON		"%B[%@] Signon by %N detected"
#define FDEF_NOTIFY_SIGNON_USERHOST	"%B[%@] Signon by %N (%*) detected"
#define FDEF_OPS			"[%!%@ %F:Ops(%T)%!] %M"
#define FDEF_OPS_OTHER			"[%!%@ %F:Ops(%T)%!] %M"
#define FDEF_PART			"%B[%@] Left %C: %F (%U)"
#define FDEF_PART_SELF			"%B[%@] Left %C: %F (%U)"
#define FDEF_PUBLIC			"[%!%@ %F%!] %M"
#define FDEF_PUBLIC_ACTION		"[%!%@%!] * %F %M"
#define FDEF_PUBLIC_MSG			"(%!%@ %F/%T%!) %M"
#define FDEF_PUBLIC_NOTICE		"[%!%@%!] -%F:%T- %M"
#define FDEF_PUBLIC_OTHER		"[%!%@ %F:%T%!] %M"
#define FDEF_RECORDER_BODY		"> %M"
#define FDEF_RECORDER_HEAD		"From \002%F\002 (%*) at %D"
#define FDEF_SEND_ACTION		"[%@] (%T) \002*\002 %F %M"
#define FDEF_SEND_DCC_CHAT		"[%@] -> =%T= %M"
#define FDEF_SEND_DCC_TALK		"[%@] -> +%T+ %M"
#define FDEF_SEND_LOCOPS		"%B[%@] WALLOPS: \026!%F!\026 LOCOPS - %M"
#define FDEF_SEND_MSG			"[%@] -> \026*%T*\026 %M"
#define FDEF_SEND_NOTICE		"[%@] -> -%T- %M"
#define FDEF_SEND_OPERWALL		"%B[%@] WALLOPS: \026!%F!\026 OPERWALL - %M"
#define FDEF_SEND_OPS			"\002[\002%@ %F:Ops(%T)\002]\002 %M"
#define FDEF_SEND_OPS_OTHER		"\002[\002%@ %F:Ops(%T)\002]\002 %M"
#define FDEF_SEND_PUBLIC		"\002[\002%@ %F\002]\002 %M"
#define FDEF_SEND_PUBLIC_ACTION		"[%@] \002*\002 %F %M"
#define FDEF_SEND_PUBLIC_MSG		"\002(\002%@ %F/%T\002)\002 %M"
#define FDEF_SEND_PUBLIC_OTHER		"\002[\002%@ %F:%T\002]\002 %M"
#define FDEF_SEND_WALLOPS		"%B[%@] WALLOPS: \026!%F!\026 WALLOPS - %M"
#define FDEF_SERVER_KILL		"%B[%@] -- \002SERVER KILL\002 for %T by %F"
#define FDEF_SERVER_NOTICE		"%B[%@] %M"
#define FDEF_SERVER_WALLOPS		"%B[%@] WALLOPS: \026!%F!\026 %M"
#define FDEF_TOPIC			"%B[%@] Topic by %F on %C: %M"
#define FDEF_QUIT			"%B[%@] Signoff: %F (%M)"
#define FDEF_UMODE			"%B[%@] Mode on %T by %F: %M"
#define FDEF_WALL			"[%!%@%!] #%F# %M"
#define FDEF_WALLOPS			"%B[%!%@%!] WALLOPS: \026!%F(%U)!\026 %M"
#define FDEF_WHOIS_ADMIN		":\037 admin    \037: %N %M"
#define FDEF_WHOIS_AWAY			":\037 away set \037: %M"
#define FDEF_WHOIS_CHANNELS		":\037 channels \037: %M"
#define FDEF_WHOIS_IDLE			":\037 idle for \037: %M"
#define FDEF_WHOIS_IRCNAME		":\037 ircname  \037: %M"
#define FDEF_WHOIS_NSLOOKUP		":\037 nslookup \037: %I -> %H"
#define FDEF_WHOIS_OPER			":\037 operator \037: %N %M"
#define FDEF_WHOIS_SERVER		":\037 server   \037: %S (%M)"
#define FDEF_WHOIS_SIGNON		":\037 sign on  \037: %M"
#define FDEF_WHOIS_USERHOST		"\002[\002 Whois \002%N\002!%I@%H \002]\002"
#define FDEF_WHOWAS_USERHOST		"\002[\002 Whowas \002%N\002!%I@%H \002]\002"

/*
 * define this if you want to have the -l and -L command line
 * options.
 */

#define COMMAND_LINE_L
#define COMMAND_LINE_B

#endif /* __config_h_ */
