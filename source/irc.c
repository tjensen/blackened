/*
 * Blackened IRC:  It's better than bad!  It's good!
 *
 * Written By Timothy Jensen
 * Copyright(c) 1999-2001 
 * See the COPYRIGHT file, or do a /HELP COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: irc.c,v 1.21.2.5 2002/03/17 23:18:19 toast Exp $";
#endif

#define BLACKENED_VERSION	"1.8.1"

/*
 * INTERNAL_VERSION is the number that the special alias $V returns.
 * Make sure you are prepared for floods, pestilence, hordes of locusts, 
 * and all sorts of HELL to break loose if you change this number.
 * Its format is actually YYYYMMDD, for the _release_ date of the
 * client..
 */
#define INTERNAL_VERSION	"20020317"

#include "irc.h"

#include <sys/stat.h>
#include <pwd.h>

#ifdef ISC22
# include <sys/bsdtypes.h>
#endif /* ISC22 */

#ifdef ESIX
# include <lan/net_types.h>
#endif /* ESIX */

#if USING_CURSES
# include <curses.h>
#endif /* POSIX */

#if DO_USER2
# include <setjmp.h>
#endif

#include "status.h"
#include "dcc.h"
#include "names.h"
#include "vars.h"
#include "input.h"
#include "alias.h"
#include "output.h"
#include "term.h"
#include "exec.h"
#include "screen.h"
#include "log.h"
#include "server.h"
#include "hook.h"
#include "keys.h"
#include "ircaux.h"
#include "edit.h"
#include "window.h"
#include "history.h"
#include "exec.h"
#include "notify.h"
#include "mail.h"
#include "debug.h"
#include "toast.h"
#include "format.h"
#include "ignore.h"
#include "fileio.h"
#include "autoop.h"
#include "rejoin.h"

int	irc_port = IRC_PORT,			/* port of ircd */
	send_text_flag = -1,			/* used in the send_text()
						 * routine */
	use_flow_control = USE_FLOW_CONTROL,	/* true: ^Q/^S used for flow
						 * cntl */
	irc_io_loop = 1,			/* see irc_io below */
	break_io_processing = 0,		/* more forceful than
						 * irc_io_loop */
	current_numeric,			/* this is negative of the
						 * current numeric! */
	dumb = 0,				/* if true, IRCII is put in
						 * "dumb" mode */
	no_fork = 0,				/* if true, IRCII won't with
						 * -b or -e */
	use_input = 1,				/* if 0, stdin is never
						 * checked */
	waiting = 0,				/* used by /WAIT command */
	who_mask = 0,				/* keeps track of which /who
						 * switchs are set */
	background = 0;

char	oper_command = 0;	/* true just after an oper() command is
				 * given.  Used to tell the difference
				 * between an incorrect password generated by
				 * an oper() command and one generated when
				 * connecting to a new server */
char	global_all_off[2];		/* lame kludge to get around lameness */
char	MyHostName[80];			/* The local machine name. Used by
					 * DCC TALK */
	struct	in_addr	MyHostAddr;		/* The local machine address */
	struct	in_addr	local_ip_address; /* Sometimes the same, sometimes not */
extern	char	*last_away_nick;

char	*invite_channel = (char *) 0,		/* last channel of an INVITE */
	buffer[BIG_BUFFER_SIZE + 1],		/* multipurpose buffer */
	*ircrc_file = (char *) 0,		/* full path .ircrc file */
	*my_path = (char *) 0,		/* path to users home dir */
	*irc_path = (char *) 0,		/* paths used by /load */
	*irc_lib = (char *) 0,		/* path to the ircII library */
	nickname[NICKNAME_LEN + 1],		/* users nickname */
	hostname[NAME_LEN + 1],			/* name of current host */
	realname[REALNAME_LEN + 1],		/* real name of user */
	username[NAME_LEN + 1],			/* usernameof user */
	*send_umode = NULL,			/* sent umode */
	*args_str = (char *) 0,		/* list of command line args */
	*last_notify_nick = (char *) 0,	/* last detected nickname */
	*who_name = (char *) 0,		/* extra /who switch info */
	*who_file = (char *) 0,		/* extra /who switch info */
	*who_server = (char *) 0,		/* extra /who switch info */
	*who_host = (char *) 0,		/* extra /who switch info */
	*who_nick = (char *) 0,		/* extra /who switch info */
	*who_real = (char *) 0,		/* extra /who switch info */
	*cannot_open = (char *) 0,		/* extra /who switch info */
	*cut_buffer = (char *) 0;		/* global cut_buffer */

int	away_set = 0;			/* set if there is an away message
					 * anywhere */
int	quick_startup = 0;		/* set if we ignore .ircrc */
time_t	idle_time = 0;
time_t	start_time;

RETSIGTYPE	cntl_c();
RETSIGTYPE	sig_user1();
#ifdef CORECATCH
RETSIGTYPE	coredump();
#endif /* CORECATCH */

static	int	cntl_c_hit = 0;

#if DO_USER2
	jmp_buf	outta_here;
#endif

char	empty_string[] = "";		/* just an empty string */

const	char	irc_version[] = BLACKENED_VERSION;
const	char	internal_version[] = INTERNAL_VERSION;

static	char	switch_help[] =
"Usage: blackened [switches] [nickname] [server list] \n\
  The [nickname] can be at most 9 characters long\n\
  The [server list] is a whitespace separate list of server name\n\
  The [switches] may be any or all of the following\n\
   -c <channel>\tjoins <channel> o startup\n\
   -p <port>\tdefault server connection port (usually 6667)\n\
   -f\t\tyour terminal uses flow controls (^S/^Q), so Blackened shouldn't\n\
   -F\t\tyour terminal doesn't use flow control (default)\n\
   -s\t\tdon't use separate server processes (ircserv)\n\
   -S\t\tuse separate server processes (ircserv)\n\
   -d\t\truns Blackened in \"dumb\" terminal mode\n\
   -q\t\tdoes not load .ircrc\n\
   -a\t\tadds default servers and command line servers to server list\n";

static	char	switch_help_l[] =
#ifdef COMMAND_LINE_L
"   -l <file>\tloads <file> in place of your .ircrc\n\
   -L <file>\tloads <file> in place of your .ircrc and expands $ expandos\n";
#else
"";
#endif

/* irc_exit: cleans up and leaves */
void
irc_exit()
{
	do_hook(EXIT_LIST, "Exiting");
	close_server(-1, "Eject! Eject! Eject!");
	logger(0);
	if (get_int_var(MSGLOG_VAR))
		msglogger(0);
	log_kill(NULL, NULL, "", "", "");
	set_history_file((char *) 0);
	clean_up_processes();
	if (!dumb)
	{
		cursor_to_input();	/* Needed so that ircII doesn't gobble
					 * the last line of the kill. */
		term_cr();
		if (term_clear_to_eol())
			term_space_erase(0);
		term_reset();
#if defined(_HPUX_SOURCE) || defined(ESIX)
		endwin();		/* Added for curses */
#ifdef ESIX
		system("tput reset");
		new_stty("sane");
#endif /* ESIX */
#endif /* defined(_HPUX_SOURCE) || defined(ESIX) */
	}
	exit(0);
}

#ifdef CORECATCH
/* sigsegv: something to handle segfaults in a nice way */
RETSIGTYPE
coredump(sig)
	int	sig;
{
	printf("Blackened has been terminated by signal %s\n\r", signals[sig]);
	printf("Please inform Toast by emailing <blackened-bugs@blackened.com> with as\n\r");
	printf("much detail as possible about what you were doing when it happened.\n\r");
	printf("Please include the version of Blackened (%s) and type of system in\n\r", irc_version);
	printf("the report.\n\r");
	fflush(stdout);
	close_server(-1, "I can't find my babies!");
	abort();
}
#endif

/*
 * quit_response: Used by irc_io when called from irc_quit to see if we got
 * the right response to our question.  If the response was affirmative, the
 * user gets booted from irc.  Otherwise, life goes on. 
 */
static	void
quit_response(dummy, ptr)
	char	*dummy;
	char	*ptr;
{
	int	len,
		old_irc_io_loop;

	old_irc_io_loop = irc_io_loop;
	irc_io_loop = 0;
	if ((len = strlen(ptr)) != 0)
	{
		if (!my_strnicmp(ptr, "yes", len))
		{
			send_to_server("QUIT");
			irc_exit();
		}
	}
	irc_io_loop = old_irc_io_loop;
}

/* irc_quit: prompts the user if they wish to exit, then does the right thing */
void
irc_quit()
{
	static	int in_it = 0;

	if (in_it)
		return;
	in_it = 1;
	add_wait_prompt("Do you really want to quit? ", quit_response,
		empty_string, WAIT_PROMPT_LINE);
	in_it = 0;
}

/*
 * cntl_c: emergency exit.... if somehow everything else freezes up, hitting
 * ^C five times should kill the program. 
 */
RETSIGTYPE
cntl_c()
{
#ifdef SYSVSIGNALS
	(void) MY_SIGNAL(SIGINT, cntl_c, 0);
#endif /* SYSVSIGNALS */
	if (cntl_c_hit++ >= 4)
		irc_exit();
}

RETSIGTYPE
sig_user1()
{
#ifdef SYSVSIGNALS
	(void) MY_SIGNAL(SIGUSR1, sig_user1, 0);
#endif /* SYSVSIGNALS */
	say("Got SIGUSR1, closing DCC connections and EXECed processes");
	close_all_dcc();
	close_all_exec();
}

#if DO_USER2
RETSIGTYPE
sig_user2()
{
#ifdef SYSVSIGNALS
	(void) MY_SIGNAL(SIGUSR2, sig_user2, 0);
#endif /* SYSVSIGNALS */
	say("Got SIGUSR2, jumping to normal loop");
	longjmp(outta_here);
}
#endif 

#ifdef MUNIX
/* A characteristic of PCS MUNIX - Ctrl-Y produces a SIGQUIT */
RETSIGTYPE
cntl_y()
{
	(void) MY_SIGNAL(SIGQUIT, cntl_y, 0);
	edit_char((char) 25); /* Ctrl-Y */
}
#endif

/* shows the version of irc */
static	void
show_version()
{
	printf("Blackened version %s (%s)\n\r", irc_version, internal_version);
	exit (0);
}

/* get_arg: used by parse_args() to get an argument after a switch */
static	char	*
get_arg(arg, next_arg, ac)
	char	*arg;
	char	*next_arg;
	int	*ac;
{
	(*ac)++;
	if (*arg)
		return (arg);
	else
	{
		if (next_arg)
			return (next_arg);
		fprintf(stderr, "irc: missing parameter\n");
		exit(1);
	}
}

/*
 * parse_args: parse command line arguments for irc, and sets all initial
 * flags, etc. 
 */
static	char	*
parse_args(argv, argc)
	char	*argv[];
	int	argc;
{
	char	*arg,
		*ptr;
	int	ac;
	int	add_servers = 0;
	char	*channel = (char *) NULL;
	struct	passwd	*entry;
	struct	hostent	*hp;
	int	minus_minus = 0;

    /*
     * Note that this uses the global buffer to build the args_str list,
     * which is a whitespace separated list of the arguments used when
     * loading the .ircrc and GLOBAL_IRCRC files.
     */
	*nickname = '\0';
	*realname = '\0';
	ac = 1;
	strmcpy(buffer, argv[0], BIG_BUFFER_SIZE);
	strmcat(buffer, " ", BIG_BUFFER_SIZE);
	while ((arg = argv[ac++]) != (char *) NULL)
	{
		strmcat(buffer, argv[ac-1], BIG_BUFFER_SIZE);
		strmcat(buffer, " ", BIG_BUFFER_SIZE);
		if ((*arg == '-') != '\0')
		{
			++arg;
			while (*arg)
			{
				switch (*(arg++))
				{
				case 'v':
					show_version();
					break;
				case 'c':
					malloc_strcpy(&channel, get_arg(arg,
							argv[ac], &ac));
					break;
				case 'p':
					irc_port = atoi(get_arg(arg, argv[ac],
							&ac));
					break;
				case 'f':
					use_flow_control = 1;
					break;
				case 'F':
					use_flow_control = 0;
					break;
				case 'd':
					dumb = 1;
					break;
#ifdef DEBUG
				case 'D':
					setdlevel(atoi(get_arg(arg, argv[ac],
						&ac)));
					break;
				case 'o':
					{
						FILE	*fp;
						char	*file = get_arg(arg, argv[ac], &ac);

						if (!file)
						{
							printf("irc: need filename for -o\n");
							exit(-1);
						}
						fp = freopen(file, "w", stderr);
						if (!fp)
						{
							printf("irc: can not open %s: %s\n", file, errno ? "" : strerror(errno));
							exit(-1);
						}
					}
#endif /* DEBUG */
#ifdef COMMAND_LINE_L
				case 'l':
					malloc_strcpy(&ircrc_file, get_arg(arg,
							argv[ac], &ac));
					break;
				case 'L':
					malloc_strcpy(&ircrc_file, get_arg(arg,
							argv[ac], &ac));
					malloc_strcat(&ircrc_file," -");
					break;
#endif /* COMMAND_LINE_L */
				case 'a':
					add_servers = 1;
					break;
				case 's':
					using_server_process = 0;
					break;
				case 'S':
					using_server_process = 1;
					break;
				case 'q':
					quick_startup = 1;
					break;
#ifdef COMMAND_LINE_B
				case 'b':
					dumb = 1;
					use_input = 0;
					background = 1;
					break;
#endif /* COMMAND_LINE_B */
				case '-':
					if (argv[ac])
					{
						while ((arg = argv[ac++]) != NULL)
						{
							strmcat(command_line, arg, BIG_BUFFER_SIZE);
							strmcat(command_line, " ", BIG_BUFFER_SIZE);
						}
						command_line[strlen(command_line)-1] = '\0';
					}
					minus_minus = 1;
					break;
				default:
					fprintf(stderr, "%s%s", switch_help, switch_help_l);
					exit(1);
				}
			}
		}
		else
		{
			if (*nickname)
				build_server_list(arg);
			else
				strmcpy(nickname, arg, NICKNAME_LEN);
		}
		if (minus_minus)
			break;
	}
	malloc_strcpy(&args_str, buffer);
	if ((char *) 0 != (ptr = (char *) getenv("IRCLIB")))
	{
		malloc_strcpy(&irc_lib, ptr);
		malloc_strcat(&irc_lib, "/");
	}
	else
		malloc_strcpy(&irc_lib, IRCLIB);

	if ((char *) 0 == ircrc_file && (char *) 0 != (ptr = getenv("IRCRC")))
		malloc_strcpy(&ircrc_file, ptr);

	if (*nickname == '\0' && (char *) 0 != (ptr = getenv("IRCNICK")))
		strmcpy(nickname, ptr, NICKNAME_LEN);

	if ((char *) 0 != (ptr = getenv("IRCUMODE")))
		malloc_strcpy(&send_umode, ptr);

	if ((char *) 0 != (ptr = getenv("IRCNAME")))
		strmcpy(realname, ptr, REALNAME_LEN);

	if ((char *) 0 != (ptr = getenv("IRCPATH")))
		malloc_strcpy(&irc_path, ptr);
	else
	{
#ifdef IRCPATH
		malloc_strcpy(&irc_path, IRCPATH);
#else
		malloc_strcpy(&irc_path, ".:~/.irc:");
		malloc_strcat(&irc_path, irc_lib);
		malloc_strcat(&irc_path, "script");
#endif
	}

	set_string_var(LOAD_PATH_VAR, irc_path);
	new_free(&irc_path);
	if ((char *) 0 != (ptr = getenv("IRCSERVER")))
		build_server_list(ptr);
	if (0 == server_list_size() || add_servers)
	{
#ifdef SERVERS_FILE
		if (read_server_file() || (server_list_size() == 0))
#endif
		{
			char *ptr = (char *) 0;

			malloc_strcpy(&ptr, DEFAULT_SERVER);
			build_server_list(ptr);
			new_free(&ptr);
		}
	}
	if ((struct passwd *) 0 != (entry = getpwuid(getuid())))
	{
		if ((*realname == '\0') && entry->pw_gecos && *(entry->pw_gecos))
		{
#ifdef GECOS_DELIMITER
			if ((ptr = index(entry->pw_gecos, GECOS_DELIMITER)) 
					!= NULL)
				*ptr = '\0';
#endif /* GECOS_DELIMITER */
			strmcpy(realname, entry->pw_gecos, REALNAME_LEN);
		}
		if (entry->pw_name && *(entry->pw_name))
			strmcpy(username, entry->pw_name, NAME_LEN);

		if (entry->pw_dir && *(entry->pw_dir))
			malloc_strcpy(&my_path, entry->pw_dir);
	}
	if ((char *) 0 != (ptr = getenv("HOME")))
		malloc_strcpy(&my_path, ptr);
	else if (*my_path == '\0')
		malloc_strcpy(&my_path, "/");

	if ('\0' == *realname)
		strmcpy(realname, "*Unknown*", REALNAME_LEN);

	if ('\0' == *username)
	{
		if ((ptr = getenv("USER")) != NULL)
			strmcpy(username, ptr, NAME_LEN);
		else
			strmcpy(username, "Unknown", NAME_LEN);
	}
        if ((char *) 0 != (ptr = getenv("IRCHOST")))
	{
		set_string_var(VHOST_VAR, ptr);
		strmcpy(MyHostName, ptr, sizeof(MyHostName));
	}
        else
	{
                gethostname(MyHostName, sizeof(MyHostName));
		if ((char *) 0 != (ptr = getenv("IRC_HOST")))
		{
			set_string_var(VHOST_VAR, ptr);
		}
		else if ((char *) 0 != (ptr = getenv("MYHOSTADDR")))
		{
			set_string_var(VHOST_VAR, ptr);
		}
	}

	if ((hp = gethostbyname(MyHostName)) != NULL)
	{
		bcopy(hp->h_addr, (char *) &MyHostAddr, sizeof(MyHostAddr));
		local_ip_address.s_addr = ntohl(MyHostAddr.s_addr);
	}
	if (*nickname == '\0')
		strmcpy(nickname, username, NICKNAME_LEN);
	if (0 == check_nickname(nickname))
	{
		fprintf(stderr, "Illegal nickname %s\n", nickname);
		exit(1);
	}
	if ((char *) 0 == ircrc_file)
	{
		ircrc_file = (char *) new_malloc(strlen(my_path) +
			strlen(IRCRC_NAME) + 1);
		strcpy(ircrc_file, my_path);
		strcat(ircrc_file, IRCRC_NAME);
	}
	return (channel);
}

/*
 * TimerTimeout:  Called from irc_io to help create the timeout
 * part of the call to select.
 */
time_t
TimerTimeout()
{
	time_t	current;
	time_t	timeout_in;

	return 1;

	if (!PendingTimers)
		return 70; /* Just larger than the maximum of 60 */
	time(&current);
	timeout_in = PendingTimers->time - current;
	return (timeout_in < 0) ? 0 : timeout_in;
}

/*
 * irc_io: the main irc input/output loop.   Handles all io from keyboard,
 * server, exec'd processes, etc.  If a prompt is specified, it is displayed
 * in the input line and cannot be backspaced over, etc. The func is a
 * function which will take the place of the SEND_LINE function (which is
 * what happens when you hit return at the end of a line). This function must
 * decide if it's ok to exit based on anything you really want.  It can then
 * set the global irc_io_loop to false to cause irc_io to exit. 
 */
int
irc_io(prompt, func, use_input, loop)
	char	*prompt;
	void	(*func) ();
	int	use_input;
	int	loop;
{
	static	int	level = 0;
	fd_set	rd,
		wd;
	char	buffer[BIG_BUFFER_SIZE + 1];	/* buffer much bigger than
						 * IRCD_BUFFER_SIZE */
	struct	timeval cursor_timeout,
		clock_timeout,
		right_away,
		timer,
		*timeptr;
	int	hold_over;
	int	old_loop;
	char	*last_input = NULL;
	char	*last_prompt = NULL;
	void	(*last_func)();
	int	one_key = 0;
	Screen	*screen,
		*old_current_screen;
	char	*banner;

	last_func = get_send_line();
	if (use_input == -1)
		one_key = 1, prompt = NULL;
#ifdef	PRIV_PORT_ULC
	seteuid(getuid());
#endif
	/* time before cursor jumps from display area to input line */
	cursor_timeout.tv_usec = 0L;
	cursor_timeout.tv_sec = 1L;

	/* time delay for updating of internal clock */
	clock_timeout.tv_usec = 0L;
	clock_timeout.tv_sec = 30L;

	right_away.tv_usec = 0L;
	right_away.tv_sec = 0L;

	timer.tv_usec = 0L;

	old_loop = irc_io_loop;
	irc_io_loop = loop;

			/*
	if (level++ > 20)
			 * irc_io has been recursive to date.
			 * with multiple xterms and screen
			 * windows, this has to change
			 */
	if (level++ > 5)
	{
		level--;
		irc_io_loop = old_loop;
		return (1);
	}
	if (!dumb)
	{
		if (use_input)
		{
			malloc_strcpy(&last_input, get_input());
			set_input(empty_string);
			last_func = get_send_line();
			change_send_line(func);
		}
		if (prompt)
		{
			malloc_strcpy(&last_prompt, get_input_prompt());
			set_input_prompt(prompt);
		}
	}
	/*
	 * Here we work out if this has been called recursively or
	 * not..  and if not so.. -phone
	 */

#if defined(DEBUG) || defined(DO_USER2)
	if (level != 1)
	{
#ifdef DEBUG
		yell("--- Recursive call to irc_io() - careful");
#endif /* DEBUG */
	}
	else
	{
#if DO_USER2
		if (setjmp(outta_here))
		{
			banner = get_string_var(BANNER_VAR);
			yell("%sGot SIGUSR2, Aborting",
				banner ? banner : empty_string);
		}
#endif /* DO_USER2 */
	}
#endif /* DEBUG || DO_USER2 */

	timeptr = &clock_timeout;
	do
	{
		break_io_processing = 0;
		FD_ZERO(&rd);
		FD_ZERO(&wd);
		if (use_input)
			for (screen = screen_list;screen; screen = screen->next)
				if (screen->alive)
					FD_SET(screen->fdin, &rd);
		set_process_bits(&rd);
		set_server_bits(&rd);
		set_dcc_bits(&rd, &wd);
		if (term_reset_flag)
		{
			refresh_screen();
			term_reset_flag = 0;
		}
		timer.tv_sec = TimerTimeout();
		if (timer.tv_sec <= timeptr->tv_sec)
			timeptr = &timer;
		if ((hold_over = unhold_windows()) != 0)
			timeptr = &right_away;
		Debug((7, "irc_io: selecting with %l:%l timeout", timeptr->tv_sec,
			timeptr->tv_usec));
		switch (new_select(&rd, &wd, timeptr))
		{
		case 0:
		case -1:
	/*
	 * yay for the QNX socket manager... drift here, drift there, oops,
	 * i fell down a hole..
	 */
#ifdef __QNX__
			if (errno == EBADF || errno == ESRCH)
				irc_io_loop = 0;
#endif
			if (cntl_c_hit)
			{
				if (one_key)
				{
					irc_io_loop = 0;
					break;
				}
				edit_char('\003');
				cntl_c_hit = 0;
			}
			if (!hold_over)
				cursor_to_input();
			break;
		default:
			if (term_reset_flag)
			{
				refresh_screen();
				term_reset_flag = 0;
			}
			old_current_screen = current_screen;
			set_current_screen(last_input_screen);
			if (!break_io_processing)
				dcc_check(&rd);
			if (!break_io_processing)
				do_server(&rd);
			set_current_screen(old_current_screen);
			for (screen = screen_list; screen &&
				!break_io_processing; screen = screen->next)
			{
				if (!screen->alive)
					continue;
				set_current_screen(screen);
				if (FD_ISSET(screen->fdin, &rd))
				{

	/*
	 * This section of code handles all in put from the terminal(s).
	 * connected to ircII.  Perhaps the idle time *shouldn't* be 
	 * reset unless its not a screen-fd that was closed..
	 *
	 * This section indented - phone, jan 1993
	 */

			idle_time = time(0);
			if (dumb)
			{
				int     old_timeout;

				old_timeout = dgets_timeout(1);
				if (dgets(buffer, INPUT_BUFFER_SIZE,
						screen->fdin, (char *) 0))
				{
					(void) dgets_timeout(old_timeout);
					if (one_key)
					{
						irc_io_loop = 0;
						break;
					}
					*(buffer + strlen(buffer) - 1) = '\0';
					if (get_int_var(INPUT_ALIASES_VAR))	
						parse_line(NULL, buffer,
						    empty_string, 1, 0);
					else
						parse_line(NULL, buffer,
						    NULL, 1, 0);
				}
				else
				{
					say("IRCII exiting on EOF from stdin");
					irc_exit();
				}
			}
			else
			{
				int server;
				char	loc_buffer[BIG_BUFFER_SIZE + 1];
				int	n, i;

				server = from_server;
				from_server = get_window_server(0);
				last_input_screen = screen;
				if (one_key)
				{
					if (read(screen->fdin, buffer, 1))
					{
						irc_io_loop = 0;
						break;
					}
				/*
				 * Following Fizzy's remark below, if we 
				 * don't use window create, we can't kill
				 * then, can we?  --FlashMan, October 1994
				 */
#ifdef WINDOW_CREATE
					else
					{
						if (!is_main_screen(screen))
							kill_screen(screen);
						else
							irc_exit();
					}
#endif /* WINDOW_CREATE */
				}
				else if ((n = read(screen->fdin, loc_buffer,
						BIG_BUFFER_SIZE)) != 0)
					for (i = 0; i < n; i++)
						edit_char(loc_buffer[i]);
		/*
		 * if the current screen isn't the main  screen,
		 * then the socket to the current screen must have
		 * closed, so we call kill_screen() to handle 
		 * this - phone, jan 1993.
		 * but not when we arent running windows - Fizzy, may 1993
		 * if it is the main screen we got an EOF on, we exit..
		 * closed tty -> chew cpu -> bad .. -phone, july 1993.
		 */
#ifdef WINDOW_CREATE
				else
				{
					if (!is_main_screen(screen))
						kill_screen(screen);
					else
						irc_exit();
				}
#endif /* WINDOW_CREATE */
				cntl_c_hit = 0;
				from_server = server;
			}

		/* End of intendation */

				}
			}
			set_current_screen(old_current_screen);
			if (!break_io_processing)
				do_processes(&rd);
			break;
		}
		ExecuteTimers();
		IgnoreTimer();
		BanTimer();
		OpTimer();
		RejoinTimer();
		check_process_limits();
		(void) check_wait_status(-1);
/*		if ((primary_server == -1) && !never_connected)
			do_hook(DISCONNECT_LIST, "%s", nickname); */
		timeptr = &clock_timeout;

		old_current_screen = current_screen;
		for (current_screen = screen_list; current_screen;
				current_screen = current_screen->next)
			if (current_screen->alive && is_cursor_in_display())
				timeptr = &cursor_timeout;
		set_current_screen(old_current_screen);

		if (update_clock(0))
		{
			Window *old_to_window;
			old_to_window = to_window;
			if (get_int_var(CLOCK_VAR) || check_mail_status())
			{
				status_update(1);
				cursor_to_input();
			}
			if (primary_server != -1)
				do_notify();
			to_window = old_to_window;
		}
	}
	while (irc_io_loop);
	level--;
	irc_io_loop = old_loop;
	if (! dumb)
	{
		if (use_input)
		{
			set_input(last_input);
			new_free(&last_input);
			change_send_line(last_func);
		}
		if (prompt)
		{
			if (level == 0)
			    set_input_prompt(get_string_var(INPUT_PROMPT_VAR));
			else
			    set_input_prompt(last_prompt);
			new_free(&last_prompt);
		}
	}
	update_input(UPDATE_ALL);
	return (0);
}

/*ARGSUSED*/
int
main(argc, argv, envp)
	int	argc;
	char	*argv[];
	char	*envp[];
{
	char	*channel;

	srand( (unsigned)time( NULL ) );
start_time = time((time_t *)0);
	channel = parse_args(argv, argc);
#if defined(_HPUX_SOURCE) || defined(ESIX)
	/* Curses code added for HP-UX use */
	if (!dumb)
	{
		initscr();
		noecho();
		cbreak();
	}
#endif /* _HPUX_SOURCE || ESIX */
	if ((use_input == 0) && !no_fork)
	{
		if (fork())
			_exit(0);
	}
#ifdef ESIX
	if (gethostname(hostname, NAME_LEN) == NULL)
#else
	if (gethostname(hostname, NAME_LEN))
#endif /* ESIX */
	{
		fprintf(stderr, "irc: couldn't figure out the name of your machine!\n");
		exit(1);
	}
	if (dumb)
		new_window();
	else
	{
		init_screen();
#if !defined(MUNIX) && !defined(_RT) && !defined(ESIX)
		(void) MY_SIGNAL(SIGCONT, term_cont, 0);
#endif /* !defined(MUNIX) && !defined(_RT) && !defined(ESIX) */
#if !defined(_RT) && defined(SIGWINCH)
		(void) MY_SIGNAL(SIGWINCH, refresh_screen, 0);
#endif /* _RT */
#ifndef ALLOC_DEBUG
# ifdef CORECATCH
		(void) MY_SIGNAL(SIGSEGV, coredump, 0);
#  ifdef SIGBUS
		(void) MY_SIGNAL(SIGBUS, coredump, 0);
#  endif
# else
		(void) MY_SIGNAL(SIGSEGV, SIG_DFL, 0);
		/* Linux doesn't have SIGBUS */
#  ifndef SIGBUS
		(void) MY_SIGNAL(SIGBUS, SIG_DFL, 0);
#  endif /* SIGBUS */
# endif /* CORECATCH */
#endif /* ALLOC_DEBUG */
#ifdef MUNIX
		(void) MY_SIGNAL(SIGQUIT, cntl_y, 0);
#endif
		(void) MY_SIGNAL(SIGHUP, irc_exit, 0);
		(void) MY_SIGNAL(SIGTERM, irc_exit, 0);
		(void) MY_SIGNAL(SIGPIPE, SIG_IGN, 0);
		(void) MY_SIGNAL(SIGINT, cntl_c, 0);
#ifdef SIGSTOP
		(void) MY_SIGNAL(SIGSTOP, SIG_IGN, 0);
#endif
		(void) MY_SIGNAL(SIGUSR1, sig_user1, 0);
#if DO_USER2
		(void) MY_SIGNAL(SIGUSR2, sig_user2, 0);
#endif
	}
#ifdef _HPUX_SOURCE
	new_stty("opost");
#endif /* _HPUX_SOURCE */

	init_variables();
	init_formats();
	init_bindings();
	init_files();

	if (!dumb)
	{
		build_status((char *) 0);
		update_input(UPDATE_ALL);
	}

	show_banner();

#ifdef MOTD_FILE
	{
		struct	stat	motd_stat,
				my_stat;
		char	*motd = NULL;
		int	des;

		malloc_strcpy(&motd, irc_lib);
		malloc_strcat(&motd, MOTD_FILE);
		if (stat_file(motd, &motd_stat) == 0)
		{
			strmcpy(buffer, my_path, BIG_BUFFER_SIZE);
			strmcat(buffer, "/.ircmotd", BIG_BUFFER_SIZE);
			if (stat_file(buffer, &my_stat))
			{
				my_stat.st_atime = 0L;
				my_stat.st_mtime = 0L;
			}
			unlink(buffer);
			if ((des = open(buffer, O_CREAT, S_IREAD | S_IWRITE))
					!= -1)
				close(des);
			if (motd_stat.st_mtime > my_stat.st_mtime)
			{
				put_file(motd);
		/* Thanks to Mark Dame <mdame@uceng.ec.edu> for this one */

#if PAUSE_AFTER_MOTD
		input_pause("********  Press any key to continue  ********");
#endif
				clear_window_by_refnum(0);
			}
		}
		new_free(&motd);
	}
#endif /* MOTD_FILE */

	global_all_off[0] = ALL_OFF;
	global_all_off[1] = '\0';
	get_connected(0);
	if (channel)
	{
		set_channel_by_refnum(0, channel);
		add_channel(channel, primary_server, 0);
		new_free(&channel);
	}
	idle_time = time(0);
	set_input(empty_string);
	irc_io(get_string_var(INPUT_PROMPT_VAR), NULL, use_input, irc_io_loop);
	irc_exit();
	return 0;
}
