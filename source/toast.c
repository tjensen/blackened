/*
 * toast.c: This is a just a bunch of routines I wrote.
 *
 * Written By Timothy Jensen
 *
 * Copyright (c) 1999-2002
 *
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: toast.c,v 1.29.2.6 2002/03/14 20:42:59 toast Exp $";
#endif

#include "irc.h"

#include "vars.h"
#include "lastlog.h"
#include "ircaux.h"
#include "screen.h"
#include "window.h"
#include "edit.h"
#include "comstud.h"
#include "toast.h"
#include "format.h"
#include "parse.h"
#include "cignore.h"
#include "whois.h"
#include "dcc.h"
#include "input.h"
#include "names.h"
#include "server.h"
#include "output.h"
#include "numbers.h"
#include "comstud.h"


/******* GLOBALS *******/

int	toast_action = 0;

int	in_whois = 0;
int	bc_count;
char	*bc_chname = 0;

int	page_updown_key = 0;

int	setting_nolog = 0;


/******* LOCALS *******/

#define DUMMY_SIZE	1024

static char	dummy[DUMMY_SIZE];	/* Stupid temporary string */

static char	*auto_msg_list[AUTOMSGLISTMAX];
static int	auto_msg_list_size = 0;
static int	auto_msg_list_pos = 0;

static char	toast_arg[2048];
static int	toast_arg_int;
static int	find_cnt, find_fnd, find_type;

static char	*oops_nick = (char *) 0,
		*oops_line = (char *) 0,
		*oops_command = (char *) 0;

typedef struct away_struct
{
	char *nick;
	char *message;
} Away;

static Away	last_away[10] =
	{{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};

static char	*cs_channel;
static int	cs_total, cs_ops, cs_here, cs_ircops, cs_hops;

static	int	urllog_level;

static	FILE	*urllog_fp = NULL;


/*
 * TimeStamp -- makes a nice stampstamp in the following format:
 *
 * [15-Jul:11:36 mjr] asdf
 *
 */
char *
TimeStamp(void)
{
	static char	ts_holder[64];
	struct tm	*time_val;
	time_t		t;
	char		*ts;

	t = time(NULL);
	time_val = localtime(&t);
	ts = get_string_var(TIMESTAMP_VAR);

	if (ts != NULL)
	{
		strftime(ts_holder, 64, ts, time_val);
		ts_holder[63] = 0;
	}
	else
	{
		ts_holder[0] = 0;
	}

	return (char *)&ts_holder;
}


char *
MakeTimeStamp(thetime)
	long	thetime;
{
	static char	ts_holder[64];
	struct tm	*time_val;

	time_val = localtime((time_t *)&thetime);

	strftime(ts_holder, 64, get_string_var(TIMESTAMP_VAR), time_val);
	ts_holder[63] = 0;

	return (char *)&ts_holder;
}


/* This function converts the time_t value (in string format) received
 * from the IRC server to a human-readable date and time.
 *
 * Hi.  FreeBSD 5 broke this when it was named "long_to_time", which I
 * admit was a stupid name.  Now it's a different stupid name, which
 * hopefully FreeBSD won't break again! :-)
 */
char *
timet_to_human(input)
        char    *input;
{
        char    result[64];
        time_t  ltime;
        struct tm *tm;

        ltime = atol(input);

        tm = localtime(&ltime);
        if (strftime(result, 64, "%c %Z", tm))
        {
                char *  s = (char *) 0;

                malloc_strcpy(&s, result);
                return s;
        } else {
                return (char *) 0;
        }
}


void
channel_created(from, ArgList)
	char	*from,
		**ArgList;
{
	char	*channel,
		*when;
	
	channel = ArgList[0];
	if (is_cignored(channel, CIGNORE_CRAP))
		return;
	when = ArgList[1];
	message_from(channel, LOG_CRAP);
	if (num_format_exists(-current_numeric))
	{
		put_it("%s", numericformat(-current_numeric, from,
			numeric_banner(), ArgList));
	}
	else
	{
		put_it("%sChannel %s created on %s", numeric_banner(), channel,
			timet_to_human(when));
	}
}

void
channel_topictime(from, ArgList)
        char    *from,
                **ArgList;
{
        char    *channel,
		*nick,
                *when;

        channel = ArgList[0];
	if (is_cignored(channel, CIGNORE_CRAP))
		return;
	nick = ArgList[1];
        when = ArgList[2];
        message_from(channel, LOG_CRAP);
	if (num_format_exists(-current_numeric))
	{
		put_it("%s", numericformat(-current_numeric, from,
			numeric_banner(), ArgList));
	}
	else
	{
        	put_it("%sTopic for %s set by %s on %s", numeric_banner(),
			channel, nick, timet_to_human(when));
	}
}


int
Do_Not_Log(from)
	char	*from;
{
	char	*NoLogList;
	char	*NextSpace;
	int	CompLen;

	NoLogList = get_string_var(NO_LOG_VAR);

	if (NoLogList && from)
		while ((NoLogList != NULL) && (NoLogList[0] != 0)) {
			while (NoLogList[0]==' ')
				NoLogList++;

			NextSpace = index(NoLogList, ' ');
			if (!NextSpace)
				CompLen = strlen(NoLogList);
			else
				CompLen = NextSpace - NoLogList;
			if (strlen(from) > CompLen)
				CompLen = strlen(from);

			if (!my_strnicmp(from, NoLogList, CompLen))
				return 1;
			NoLogList = NextSpace;
		}
	return 0;
}

/*
 * NOLOG - Adds/removes nicks to/from the NO_LOG variable.
 *
 * /NOLOG [<nick1> [<nick2> [<nick3> [...]]]]
 *
 */

void
nolog(command, args)
	char	*command,
		*args;
{
	char	*NoLogList,
		*NL_nick, *NLL_nick;

	if (strlen(args)==0) {
		NoLogList = get_string_var(NO_LOG_VAR);
		if (!NoLogList)
			say("NO_LOG is empty!");
		else
			say("Currently not logging: %s", NoLogList);
		return;
	}

	/* Go through each nick specified afer command */
	while (NULL != (NL_nick = next_arg(args, &args)))
	{
		int	flag;

		NoLogList = get_string_var(NO_LOG_VAR);

		dummy[0] = 0;
		flag = 0;
		/* Now check if this nick is already in the list */
		while (NULL != (NLL_nick = next_arg(NoLogList, &NoLogList)))
		{
			int	len;

			if (strlen(NL_nick) > strlen(NLL_nick))
				len = strlen(NL_nick);
			else
				len = strlen(NLL_nick);

			if (!my_strnicmp(NL_nick, NLL_nick, len)) {
				flag = 1;
				say("Removing from NO_LOG: %s", NL_nick);
				logremovenolog(NL_nick);
			} else {
				strlcat(dummy, NLL_nick, DUMMY_SIZE);
				strlcat(dummy, " ", DUMMY_SIZE);
			}
		}
		if (!flag) {
			strlcat(dummy, NL_nick, DUMMY_SIZE);
			say("Adding to NO_LOG: %s", NL_nick);
			logaddnolog(NL_nick);
		}
		setting_nolog = 1;
		set_string_var(NO_LOG_VAR, &dummy);
		setting_nolog = 0;
	}
}

void
set_nolog(data)
	char	*data;
{
	if (!setting_nolog)
		lognolog(data);
}

void
doleave(command, args)
	char	*command,
		*args;
{
	char	*s;

	if ((!strlen(args))||(!strcmp(args, "*")))
	{
		if ((s = get_channel_by_refnum(0)) != NULL)
			send_to_server("PART %s", s);
		else
			say("You aren't on a channel in this window");
	}
	else
		send_to_server("PART %s", args);
}

void
dowho(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (!strlen(args))
		who("WHO", "*", (char) 0);
	else
		who("WHO", args, subargs);
}

void
dowii(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*ptr;

	if ((ptr = index(args, ' ')) != NULL)
		ptr[0] = 0;
	sprintf(dummy, "%s %s", args, args);
	whois("WHOIS", dummy, subargs);
}

void
dotopic(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	if (!args || ((*args != '#') && (*args != '&')))
		sprintf(dummy, "* %s", args ? args : empty_string);
	else
		sprintf(dummy, "%s %s", next_arg(args, &args),
			args ? args : empty_string);
	send_topic("TOPIC", dummy, subargs);
}

void
domode(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	sprintf(dummy, "* %s", args);
	send_channel_com("MODE", dummy, subargs);
}


/*
 * KLINE - Why didn't anyone ever add this command before?
 *
 * Two formats:
 *  /KLINE <target> <reason>
 *  /KLINE <duration> <target> <reason>     (also /TKLINE)
 *
 */
void
dokline(command, args)
	char	*command,
		*args;
{
	char	*duration,
		*target,
		*comma;
	int	tkline,
		dline,
		dur = 0;

	duration = next_arg(args, &args);

	dline = !strcmp(command, "DLINE");

	if (!dline)
		tkline = (!strcmp(command, "TKLINE") || (duration && is_number(duration)));
	else
		tkline = 0;

	if (!dline && tkline) {
		if (!duration || !is_number(duration)) {
			say("Invalid temporary kline duration!");
			return;
		} else
			dur = atoi(duration);
		target = next_arg(args, &args);
	} else
		target = duration;

	if (!target || !strlen(target)) {
		say("Who am I supposed to %cline?", dline ? 'd' : 'k');
		return;
	}

	while (args && (args[0]==':'))
		args++;

	if (!args || !strlen(args)) {
		say("Don't forget that ever-important %cline reason!", dline ? 'd' : 'k');
		return;
	}
	while (target && *target) {
		comma = index(target, ',');
		if (comma)
			*comma++ = '\0';
		if (tkline)
			send_to_server("KLINE %u %s :%s", dur, target, args);
		else
			send_to_server("%cLINE %s :%s", dline ? 'D' : 'K',
				target, args);
		target = comma;
	}
}

void
unkline(command, args)
	char	*command,
		*args;
{
	if (!args || !strlen(args)) {
		say("Please specify a mask to unkline!");
		return;
	}

	send_to_server("UNKLINE %s", args);
}

void
vercmd(command, args)
	char	*command,
		*args;
{
	if (args && *args)
		sprintf(dummy, "%s VERSION", args);
	else
		sprintf(dummy, "* VERSION");
	ctcp(command, dummy, empty_string);
}

void
raw_type(text)
	char	*text;
{
	int	i = 0;

	while (text && (text[i] != 0))
		input_add_character(text[i++], NULL);
}

void
auto_msg(void)
{
	char	*msgnick;
	char	*oldline = (char *) 0;
	char	*oldcmd, *oldnick, *oldrest;
	char	*cmdchars;

	if (!auto_msg_list_size) {
		return;
	}
	
	/* Get the "/MSG" first */
	if ((cmdchars = get_string_var(CMDCHARS_VAR)) == (char *) 0)
		cmdchars = DEFAULT_CMDCHARS;
	sprintf(dummy, "%cMSG", cmdchars[0]);

	malloc_strcpy(&oldline, get_input());
	oldrest = oldline;
	oldcmd = my_next_arg(&oldrest);
	if (oldcmd && *oldcmd && !my_stricmp(dummy, oldcmd))
		oldnick = my_next_arg(&oldrest);
	else
		oldrest = "";

	strcat(dummy, " ");

	msgnick = auto_msg_list[auto_msg_list_pos++];
	if (auto_msg_list_pos >= auto_msg_list_size)
		auto_msg_list_pos = 0;
	strcat(dummy, msgnick);
	strcat(dummy, " ");
	strcat(dummy, oldrest);
	set_input(dummy);
	update_input(UPDATE_ALL);
	new_free(&oldline);
}

void
remnick(anick)
	char	*anick;
{
	int	i,j;

	i = 0;
	while (i < auto_msg_list_size) {
		if (!my_stricmp(anick, auto_msg_list[i])) {
			for (j = i; j < auto_msg_list_size-1; j++)
				auto_msg_list[j] = auto_msg_list[j+1];
			auto_msg_list_size--;
		} else
			i++;
	}
}

void
addnick(anick)
	char	*anick;
{
	int	i;
	char	*temp = (char *) NULL;

	malloc_strcpy(&temp, anick);

	remnick(temp);

	for (i = AUTOMSGLISTMAX-1; i > 0; i--)
		auto_msg_list[i] = auto_msg_list[i-1];
	auto_msg_list[0] = temp;
	if (auto_msg_list_size < AUTOMSGLISTMAX)
		auto_msg_list_size++;
	auto_msg_list_pos = 0;
}

void
adddccnick(anick)
	char	*anick;
{
    /*
     * Prepends a nick with = for DCC chat. This just makes my life
     * a little easier.
     */
	sprintf(dummy, "=%s", anick);
	addnick(dummy);
}

void
do_new_window()
{
	in_window_command = 1;  
	message_from((char *) 0, LOG_CURRENT); 
	new_window();
	in_window_command = 0;
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	cursor_to_input();
}

void
do_kill_window()
{
	in_window_command = 1;  
	message_from((char *) 0, LOG_CURRENT); 
	delete_window(curr_scr_win);
	in_window_command = 0;
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	cursor_to_input();
}

void
do_list_windows()
{
	in_window_command = 1;  
	message_from((char *) 0, LOG_CURRENT); 
	list_windows();
	in_window_command = 0;
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	cursor_to_input();
}

void
do_grow_window()
{
	in_window_command = 1;  
	message_from((char *) 0, LOG_CURRENT); 
	grow_window(curr_scr_win, 1);
	in_window_command = 0;
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	cursor_to_input();
}

void
do_shrink_window()
{
	in_window_command = 1;  
	message_from((char *) 0, LOG_CURRENT); 
	grow_window(curr_scr_win, -1);
	in_window_command = 0;
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	cursor_to_input();
}

void
do_hide_window()
{
	in_window_command = 1;  
	message_from((char *) 0, LOG_CURRENT); 
	hide_window(curr_scr_win);
	in_window_command = 0;
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	cursor_to_input();
}

void
do_opervision(void)
{
	Window	*window;

	window = get_window_by_name(get_string_var(SERVER_NOTICE_WINDOW_VAR));
	
	in_window_command = 1;  
	message_from((char *) 0, LOG_CURRENT); 
	if (!window) {
		window = new_window();
		malloc_strcpy(&window->name, get_string_var(SERVER_NOTICE_WINDOW_VAR));
		window->update |= UPDATE_STATUS;
		window->window_level = LOG_WALLOP | LOG_SNOTE | LOG_OPNOTE;
		revamp_window_levels(window);
	} else
		delete_window(window);
	in_window_command = 0;
	message_from((char *) 0, LOG_CRAP);
	update_all_windows();
	cursor_to_input();
}

void
do_pageup()
{
	page_updown_key = 1;
	scrollback_backwards();
}

void
do_pagedown()
{
	page_updown_key = 1;
	scrollback_forwards();
}

void
handle_findu(class, nuh)
	int	class;
	char	*nuh;
{
	char	*uh;
	if (nuh) {
		find_cnt++;
		if ((toast_arg_int < 0) || (class == toast_arg_int)) {
			uh = index(nuh, '[')+1;
			uh[strlen(uh)-1] = 0;
			if (wild_match(toast_arg, uh)) {
				find_fnd++;
				if (find_type != FIND_COUNT)
					say("FindU: %d %s]", class, nuh);
			}
		}
	} else {
		toast_action = TACTION_NONE;
		if (toast_arg_int < 0)
			say("End of FindU %s (%u users, %u matches)", toast_arg,
				find_cnt, find_fnd);
		else
			say("End of FindU -class %d %s (%u users, %u matches)",
				toast_arg_int, toast_arg, find_cnt, find_fnd);
	}
}

void
findu(command, args)
	char	*command,
		*args;
{
	int len;
	char *thisarg;

	if (toast_action) {
		say("Another operation in progress. Please wait.");
		return;
	}

	toast_arg_int = -1;
	find_type = FIND_NONE;
	while (NULL != (thisarg = my_next_arg(&args)))
	{
		if (*thisarg == '-') {
			thisarg++;
			len = strlen(thisarg);
			if (!my_strnicmp(thisarg, "CLASS", (len>2)?len:2)) {
				if (NULL != (thisarg = my_next_arg(&args)))
					toast_arg_int = atoi(thisarg);
				else
				{
					say("No connection class specified!");
					return;
				}
			} else if (!my_strnicmp(thisarg, "COUNT", (len>2)?len:2)) {
				find_type = FIND_COUNT;
			} else
			{
				say("Unknown or missing flag");
				return;
			}
		} else {
        		strcpy(toast_arg, thisarg);
			toast_action = TACTION_FINDU;
			find_cnt = 0;
			find_fnd = 0;
			if (toast_arg_int < 0)
				say("Starting FindU for %s", toast_arg);
			else
				say("Starting FindU -class %d for %s",
					toast_arg_int, toast_arg);
			send_to_server("TRACE");
			return;
		}
	}
	say("No pattern given for FindU");
	return;

}

/*
:irc2.blackened.com 216 toastwank K plasma.engr.Arizona.EDU * *sphere* PERM this script only is used for abuse and annoyance. (1997/11/17 18.03)
:irc2.blackened.com 219 toastwank K :End of /STATS report
*/
void
handle_findk(parms)
	char	**parms;
{
	int	i;
	char	uh[512];
	char	reason[512];

	if (parms) {
		find_cnt++;
		sprintf(uh, "%s@%s", parms[3], parms[1]);
		/*say("test: %s", uh);*/
		if (wild_match(toast_arg, uh)) {
			find_fnd++;
			if (find_type == FIND_NONE) {
				strcpy(reason, "");
				for (i = 4; parms[i]; i++) {
					if (i > 4) strcat(reason, " ");
					strcat(reason, parms[i]);
				}
				say("K: %s : %s", uh, reason);
			} else if (find_type == FIND_REMOVE)
				send_to_server("UNKLINE %s", uh);
		}
	} else {
		toast_action = TACTION_NONE;
		say("End of FindK %s (%u klines, %u matches)",
			toast_arg, find_cnt, find_fnd);
	}
}

void
findk(command, args)
	char	*command,
		*args;
{
	char *pattern = (char *) 0,
	     *server = (char *) 0;
	char *thisarg;
	int len;

	if (toast_action) {
		say("Another operation in progress. Please wait.");
		return;
	}
	
	find_type = FIND_NONE;
	while (NULL != (thisarg = my_next_arg(&args)))
	{
		if (*thisarg == '-')
		{
			thisarg++;
			len = strlen(thisarg);
			if (!my_strnicmp(thisarg, "COUNT", (len>1)?len:1)) {
				find_type = FIND_COUNT;
			} else if (!my_strnicmp(thisarg, "REMOVE", (len>1)?len:1)) {
				find_type = FIND_REMOVE;
			} else {
				say("Unknown or missing flag");
				return;
			}
		} else {
			if (!pattern)
				pattern = thisarg;
			else if (!server)
				server = thisarg;
		}
	}

	if (!pattern || !*pattern) {
		say("No pattern given for FindK");
		return;
	}
        strcpy(toast_arg, pattern);
	toast_action = TACTION_FINDK;
	find_cnt = 0;
	find_fnd = 0;
	if (server) {
		say("Starting FindK for %s on %s", toast_arg, server);
		send_to_server("STATS K %s", server);
	} else {
		say("Starting FindK for %s", toast_arg);
		send_to_server("STATS K");
	}
}

void
bancount(command, args)
	char	*command,
		*args;
{
	char *tmp_chname = 0;
	if (toast_action) {
		say("Another operation in progress. Please wait.");
		return;
	}
	bc_count = 0;
	tmp_chname = my_next_arg(&args);
	if (!tmp_chname || (!strcmp(tmp_chname, "*")))
		malloc_strcpy(&bc_chname, get_channel_by_refnum(0));
	else
		malloc_strcpy(&bc_chname, tmp_chname);
	if (bc_chname) {
		toast_action = TACTION_BANCOUNT;
		send_to_server("MODE %s +b", bc_chname);
	} else
		say("You aren't in a channel!");
}

void
handle_cstat(stat, hops)
	char	*stat;
	int	hops;
{
	if (stat) {
		cs_total++;
		if (hops > cs_hops)
			cs_hops = hops;
		if (*stat == 'H')
			cs_here++;
		if (*(stat+1) == '*')
			cs_ircops++;
		if ((*(stat+1) == '@') || (*(stat+2) == '@'))
			cs_ops++;
	} else {
		toast_action = TACTION_NONE;
		message_from(cs_channel, LOG_CRAP);
		say("Stats for channel %s:", cs_channel);
		say("Total Users:%u   Ops:%u   Non-ops:%u   IRCops:%u",
		  cs_total, cs_ops, cs_total-cs_ops, cs_ircops);
		say("Here:%u   Away:%u   Server hops to farthest user:%u",
		  cs_here, cs_total-cs_here, cs_hops);
		message_from((char *) 0, LOG_CURRENT);
	}
}

void
cstat(command, args)
	char	*command,
		*args;
{
	if (toast_action) {
		say("Another operation in progress. Please wait.");
		return;
	}
	cs_total = 0;
	cs_hops = 0;
	cs_ops = 0;
	cs_here = 0;
	cs_ircops = 0;
	cs_channel = my_next_arg(&args);
	if (!cs_channel || (!strcmp(cs_channel, "*")))
		malloc_strcpy(&cs_channel, get_channel_by_refnum(0));
	if (cs_channel) {
		toast_action = TACTION_CSTAT;
		send_to_server("WHO %s", cs_channel);
	} else
		say("You aren't in a channel!");
}

void
userhost_notify(stuff, nick, args)
	WhoisStuff *stuff;
	char	*nick,
		*args;
{
        char junk[512];

        if (!stuff || !stuff->nick || !nick || my_stricmp(stuff->nick, nick))
                return;
	sprintf(junk, "%s@%s", stuff->user, stuff->host);
	malloc_strcpy(&FromUserHost, junk);
	put_it("%s", parseformat(NOTIFY_SIGNON_USERHOST_FMT, nick));
}

int
handle_sping(from)
	char *from;
{
	int result;
	SpingStuff *stuff, *laststuff;
	struct timeval tp;
	struct timezone tzp;
	time_t timediff, timediff2;

	if (!from)
		return 1;

	laststuff = (SpingStuff *) 0;
	stuff = server_list[from_server].spingstuff;
	while (stuff) {
		if (wild_match(stuff->server, from)) {
			result = gettimeofday(&tp, &tzp);
			utimediff(tp.tv_sec, tp.tv_usec, stuff->start,
				stuff->ustart, &timediff, &timediff2);
			timediff2 /= 1000;
			if (!laststuff)
				server_list[from_server].spingstuff = stuff->next;
			else
				laststuff->next = stuff->next;
			say("Received PONG from server %s (%d.%0.3d seconds)",
				from, (int)timediff, (int)timediff2);
			new_free(&stuff->server);
			new_free(&stuff);
			return 0;
		}
		laststuff = stuff;
		stuff = stuff->next;
	}
	return 1;
}

void
handle_sping_nosuchserver(from)
	char	*from;
{
	SpingStuff *stuff, *laststuff;

	if (!from)
		return;

	laststuff = 0;
	stuff = server_list[from_server].spingstuff;
	while (stuff) {
		if (!my_stricmp(stuff->server, from)) {
			if (!laststuff)
				server_list[from_server].spingstuff = stuff->next;
			else
				laststuff->next = stuff->next;
			new_free(&stuff->server);
			new_free(&stuff);
			return;
		}
		laststuff = stuff;
		stuff = stuff->next;
	}
}

void
sping(command, args)
	char	*command,
		*args;
{
	int result;
	SpingStuff *stuff,*tempstuff;
	char *server;
	struct timeval tp;
	struct timezone tzp;

	server = my_next_arg(&args);
	if (!server) {
		say("Currently awaiting PONGs from these servers:");
		stuff = server_list[from_server].spingstuff;
		while (stuff) {
			say("     %s",stuff->server);
			stuff = stuff->next;
		}
	} else if (!strcasecmp(server, "-CLEAR")) {
		while (server_list[from_server].spingstuff != 0) {
			stuff = server_list[from_server].spingstuff;
			server_list[from_server].spingstuff = server_list[from_server].spingstuff->next;
			new_free(&stuff->server);
			new_free(&stuff);
		}
		say("Server PING list cleared.");
	} else {
		if (!index(server, '.')) {
			say("%s is not a server.", server);
			return;
		}
		stuff = (SpingStuff *) new_malloc(sizeof(SpingStuff));
		stuff->server = 0;
		malloc_strcpy(&stuff->server, server);
		stuff->next = 0;
		if (!server_list[from_server].spingstuff)
			server_list[from_server].spingstuff = stuff;
		else {
			tempstuff = server_list[from_server].spingstuff;
			while (tempstuff->next)
				tempstuff = tempstuff->next;
			tempstuff->next = stuff;
		}
		say("Sent PING to server %s", server);
		result = gettimeofday(&tp,&tzp);
		stuff->start = tp.tv_sec;
		stuff->ustart = tp.tv_usec;
		send_to_server("VERSION %s", server);
	}
}

void
show_banner(void)
{
	put_it(":");
	put_it(":       \026\002    ____  _        _    ____ _  _______ _   _ _____ ____     \002\026");
	put_it(":       \026\002   | __ )| |      / \\  / ___| |/ / ____| \\ | | ____|  _ \\    \002\026");
	put_it(":       \026\002   |  _ \\| |     / _ \\| |   | ' /|  _| |  \\| |  _| | | | |   \002\026");
	put_it(":       \026\002   | |_) | |___ / ___ \\ |___| . \\| |___| |\\  | |___| |_| |   \002\026");
	put_it(":       \026\002   |____/|_____/_/   \\_\\____|_|\\_\\_____|_| \\_|_____|____/    \002\026");
	put_it(":       \026\002                                                             \002\026");
	put_it(":           ");
	put_it(":           Blackened IRC version %s, an ircii-2.8.2 based client", irc_version);
	put_it(":           Copyright (C) 1999-2002  Timothy L. Jensen");
	put_it(":             ");
	put_it(":             Offical Website: \002http://www.blackened.com/blackened/\002");
	put_it(":             ");
	put_it(": Blackened IRC is free software with ABSOLUTELY NO WARRANTY.");
	put_it(": For details type `/help warranty'.");
	put_it(":             ");
}

void
umode(command, args)
	char	*command,
		*args;
{
  if (args) {
    send_to_server("MODE %s %s", get_server_nickname(curr_scr_win->server), args);
  }
}

void
handle_whokill(nick, user, host, server)
	char 	*nick,
		*user,
		*host,
		*server;
{
	sprintf(dummy, "%s@%s", user, host);
	if (!nick)
	{
		com_action = 0;
		strcpy(reason, "");
		return;
	}
	if (isme(nick))
		return;
	send_to_server("KILL %s :%s (%s - %s)", nick, 
		*reason ? reason : get_reason(), cluster(dummy), server);	
}

void
add_to_away(nick, message)
	char	*nick,
		*message;
{
	int i;
 	new_free(&last_away[9].nick);   
 	new_free(&last_away[9].message);
	for (i = 9; i > 0; i--) {
		last_away[i].nick = last_away[i-1].nick;
		last_away[i].message = last_away[i-1].message;
	}
	last_away[0].nick = last_away[0].message = 0;
	malloc_strcpy(&last_away[0].nick, nick);
	malloc_strcpy(&last_away[0].message, message);
}

int
seen_away_msg(nick, message)
	char	*nick,
		*message;
{
	int i;
	for (i = 0; i < 10; i++) {
		if (nick && message && last_away[i].nick &&
				last_away[i].message &&
				!strcmp(last_away[i].nick, nick) &&
				!strcmp(last_away[i].message, message))
			return 1;
	}
	return 0;
}

char *
function_randread(input)
	char	*input;
{
  int count, min, i;
  FILE *bleah;
  char buffer[1024];
  char *result = (char *) 0;

  min = 1;
  count = 0;

  strcpy(buffer, "");
  if (!(bleah = fopen(input, "r")))
    return NULL; 
  while (!feof(bleah))
      if (freadln(bleah, buffer))
        count++;
  fseek(bleah, 0, 0);
  i = getrandom(1, count);
  count = 0;
  while (!feof(bleah) && (count < i))
      if (freadln(bleah, buffer))
        count++;
  fclose(bleah);
  if (*buffer)
    malloc_strcpy(&result, buffer);
  else
    result = NULL;
  return result;
}

void
invitecmd(command, args)
	char	*command,
		*args;
{
	char	*nick, *channel;

	nick = my_next_arg(&args);
	if (!nick || !*nick) {
		say("Invite whom?");
		return;
	}
	channel = my_next_arg(&args);
	if (!channel || !strcmp(channel, "*"))
		if ((channel = get_channel_by_refnum(0)) == NULL) {
			say("You aren't on a channel in this window");
			return;
		}
	send_to_server("INVITE %s %s", nick, channel);
}

char	*
SmallTS(when)
	const time_t	*when;
{
  struct tm	*tm;
  static char	ts[128];

  tm = localtime(when);

  strftime(ts, 128, "%d %b %Y %T %Z", tm);

  return (char *)&ts;
}

void
awaylistcmd(void)
{
	int i;
	say("Away list:");
	for (i = 0; i < 10; i++)
		say("%d: %9s %s", i, last_away[i].nick?last_away[i].nick:"<empty>",
			last_away[i].message?last_away[i].message:"<empty>");
}

void
set_oops(nick, line, command)
	char	*nick,
		*line,
		*command;
{
	malloc_strcpy(&oops_nick, nick);
	malloc_strcpy(&oops_line, line);
	malloc_strcpy(&oops_command, command);
}

void
oopscmd(command, args)
	char	*command,
		*args;
{
	char	*nick, *temp_line = (char *) 0, *temp_command = (char *) 0;

	if (!oops_nick) {
		say("Nothing to say Oops! about");
		return;
	}
	nick = my_next_arg(&args);

	if (nick && *nick) {
		malloc_strcpy(&temp_line, oops_line);
		malloc_strcpy(&temp_command, oops_command);
		send_text(oops_nick, "Oops! That message was not intended for you!", "PRIVMSG");
		send_text(nick, temp_line, temp_command);
		new_free(&temp_line);
		new_free(&temp_command);
	} else
		say("Who should the last message have gone to?");
}

void
userhost_finger(stuff, nick, args)
	WhoisStuff *stuff;
	char	*nick;
	char	*args;
{
        char userhost[512];

        if (!stuff || !stuff->nick || !nick || my_stricmp(stuff->nick, nick))
                return;
	if (stuff->not_on) {
		say("%s: No such nick", nick);
		return;
	}
	sprintf(userhost, "%s@%s", stuff->user, stuff->host);
	netfinger(userhost);
}

typedef struct banitem
{
	char	*mask;
	char	*channel;
	int	server;
	time_t	timeout;
	struct banitem	*next;
} BanList;

static BanList	*tempbanlist = (BanList *) 0;

void
change_server_tempbans(old, new)
	int	old,
		new;
{
	BanList	*tmp;

	for (tmp = tempbanlist; tmp; tmp = tmp->next)
		if (tmp->server == old)
			tmp->server = new;
}

void
insert_ban(channel, mask, timeout)
char	*channel,
	*mask;
int	timeout;
{
	BanList	*tmp, *fnd;

	fnd = tempbanlist;
	while (fnd) {
		if ((fnd->server == from_server) && fnd->mask && fnd->channel &&
				!my_stricmp(mask, fnd->mask) &&
				!my_stricmp(channel, fnd->channel))
		{
			fnd->timeout = time(NULL) + (timeout * 60);
			return;
		}
		fnd = fnd->next;
	}

	tmp = (BanList *) new_malloc(sizeof(BanList));
	tmp->mask = (char *) 0;
	tmp->channel = (char *) 0;

	malloc_strcpy(&tmp->mask, mask);
	malloc_strcpy(&tmp->channel, channel);
	tmp->timeout = time(NULL) + (timeout * 60);
	tmp->server = from_server;
	tmp->next = tempbanlist;
	tempbanlist = tmp;
}

int
remove_ban(channel, mask)
	char	*channel,
		*mask;
{
	BanList	*tmp = tempbanlist;

	if (tmp) {
		if ((tmp->server == from_server ) &&
				!my_stricmp(mask, tmp->mask) &&
				!my_stricmp(channel, tmp->channel)) {
			new_free(&tmp->mask);
			new_free(&tmp->channel);
			tempbanlist = tmp->next;
			new_free(&tmp);
			return 1;
		} else
			while (tmp->next) {
				if ((tmp->next->server == from_server) &&
				   !my_stricmp(mask, tmp->next->mask) &&
				   !my_stricmp(channel, tmp->next->channel)) {
					BanList *tmp2;
					tmp2 = tmp->next;
					new_free(&tmp2->mask);
					new_free(&tmp2->channel);
					tmp->next = tmp2->next;
					new_free(&tmp2);
					return 1;
				}
				tmp = tmp->next;
			}
	}
	return 0;
}

void
BanTimer()
{
	time_t	now;
	int	diff;
	BanList	*tmp, *this;
	int	save_server;

	now = time(NULL);

	tmp = tempbanlist;
	while (tmp) {
		this = tmp;
		tmp = tmp->next;

		diff = this->timeout - now;

		if (diff <= 0) {
			save_server = from_server;
			from_server = this->server;
			if (get_channel_oper(this->channel, from_server))
				send_to_server("MODE %s -b %s", this->channel,
					this->mask);
			remove_ban(this->channel, this->mask);
			from_server = save_server;
		}
	}
}

/*
:plasma.engr.arizona.edu 225 lijasdf D 1.1.1.* foo [Toast@OOMon]
:plasma.engr.arizona.edu 219 lijasdf d :End of /STATS report
*/
void
handle_findd(parms)
	char	**parms;
{
	int	i;
	char	*mask;
	char	reason[512];

	if (parms) {
		find_cnt++;
		mask = parms[1];
		if (wild_match(toast_arg, mask)) {
			find_fnd++;
			strcpy(reason, "");
			for (i = 2; parms[i]; i++) {
				if (i > 2) strcat(reason, " ");
				strcat(reason, parms[i]);
			}
			say("D: %s : %s", mask, reason);
		}
	} else {
		toast_action = TACTION_NONE;
		say("End of FindD %s (%u dlines, %u matches)",
			toast_arg, find_cnt, find_fnd);
	}
}

void
findd(command, args)
	char	*command;
	char	*args;
{
	char *pattern,
	     *server;

	if (toast_action) {
		say("Another operation in progress. Please wait.");
		return;
	}

	if (!(pattern = my_next_arg(&args)))
	{
		say("No pattern given for FindD");
		return;
	}
	server = my_next_arg(&args);
        strcpy(toast_arg, pattern);
	toast_action = TACTION_FINDD;
	find_cnt = 0;
	find_fnd = 0;
	say("Starting FindD for %s", toast_arg);
	send_to_server("STATS D");
}

static char *filtered = (char *) 0;

char *
filterlog(line)
char	*line;
{
	int	len, pos, i;
	char	c;

	len = strlen(line);

	if (filtered)
		new_free(&filtered);
	filtered = new_malloc(len+1);
	bzero(filtered, len+1);
	pos = 0;
	for (i = 0; i < len; i++)
		if (((c = line[i]) > 31) || (c == '\t'))
			filtered[pos++] = c;
	return filtered;
}

void chat_cmd(command, args)
char *command;
char *args;
{
#ifdef DISABLE_DCC
	say("DCC has been disabled.");
#else
	char	junk[1024];

	sprintf(junk, "CHAT %s", args);
	process_dcc(junk);
#endif
}

void rchat_cmd(command, args)
char *command;
char *args;
{
#ifdef DISABLE_DCC
	say("DCC has been disabled.");
#else
	char	junk[1024];

	sprintf(junk, "CLOSE CHAT %s", args);
	process_dcc(junk);
#endif
}

void show_tempbans()
{
	char	format[40], timestr[15];
	BanList	*tmp;
	time_t	now;
	int	cwidth;
	int	left;

	now = time(NULL);

	tmp = tempbanlist;

	if (!tmp) {
		say("No temporary bans are pending!");
		return;
	}
	cwidth = get_int_var(CHANNEL_NAME_WIDTH_VAR);
	sprintf(format, "%%-6s %%-%ds %%s", (unsigned char) cwidth);
	say(format, "Time", "Channel", "Mask");
	while (tmp) {
		if (tmp->server == from_server) {
			left = tmp->timeout - now;
			sprintf(timestr, left < 60 ? "%ds" : "%dm",
				left < 60 ? left : left/60);
			say(format, timestr, tmp->channel, tmp->mask);
		}
		
		tmp = tmp->next;
	}
}

void dovoice(command, args)
char *command;
char *args;
{
	char *to, *temp;
	int count, max;

	count = 0;
	temp = (char *)0;
	max = get_int_var(NUM_OPMODES_VAR);
	strcpy(buffer, "");
        if (!(to=my_next_arg(&args)))
                to = "*";
        if (!is_channel(to) && strcmp(to, "*"))
        {
                temp = to;
                to="*";
        }
        if (!strcmp(to, "*"))
                if ((to = get_channel_by_refnum(0)) == NULL)
                {
                        not_on_a_channel();
                        return;
                }
#ifdef CHECK_FOR_OPS
        if (!are_you_opped(to))
        {
                you_not_opped(to);
                return;
        }
#endif /* CHECK_FOR_OPS */
	if (!temp)
		temp = my_next_arg(&args);
	while (temp && *temp)
	{
		count++;
		strcat(buffer, temp);
		if (count == max)
		{
			send_to_server("MODE %s +vvvv %s", to, buffer);
			count = 0;
			strcpy(buffer, "");
		}
		else
			strcat(buffer, " ");
		temp = my_next_arg(&args);
	}
	if (count)
		send_to_server("MODE %s +vvvv %s", to, buffer);
}

void dodevoice(command, args)
char *command;
char *args;
{
        char *to, *temp;
        int count, max;

        count = 0;
	temp = (char *)0;
        max = get_int_var(NUM_OPMODES_VAR);
        strcpy(buffer, "");
        if (!(to=my_next_arg(&args)))
                to = "*";
        if (!is_channel(to) && strcmp(to, "*"))
        {
                temp = to;
                to="*";
        }
        if (!strcmp(to, "*"))
                if ((to = get_channel_by_refnum(0)) == NULL)
                {
                        not_on_a_channel();
                        return;
                }
#ifdef CHECK_FOR_OPS
        if (!are_you_opped(to))
        {
                you_not_opped(to);
                return;
        }
#endif /* CHECK_FOR_OPS */
	if (!temp)
		temp = my_next_arg(&args);
	while (temp && *temp)
        {
                count++;
                strcat(buffer, temp);
                if (count == max)
                {
                        send_to_server("MODE %s -vvvv %s", to, buffer);
                        count = 0;
                        strcpy(buffer, "");
                }
		else
			strcat(buffer, " ");
		temp = my_next_arg(&args);
        }
        if (count)
                send_to_server("MODE %s -vvvv %s", to, buffer);
}

void massvoice(command, args)
char *command;
char *args;
{
	ChannelList	*chan;
	NickList	*nicks;
	char		*to, *spec, *rest;
	char		modebuf[1024];
	int		maxmodes, count, i, all;
        
	all = 0;  /* voice those who are already opped too? */
	maxmodes = get_int_var(NUM_OPMODES_VAR);
	rest = (char *) 0;
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to="*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0)) == NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec = my_next_arg(&args)))
		spec = "*!*@*";
	if (*spec == '-')
	{
		rest = spec;
		spec = "*!*@*";
	}
	else
		rest = args;
	if (rest && !my_stricmp(rest, "-all"))
		all = 1;
	chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
	{
        	say("Error looking up channel %s", to);
		return;
	}
	nicks = chan->nicks;
	count = 0;
	while (nicks)
	{
        	i = 0;
		strcpy(modebuf, "");
		while (nicks && (i < maxmodes))
		{
			strcpy(buffer, "");
			strcat(buffer, nicks->nick);
                	strcat(buffer, "!");
                	strcat(buffer, nicks->user);
	                strcat(buffer, "@");
	                strcat(buffer, nicks->host);
			if ((all || !nicks->chanop) &&
				my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
				wild_match(spec, buffer))
			{
				count++;
				i++;
				strcat(modebuf, " ");
				strcat(modebuf, nicks->nick);
			}
                        nicks = nicks->next;
		}
		if (i)
			send_to_server("MODE %s +vvvv %s", to, modebuf);
	}
	if (!count)
		say("No matches for massvoice of %s on %s", spec, to);
}

void massdevoice(command, args)
char *command;
char *args;
{
	ChannelList	*chan;
	NickList	*nicks;
	char		*to, *spec, *rest;
	char		modebuf[1024];
	int		maxmodes, count, i, all;
        
	all = 0;  /* devoice those who are already opped too? */
	maxmodes = get_int_var(NUM_OPMODES_VAR);
	rest = (char *) 0;
	spec = (char *) 0;
	if (!(to=my_next_arg(&args)))
		to = "*";
	if (!is_channel(to) && strcmp(to, "*"))
	{
		spec = to;
		to="*";
	}
	if (!strcmp(to, "*"))
		if ((to = get_channel_by_refnum(0)) == NULL)
		{
			not_on_a_channel();
			return;
		}
#ifdef CHECK_FOR_OPS
	if (!are_you_opped(to))
	{
		you_not_opped(to);
		return;
	}
#endif /* CHECK_FOR_OPS */
	if (!spec && !(spec = my_next_arg(&args)))
		spec = "*!*@*";
	if (*spec == '-')
	{
		rest = spec;
		spec = "*!*@*";
	}
	else
		rest = args;
	if (rest && !my_stricmp(rest, "-all"))
		all = 1;
	chan = lookup_channel(to, from_server, 0);
        if ((ChannelList *) 0 == chan)
	{
        	say("Error looking up channel %s", to);
		return;
	}
	nicks = chan->nicks;
	count = 0;
	while (nicks)
	{
        	i = 0;
		strcpy(modebuf, "");
		while (nicks && (i < maxmodes))
		{
			strcpy(buffer, "");
			strcat(buffer, nicks->nick);
                	strcat(buffer, "!");
                	strcat(buffer, nicks->user);
	                strcat(buffer, "@");
	                strcat(buffer, nicks->host);
			if ((all || !nicks->chanop) &&
				my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
				wild_match(spec, buffer))
			{
				count++;
				i++;
				strcat(modebuf, " ");
				strcat(modebuf, nicks->nick);
			}
                        nicks = nicks->next;
		}
		if (i)
			send_to_server("MODE %s -vvvv %s", to, modebuf);
	}
	if (!count)
		say("No matches for massdevoice of %s on %s", spec, to);
}



void
dmsg(command, args, subargs)
	char	*command,
		*args,
		*subargs;
{
	char	*tmp;
	char	*nick;

	if ((nick = next_arg(args, &args)) != NULL)
	{
		tmp = new_malloc(strlen(nick) + 2);
		if (tmp)
		{
			tmp[0] = '=';
			strcpy(tmp + 1, nick);
			send_text(tmp, args, command);
			new_free(&tmp);
		}
	}
	else
	{
		say("You must specify a nickname!");
	}
}

/*
 * set_urllog_level: called whenever a "SET URLLOG_LEVEL" is done.  It
 * parses the settings and sets the urllog_level variable appropriately.  It
 * also rewrites the URLLOG_LEVEL variable to make it look nice 
 */
void
set_urllog_level(str)
	char	*str;
{
	urllog_level = parse_lastlog_level(str, "URLLOG");
	set_string_var(URLLOG_LEVEL_VAR, bits_to_lastlog_level(urllog_level));
}

/*
 * set_urllog_file: called whenever a "SET URLLOG_FILE" is done.
 */
void
set_urllog_file(str)
	char	*str;
{
	if (urllog_fp)
	{
		fclose(urllog_fp);
		urllog_fp = NULL;
	}
}

void
add_to_urllog(str)
	char	*str;
{
	time_t	t = time(NULL);

	if (get_lastlog_msg_level() & urllog_level)
	{
		char	*copy = NULL;
		char	*http;

		malloc_strcpy(&copy, str);

		if (NULL != (http = my_strstr(copy, "http://")))
		{
			unsigned char	*end;

			for (end = http; *end > 32; end++);
			*end = 0;

			if (!urllog_fp)
			{
				char	*filename = get_string_var(URLLOG_FILE_VAR);

				if (filename && *filename)
				{
					filename = NULL;
					malloc_strcpy(&filename, my_path);
					malloc_strcat(&filename, "/");
					malloc_strcat(&filename, get_string_var(URLLOG_FILE_VAR));
					urllog_fp = fopen(filename, "a");
					new_free(&filename);
				}
			}

			if (urllog_fp)
			{
				fprintf(urllog_fp, "%s  <%s>\n", http, SmallTS(&t));
				fflush(urllog_fp);
			}
		}
	}
}
