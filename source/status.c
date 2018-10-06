/*
 * status.c: handles the status line updating, etc for IRCII 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: status.c,v 1.5 2001/12/09 05:28:31 toast Exp $";
#endif

#include "irc.h"

#include "term.h"
#include "status.h"
#include "server.h"
#include "vars.h"
#include "hook.h"
#include "input.h"
#include "edit.h"
#include "window.h"
#include "screen.h"
#include "mail.h"
#include "output.h"
#include "names.h"
#include "ircaux.h"
#include "translat.h"
#include "recorder.h"

extern	time_t	time();

extern	char	*update_clock();
static	char	*convert_format();
static	char	*status_nickname();
static	char	*status_query_nick();
static	char	*status_right_justify();
static	char	*status_chanop();
static	char	*status_chanvoice();
static	char	*status_channel();
static	char	*status_server();
static	char	*status_mode();
static	char	*status_umode();
static	char	*status_insert_mode();
static	char	*status_overwrite_mode();
static	char	*status_away();
static	char	*status_oper();
static	char	*status_user0();
static	char	*status_user1();
static	char	*status_user2();
static	char	*status_user3();
static	char	*status_hold();
static	char	*status_version();
static	char	*status_clock();
static	char	*status_hold_lines();
static	char	*status_window();
static	char	*status_mail();
static	char	*status_msgs();
static	char	*status_refnum();
static	char	*status_null_function();
static	char	*status_notify_windows();
static	void	status_make_printable _((unsigned char *, int));

/*
 * Maximum number of "%" expressions in a status line format.  If you change
 * this number, you must manually change the sprintf() in make_status 
 */
#define MAX_FUNCTIONS 33

/* The format statements to build each portion of the status line */
static	char	*mode_format = (char *) 0;
static	char	*umode_format = (char *) 0;
static	char	*status_format = (char *) 0;
static	char	*query_format = (char *) 0;
static	char	*clock_format = (char *) 0;
static	char	*hold_lines_format = (char *) 0;
static	char	*channel_format = (char *) 0;
static	char	*mail_format = (char *) 0;
static	char	*msgs_format = (char *) 0;
static	char	*server_format = (char *) 0;
static	char	*notify_format = (char *) 0;

/*
 * status_func: The list of status line function in the proper order for
 3 display.  This list is set in convert_format() 
 */
static	char	*(*status_func[MAX_FUNCTIONS]) ();

/* func_cnt: the number of status line functions assigned */
static	int	func_cnt;

static	int	alarm_hours,		/* hour setting for alarm in 24 hour time */
		alarm_minutes;		/* minute setting for alarm */

/* Stuff for the alarm */
static	struct itimerval clock_timer = { { 10L, 0L }, { 1L, 0L } };
static	struct itimerval off_timer = { { 0L, 0L }, { 0L, 0L } };

static	RETSIGTYPE alarmed _((int));

/* alarmed: This is called whenever a SIGALRM is received and the alarm is on */
static	RETSIGTYPE
alarmed(unused)
	int	unused;
{
	say("The time is %s", update_clock(GET_TIME));
	term_beep();
	term_beep();
	term_beep();
}

/*
 * alarm_switch: turns on and off the alarm display.  Sets the system timer
 * and sets up a signal to trap SIGALRMs.  If flag is 1, the alarmed()
 * routine will be activated every 10 seconds or so.  If flag is 0, the timer
 * and signal stuff are reset 
 */
static	void
alarm_switch(flag)
	int	flag;
{
	static	int	alarm_on = 0;

	if (flag)
	{
		if (!alarm_on)
		{
			setitimer(ITIMER_REAL, &clock_timer,
				(struct itimerval *) 0);
			(void) MY_SIGNAL(SIGALRM, alarmed, 0);
			alarm_on = 1;
		}
	}
	else if (alarm_on)
	{
		setitimer(ITIMER_REAL, &off_timer, (struct itimerval *) 0);
		(void) MY_SIGNAL(SIGALRM, SIG_IGN, 0);
		alarm_on = 0;
	}
}

/*
 * set_alarm: given an input string, this checks it's validity as a clock
 * type time thingy.  It accepts two time formats.  The first is the HH:MM:XM
 * format where HH is between 1 and 12, MM is between 0 and 59, and XM is
 * either AM or PM.  The second is the HH:MM format where HH is between 0 and
 * 23 and MM is between 0 and 59.  This routine also looks for one special
 * case, "OFF", which sets the alarm string to null 
 */
void
set_alarm(str)
	char	*str;
{
	char	hours[10],
		minutes[10],
		merid[3];
	char	time_str[10];
	int	c,
		h,
		m,
		min_hours,
		max_hours;

	if (str == (char *) 0)
	{
		alarm_switch(0);
		return;
	}
	if (!my_stricmp(str, var_settings[OFF]))
	{
		set_string_var(CLOCK_ALARM_VAR, (char *) 0);
		alarm_switch(0);
		return;
	}

	c = sscanf(str, " %2[^:]:%2[^paPA]%2s ", hours, minutes, merid);
	switch (c)
	{
	case 2:
		min_hours = 0;
		max_hours = 23;
		break;
	case 3:
		min_hours = 1;
		max_hours = 12;
		upper(merid);
		break;
	default:
		say("CLOCK_ALARM: Bad time format.");
		set_string_var(CLOCK_ALARM_VAR, (char *) 0);
		return;
	}

	h = atoi(hours);
	m = atoi(minutes);
	if (h >= min_hours && h <= max_hours && isdigit(hours[0]) &&
		(isdigit(hours[1]) || hours[1] == (char) 0))
	{
		if (m >= 0 && m <= 59 && isdigit(minutes[0]) &&
				isdigit(minutes[1]))
		{
			alarm_minutes = m;
			alarm_hours = h;
			if (max_hours == 12)
			{
				if (merid[0] != 'A')
				{
					if (merid[0] == 'P')
					{
						if (h != 12)
							alarm_hours += 12;
					}
					else
					{
	say("CLOCK_ALARM: alarm time must end with either \"AM\" or \"PM\"");
	set_string_var(CLOCK_ALARM_VAR, (char *) 0);
					}
				}
				else
				{
					if (h == 12)
						alarm_hours = 0;
				}
				if (merid[1] == 'M')
				{
					sprintf(time_str, "%02d:%02d%s", h, m,
						merid);
					set_string_var(CLOCK_ALARM_VAR,
						time_str);
				}
				else
				{
	say("CLOCK_ALARM: alarm time must end with either \"AM\" or \"PM\"");
	set_string_var(CLOCK_ALARM_VAR, (char *) 0);
				}
			}
			else
			{
				sprintf(time_str, "%02d:%02d", h, m);
				set_string_var(CLOCK_ALARM_VAR, time_str);
			}
		}
		else
		{
	say("CLOCK_ALARM: alarm minutes value must be between 0 and 59.");
	set_string_var(CLOCK_ALARM_VAR, (char *) 0);
		}
	}
	else
	{
		say("CLOCK_ALARM: alarm hour value must be between %d and %d.",
			min_hours, max_hours);
		set_string_var(CLOCK_ALARM_VAR, (char *) 0);
	}
}

/* update_clock: figures out the current time and returns it in a nice format */
char	*
update_clock(flag)
	int	flag;
{
	static	char	time_str[10];
	static	int	min = -1,
			hour = -1;
	struct tm	*time_val;
	char	*merid;
	time_t	t;

	t = time(0);
	time_val = localtime(&t);
	if (get_string_var(CLOCK_ALARM_VAR))
	{
		if ((time_val->tm_hour == alarm_hours) &&
				(time_val->tm_min == alarm_minutes))
			alarm_switch(1);
		else
			alarm_switch(0);
	}
	if (flag == RESET_TIME || time_val->tm_min != min || time_val->tm_hour != hour)
	{
		int	tmp_hour,
			tmp_min,
			server;

		tmp_hour = time_val->tm_hour;
		tmp_min = time_val->tm_min;

		if (get_int_var(CLOCK_24HOUR_VAR))
			merid = empty_string;
		else
		{
			if (time_val->tm_hour < 12)
				merid = "AM";
			else
				merid = "PM";
			if (time_val->tm_hour > 12)
				time_val->tm_hour -= 12;
			else if (time_val->tm_hour == 0)
				time_val->tm_hour = 12;
		}
		server = from_server;
		from_server = primary_server;
		sprintf(time_str, "%02d:%02d%s", time_val->tm_hour, time_val->tm_min, merid);
		if (tmp_min != min || tmp_hour != hour)
		{
			Window *old_to_window;
			unsigned int display;
			hour = tmp_hour;
			min = tmp_min;

		/*   Need to keep track of to_window as this will be changed. Otherwise,
		 *   lines that might be meant to go to another window will be
		 *   misdirected whenever the clock changes. -tlj (3/27/97)
		 *
		 *   this still doesn't fix the bug.
		 */
			old_to_window = to_window;
			to_window = curr_scr_win;
			display = window_display;
			window_display = 1;
			do_hook(TIMER_LIST, "%s", time_str);
			window_display = display;
			to_window = old_to_window;
		}
		do_hook(IDLE_LIST, "%ld", (t - idle_time) / 60L);
		from_server = server;
		return (time_str);
	}
	if (flag == GET_TIME)
		return(time_str);
	else
		return ((char *) 0);
}

/*ARGSUSED*/
void
reset_clock(unused)
	char	*unused;
{
	update_clock(RESET_TIME);
	update_all_status();
}

/*
 * convert_sub_format: This is used to convert the formats of the
 * sub-portions of the status line to a format statement specially designed
 * for that sub-portions.  convert_sub_format looks for a single occurence of
 * %c (where c is passed to the function). When found, it is replaced by "%s"
 * for use is a sprintf.  All other occurences of % followed by any other
 * character are left unchanged.  Only the first occurence of %c is
 * converted, all subsequence occurences are left unchanged.  This routine
 * mallocs the returned string. 
 */
static	char	*
convert_sub_format(format, c)
	char	*format;
	char	c;
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	static	char	bletch[] = "%% ";
	char	*ptr = (char *) 0;
	int	dont_got_it = 1;

	if (format == (char *) 0)
		return ((char *) 0);
	*buffer = (char) 0;
	while (format)
	{
		if ((ptr = (char *) index(format, '%')) != NULL)
		{
			*ptr = (char) 0;
			strmcat(buffer, format, BIG_BUFFER_SIZE);
			*(ptr++) = '%';
			if ((*ptr == c) && dont_got_it)
			{
				dont_got_it = 0;
				strmcat(buffer, "%s", BIG_BUFFER_SIZE);
			}
			else
			{
				bletch[2] = *ptr;
				strmcat(buffer, bletch, BIG_BUFFER_SIZE);
			}
			ptr++;
		}
		else
			strmcat(buffer, format, BIG_BUFFER_SIZE);
		format = ptr;
	}
	malloc_strcpy(&ptr, buffer);
	return (ptr);
}

static	char	*
convert_format(str)
	char	*str;
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	*ptr,
		*format,
		*malloc_ptr = (char *) 0;

	format = get_string_var(STATUS_FORMAT_VAR);
	*buffer = (char) 0;
	while (format)
	{
		if ((ptr = (char *) index(format, '%')) != NULL)
		{
			*ptr = (char) 0;
			strmcat(buffer, format, BIG_BUFFER_SIZE);
			*(ptr++) = '%';
			if (func_cnt < MAX_FUNCTIONS)
			{
				switch (*(ptr++))
				{
				case '%':
					strmcat(buffer, "%", BIG_BUFFER_SIZE);
					break;
				case 'N':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] =
						status_nickname;
					break;
				case '>':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_right_justify;
					break;
				case 'Q':
					new_free(&query_format);
					query_format =
		convert_sub_format(get_string_var(STATUS_QUERY_VAR), 'Q');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] =
						status_query_nick;
					break;
				case 'F':
					new_free(&notify_format);
					notify_format = 
		convert_sub_format(get_string_var(STATUS_NOTIFY_VAR), 'F');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] =
						status_notify_windows;
					break;
				case '@':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_chanop;
					break;
				case '-':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_chanvoice;
					break;
				case 'C':
					new_free(&channel_format);
					channel_format =
		convert_sub_format(get_string_var(STATUS_CHANNEL_VAR), 'C');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++]= status_channel;
					break;
				case 'S':
					new_free(&server_format);
					server_format =
		convert_sub_format(get_string_var(STATUS_SERVER_VAR), 'S');
					strmcat(buffer,"%s",BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_server;
					break;
				case '+':
					new_free(&mode_format);
					mode_format =
		convert_sub_format(get_string_var(STATUS_MODE_VAR), '+');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_mode;
					break;
				case '#':
					new_free(&umode_format);
					umode_format =
		convert_sub_format(get_string_var(STATUS_UMODE_VAR), '#');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_umode;
					break;
				case 'M':
					new_free(&mail_format);
					mail_format =
		convert_sub_format(get_string_var(STATUS_MAIL_VAR), 'M');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_mail;
					break;
				case 'G':
					new_free(&msgs_format);
					msgs_format =
		convert_sub_format(get_string_var(STATUS_MSGS_VAR), 'G');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_msgs;
					break;
				case 'I':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] =
						status_insert_mode;
					break;
				case 'O':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] =
						status_overwrite_mode;
					break;
				case 'A':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_away;
					break;
				case 'V':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_version;
					break;
				case 'R':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_refnum;
					break;
				case 'T':
					new_free(&clock_format);
					clock_format =
		convert_sub_format(get_string_var(STATUS_CLOCK_VAR), 'T');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_clock;
					break;
				case 'U':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_user0;
					break;
				case 'H':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_hold;
					break;
				case 'B':
					new_free(&hold_lines_format);
					hold_lines_format =
		convert_sub_format(get_string_var(STATUS_HOLD_LINES_VAR), 'B');
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] =
						status_hold_lines;
					break;
				case '*':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_oper;
					break;
				case 'W':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_window;
					break;
				case 'X':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_user1;
					break;
				case 'Y':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_user2;
					break;
				case 'Z':
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
					status_func[func_cnt++] = status_user3;
					break;
		/* no default..?? - phone, jan 1993 */
		/* empty is a good default -lynx, mar 93 */
				}
			}
			else
				ptr++;
		}
		else
			strmcat(buffer, format, BIG_BUFFER_SIZE);
		format = ptr;
	}
	/* this frees the old str first */
	malloc_strcpy(&malloc_ptr, buffer);
	return (malloc_ptr);
}

void
build_status(format)
	char	*format;
{
	int	i;

	new_free(&status_format);
	func_cnt = 0;
	if ((format = get_string_var(STATUS_FORMAT_VAR)) != NULL)
		status_format = convert_format(format);	/* convert_format
							 * mallocs for
							 * us */
	for (i = func_cnt; i < MAX_FUNCTIONS; i++)
		status_func[i] = status_null_function;
	update_all_status();
}

void
make_status(window)
	Window	*window;
{
	int	i,
	len,
	RJustifyPos = -1,
	RealPosition;
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	*func_value[MAX_FUNCTIONS];

	if (!dumb && status_format)
	{
		for (i = 0; i < MAX_FUNCTIONS; i++)
			func_value[i] = (status_func[i]) (window);
		buffer[0] = REV_TOG;
		sprintf(buffer+1, status_format, func_value[0], func_value[1],
		    func_value[2], func_value[3], func_value[4], func_value[5],
		    func_value[6], func_value[7], func_value[8], func_value[9],
		    func_value[10], func_value[11], func_value[12],
		    func_value[13], func_value[14], func_value[15],
		    func_value[16], func_value[17], func_value[18],
		    func_value[19], func_value[20], func_value[21],
		    func_value[22], func_value[23], func_value[24],
		    func_value[25], func_value[26], func_value[27],
		    func_value[28], func_value[29], func_value[30],
		    func_value[31], func_value[32]);
		for (i = 0; i < MAX_FUNCTIONS; i++)
			new_free(&(func_value[i]));

		/*  Patched 26-Mar-93 by Aiken
		 *  make_window now right-justifies everything after a %>
		 *  it's also more efficient.
		 */

		RealPosition = 0;
		for (i = 0; buffer[i]; i++)
			/* formfeed is a marker for left/right border */
			if (buffer[i] == '\f')
			{
				RJustifyPos = i;
			}
			else if (buffer[i] != REV_TOG && buffer[i] != UND_TOG && buffer[i] != ALL_OFF && buffer[i] != BOLD_TOG && buffer[i] != BLINK_TOG)
			{
				if (RealPosition == CO)
				{
					buffer[i] = '\0';
					break;
				}
				RealPosition++;
			}

		/* note that i points to the nul, RealPosition is vis.chars */

		if (RJustifyPos == -1)
		{
			RJustifyPos = i;
		}
		else
		{
			/* get rid of the marker */
			strcpy(&buffer[RJustifyPos], &buffer[RJustifyPos+1]);
			i--;
		}

		if (get_int_var(FULL_STATUS_LINE_VAR))
		{
			int	diff;
			char	c;

			if (RJustifyPos == 0)
				c = ' ';
			else
				c = buffer[RJustifyPos - 1];

			diff = CO - RealPosition;

			for ( ; i >= RJustifyPos; i--)
				buffer[i + diff] = buffer[i];

			for (i++ ; diff > 0 ; diff--, i++)
				buffer[i] = c;
		}

		len = strlen(buffer);
		buffer[len] = ALL_OFF;
		buffer[len+1] = (char) 0;

		status_make_printable((unsigned char *)buffer, len);

	/*
	 * Thanks to Max Bell (mbell@cie.uoregon.edu) for info about TVI
	 * terminals and the sg terminal capability 
	 */
		RealPosition = 0;
		if (window->status_line && (SG == -1))
		{
			for (i = 0; buffer[i] && window->status_line[i]; i++)
			{
				if (buffer[i] != window->status_line[i])
					break;
				if (buffer[i] != REV_TOG && buffer[i] != UND_TOG && buffer[i] != ALL_OFF && buffer[i] != BOLD_TOG && buffer[i] != BLINK_TOG)
					RealPosition++;
			}
		}
		else
			i = 0;
		if ((len = strlen(buffer + i)) || buffer[i] || !window->status_line || window->status_line[i])
		{
			Screen *old_current_screen;

			old_current_screen = current_screen;
			set_current_screen(window->screen);
			term_move_cursor(RealPosition, window->bottom);
			output_line(buffer, NULL, i);
			cursor_in_display();
			if (term_clear_to_eol())
				term_space_erase(len);
			malloc_strcpy(&window->status_line, buffer);
			set_current_screen(old_current_screen);
		}
	}
	cursor_to_input();
}

static	char	*
status_nickname(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	if ((connected_to_server == 1) && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (!window->current_channel) &&
	    (window->screen->current_window != window))
		malloc_strcpy(&ptr, empty_string);
	else
		malloc_strcpy(&ptr, get_server_nickname(window->server));
	return (ptr);
}

static	char	*
status_server(window)
	Window	*window;
{
	char	*ptr = NULL,
		*rest,
		*name;

	if (connected_to_server != 1)
	{
		if (window->server != -1)
		{
			if (server_format)
			{
				name = get_server_name(window->server);
				if ((rest = (char *) index(name, '.')) != NULL)
				{
					if (is_number(name))
						sprintf(buffer, server_format,
							name);
					else
					{
						*rest = '\0';
						sprintf(buffer, server_format,
							name);
						*rest = '.';
					}
				}
				else
					sprintf(buffer, server_format, name);
			}
			else
				*buffer = '\0';
		}
		else
			strcpy(buffer, "No Server");
	}
	else
		*buffer = '\0';
	malloc_strcpy(&ptr, buffer);
	return (ptr);
}

static	char	*
status_query_nick(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	if (window->query_nick && query_format)
	{
		sprintf(buffer, query_format, window->query_nick);
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_right_justify(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	malloc_strcpy(&ptr, "\f");
	return (ptr);
}

static	char	*
status_notify_windows(window)
	Window	*window;
{
	char	refnum[10];
	int	doneone = 0;
	char	*ptr = (char *) 0;
	int	flag = 1;
	char	buf2[81];

	if (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    window == window->screen->current_window)
	{
		*buf2='\0';
		while ((window = traverse_all_windows(&flag)) != NULL)
		{
			if (window->miscflags & WINDOW_NOTIFIED)
			{
				if (!doneone)
				{
					doneone++;
					sprintf(refnum, "%d", window->refnum);
				}
				else
					sprintf(refnum, ",%d", window->refnum);
				strmcat(buf2, refnum, 81);
			}
		}
	}
	if (doneone && notify_format)
	{
		sprintf(buffer, notify_format, buf2);
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return ptr;
}

static	char	*
status_clock(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	if ((get_int_var(CLOCK_VAR) && clock_format)  &&
	    (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    (window == window->screen->current_window)))
	{
		sprintf(buffer, clock_format, update_clock(GET_TIME));
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_mode(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
		*mode;

	if (window->current_channel)
	{
		mode = get_channel_mode(window->current_channel,window->server);
		if (mode && *mode && mode_format)
		{
			sprintf(buffer, mode_format, mode);
			malloc_strcpy(&ptr, buffer);
			return (ptr);
		}
	}
	malloc_strcpy(&ptr, empty_string);
	return (ptr);
}


static	char	*
status_umode(window)
	Window	*window;
{
	char	*ptr = (char *) 0;
	char	localbuf[32];
	char	*c;

	if ((connected_to_server == 1) && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		malloc_strcpy(&ptr, empty_string);
	else
	{
		c = localbuf;
		if (get_server_operator(window->server))
			*c++ = 'o';
		if (get_server_flag(window->server, USER_MODE_I))
			*c++ = 'i';
		if (get_server_flag(window->server, USER_MODE_W))
			*c++ = 'w';
		if (get_server_flag(window->server, USER_MODE_S))
			*c++ = 's';
		if (get_server_flag(window->server, USER_MODE_Z))
			*c++ = 'z';
		if (get_server_flag(window->server, USER_MODE_C))
			*c++ = 'c';
		if (get_server_flag(window->server, USER_MODE_R))
			*c++ = 'r';
		if (get_server_flag(window->server, USER_MODE_K))
			*c++ = 'k';
		if (get_server_flag(window->server, USER_MODE_F))
			*c++ = 'f';
		if (get_server_flag(window->server, USER_MODE_Y))
			*c++ = 'y';
		if (get_server_flag(window->server, USER_MODE_X))
			*c++ = 'x';
		if (get_server_flag(window->server, USER_MODE_D))
			*c++ = 'd';
		if (get_server_flag(window->server, USER_MODE_N))
			*c++ = 'n';
		if (get_server_flag(window->server, USER_MODE_U))
			*c++ = 'u';
		*c++ = '\0';
		if (*localbuf!='\0')
		{
			sprintf(buffer, umode_format, localbuf);
			malloc_strcpy(&ptr, buffer);
		}
		else
			malloc_strcpy(&ptr, empty_string);
	}
	return (ptr);
}

static	char	*
status_chanop(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
		*text;

	if (window->current_channel &&
	    get_channel_oper(window->current_channel, window->server) &&
	    (text = get_string_var(STATUS_CHANOP_VAR)))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}


static	char	*
status_chanvoice(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
		*text;

	if (window->current_channel &&
	    !get_channel_oper(window->current_channel, window->server) &&
	    get_channel_voice(window->current_channel, window->server) &&
	    (text = get_string_var(STATUS_CHANVOICE_VAR)))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}


static	char	*
status_hold_lines(window)
	Window	*window;
{
	char	*ptr = (char *) 0;
	int	num;
	char	localbuf[40];

	num = window->held_lines - window->held_lines%10;
	if (num)
	{
		sprintf(localbuf, "%d", num);
		sprintf(buffer, hold_lines_format, localbuf);
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_channel(window)
	Window	*window;
{
	int	num;
	char	*ptr,
		channel[IRCD_BUFFER_SIZE + 1];

	if (window->current_channel)
	{
		if (get_int_var(HIDE_PRIVATE_CHANNELS_VAR) &&
		    is_channel_mode(window->current_channel,
				MODE_PRIVATE | MODE_SECRET,
				window->server))
			ptr = "*private*";
		else
			ptr = window->current_channel;
		strmcpy(channel, ptr, IRCD_BUFFER_SIZE);
		if ((num = get_int_var(CHANNEL_NAME_WIDTH_VAR)) &&
		    (strlen(channel) > num))
			channel[num] = (char) 0;
		/* num = atoi(channel); */
		ptr = (char *) 0;
		sprintf(buffer, channel_format, channel);
		malloc_strcpy(&ptr, buffer);
	}
	else
	{
		ptr = (char *) 0;
		malloc_strcpy(&ptr, empty_string);
	}
	return (ptr);
}

static	char	*
status_mail(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
		*number;

	if ((get_int_var(MAIL_VAR) && (number = check_mail()) && mail_format) &&
	    (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    (window == window->screen->current_window)))
	{
		sprintf(buffer, mail_format, number);
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_msgs(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
		*number;

	if (((number = count_messages()) && msgs_format) &&
	    (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    (window == window->screen->current_window)))
	{
		sprintf(buffer, msgs_format, number);
		malloc_strcpy(&ptr, buffer);
	}
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_insert_mode(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	text = empty_string;
	if (get_int_var(INSERT_MODE_VAR) && (get_int_var(SHOW_STATUS_ALL_VAR)
	    || (window->screen->current_window == window)))
	{
		if ((text = get_string_var(STATUS_INSERT_VAR)) == (char *) 0)
			text = empty_string;
	}
	malloc_strcpy(&ptr, text);
	return (ptr);
}

static	char	*
status_overwrite_mode(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	text = empty_string;
	if (!get_int_var(INSERT_MODE_VAR) && (get_int_var(SHOW_STATUS_ALL_VAR)
	    || (window->screen->current_window == window)))
	{
	    if ((text = get_string_var(STATUS_OVERWRITE_VAR)) == (char *) 0)
		text = empty_string;
	}
	malloc_strcpy(&ptr, text);
	return (ptr);
}

static	char	*
status_away(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	if ((connected_to_server == 1) && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		malloc_strcpy(&ptr, empty_string);
	else
	{
		if (server_list[window->server].away &&
				(text = get_string_var(STATUS_AWAY_VAR)))
			malloc_strcpy(&ptr, text);
		else
			malloc_strcpy(&ptr, empty_string);
	}
	return (ptr);
}

static	char	*
status_user0(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	if ((text = get_string_var(STATUS_USER_VAR)) &&
	    (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    (window == window->screen->current_window)))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static  char    *
status_user1(window)
	Window  *window;
{
        char    *ptr = (char *) 0,
        *text;

        if ((text = get_string_var(STATUS_USER1_VAR)) &&
            (get_int_var(SHOW_STATUS_ALL_VAR) ||
            (window == window->screen->current_window)))
                malloc_strcpy(&ptr, text);
        else
                malloc_strcpy(&ptr, empty_string);
        return (ptr);
}

static  char    *
status_user2(window)
	Window  *window;
{
        char    *ptr = (char *) 0,
        *text;

        if ((text = get_string_var(STATUS_USER2_VAR)) &&
            (get_int_var(SHOW_STATUS_ALL_VAR) ||
            (window == window->screen->current_window)))
                malloc_strcpy(&ptr, text);
        else
                malloc_strcpy(&ptr, empty_string);
        return (ptr);
}

static  char    *
status_user3(window)
	Window  *window;
{
        char    *ptr = (char *) 0,
        *text;

        if ((text = get_string_var(STATUS_USER3_VAR)) &&
            (get_int_var(SHOW_STATUS_ALL_VAR) ||
            (window == window->screen->current_window)))
                malloc_strcpy(&ptr, text);
        else
                malloc_strcpy(&ptr, empty_string);
        return (ptr);
}

static	char	*
status_hold(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	if (window->held && (text = get_string_var(STATUS_HOLD_VAR)))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_oper(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	if (get_server_operator(window->server) &&
			(text = get_string_var(STATUS_OPER_VAR)) &&
			(get_int_var(SHOW_STATUS_ALL_VAR) ||
			connected_to_server != 1 || 
			(window->screen->current_window == window)))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_window(window)
	Window	*window;
{
	char	*ptr = (char *) 0,
	*text;

	if ((text = get_string_var(STATUS_WINDOW_VAR)) &&
	    (number_of_windows() > 1) && (window->screen->current_window == window))
		malloc_strcpy(&ptr, text);
	else
		malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

static	char	*
status_refnum(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	if (window->name)
		malloc_strcpy(&ptr, window->name);
	else
	{
		sprintf(buffer, "%u", window->refnum);
		malloc_strcpy(&ptr, buffer);
	}
	return (ptr);
}

static	char	*
status_version(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	if ((connected_to_server == 1) && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		malloc_strcpy(&ptr, empty_string);
	else
	{
		malloc_strcpy(&ptr, irc_version);
	}
	return (ptr);
}


static	char	*
status_null_function(window)
	Window	*window;
{
	char	*ptr = (char *) 0;

	malloc_strcpy(&ptr, empty_string);
	return (ptr);
}

/*
 * pass an already allocated char array with n bits, and this
 * gets rid of nasty unprintables.
 */
static	void
status_make_printable(str, n)
	unsigned char	*str;
	int n;
{
	unsigned char	*s;
	int	pos;
	char	buffer[BIG_BUFFER_SIZE];

	if (!str || !*str)
		return;

	bzero(buffer, BIG_BUFFER_SIZE);
	for (pos = 0, s = str; s && pos < BIG_BUFFER_SIZE && pos < n; s++)
	{
		if (translation)
			*s = transToClient[*s];
		if (*s < 32)
		{
			switch(*s)
			{
			case UND_TOG:
			case ALL_OFF:
			case REV_TOG:
			case BOLD_TOG:
			case BLINK_TOG:
				buffer[pos++] = *s;
				break;
			default:
				buffer[pos++] = REV_TOG;
				buffer[pos++] = (*s & 0x7f) | 0x40;
				buffer[pos++] = REV_TOG;
				break;
			}
		}
		else if ((u_char) 0x7f == *s)
		{
			buffer[pos++] = REV_TOG;
			buffer[pos++] = '?';
			buffer[pos++] = REV_TOG;
		}
		else
			buffer[pos++] = *s;
	}
	buffer[pos] = '\0';
	strncpy((char *)str, buffer, pos);
}
