/*
 * term.c: termcap stuff... 
 *
 * Written By Michael Sandrof
 * HP-UX modifications by Mark T. Dame (Mark.Dame@uc.edu) 
 * Termio modifications by Stellan Klebom (d88-skl@nada.kth.se) 
 * Copyright(c) 1990 
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * i want to clean this file up, but i can't be bothered.  anyone
 * wanna do it for me ?
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: term.c,v 1.4.2.1 2002/03/01 23:06:34 toast Exp $";
#endif

#include "irc.h"

#ifdef ESIX
# include <lan/net_types.h>
# include <lan/net_ioctl.h>
# include <sgtty.h>
# include <termio.h>
#endif /* ESIX */

#ifdef XD88
# include <sys/termios.h>
#endif /* XD88 */

#ifdef SVR3
# include <sys/stat.h>
# include <sgtty.h>
# include <sys/stream.h>
# ifdef HAVE_SYS_PTEM_H
#  include <sys/ptem.h>
# endif
# define CBREAK RAW
# define USE_TERMIO /* bah */
#endif /* SVR3 */

#ifdef SVR4
# include <sys/stat.h>
# include <termios.h>
# include <sys/ttold.h>
# include <sys/stream.h>
# include <sys/ttcompat.h>
# define CBREAK RAW
# define USE_TERMIO
#endif /* SVR4 */

#ifdef M_UNIX
# include <sys/stat.h>
# include <sys/stream.h>
# include <sys/ptem.h>
# include <termio.h>
#endif /* M_UNIX */

#ifdef HPUX
# define _TERMIOS_INCLUDED
# define _INCLUDE_TERMIO
# include <sys/termio.h>
# define USE_TERMIO
#endif /* HPUX */

#include <sys/ioctl.h>
#include "term.h"
#include "translat.h"

#ifdef ISC
# include <sys/sioctl.h>
# define USE_TERMIO
#endif /* ISC22 */

#ifdef USE_TERMIO
# include <sys/termio.h>
#endif /* USE_TERMIO */

#ifdef ISC22
# undef TIOCSETC
# undef TIOCGETC
#endif /* ISC22 */

#ifdef USING_CURSES
# include <curses.h>
#endif /* USING_CURSES */

#if defined(POSIX) && !defined(sun) && !defined(_IBMR2)
# ifdef HAVE_SYS_TTOLD_H
#  include <sys/ttold.h>
# endif /* HAVE_SYS_TTOLD_H */
#endif /* POSIX && !sun && !_IBMR2 */

#ifdef __linux__
# include <termios.h>
# define USE_TERMIO
#endif

#ifdef __CYGWIN__
# define TERMCAPSIZE	4096	/* cygwin needs a huge-ass buffer */
#else
# define TERMCAPSIZE	1024
#endif

/*
 * net/2 defined this as 0.5, and 4.4 as 1..
 * i hope other systems based on this don't change it (netbsd,
 * bsdi, freebsd ..)
 */
#ifdef BSD4_4
# include <termios.h>
# define termio termios
# define USE_TERMIO
#endif

#ifdef __QNX__
# include <termios.h>
# define termio termios
# define USE_TERMIO
# undef TIOCLSET
# undef TCSETA
# undef TCGETA
#endif

/* Missing on ConvexOS, idea picked from SunOS */
#if defined(__convex__) && !defined(LPASS8)
# define LPASS8 (L004000>>16)
#endif

#ifdef HAVE_TERMBITS_H
# include <termbits.h>
#endif

#include "window.h"
#include "screen.h"

#ifndef	STTY_ONLY
extern	char	*tgetstr();
extern	int	tgetent();
extern	char	*getenv();
extern	void	new_stty();

static	int	term_CE_clear_to_eol();
static	int	term_CS_scroll();
static	int	term_ALDL_scroll();
static	int	term_param_ALDL_scroll();
static	int	term_IC_insert();
static	int	term_IMEI_insert();
static	int	term_DC_delete();
static	int	term_null_function();
static	int	term_BS_cursor_left();
static	int	term_LE_cursor_left();
static	int	term_ND_cursor_right();
#endif /* STTY_ONLY */

static	int	tty_des;		/* descriptor for the tty */

#ifndef USE_TERMIO
static	struct	ltchars	oldltchars,
			newltchars = { -1, -1, -1, -1, -1, -1};
static	struct	tchars	oldtchars,
			newtchars = { '\003', -1, -1, -1, -1, -1};
static	struct	sgttyb	oldb,
			newb;
#else
static	struct	termio	oldb,
			newb;
#endif /* USE_TERMIO */

#ifndef STTY_ONLY

static	char	termcap[TERMCAPSIZE];

/*
 * Function variables: each returns 1 if the function is not supported on the
 * current term type, otherwise they do their thing and return 0 
 */
int	(*term_scroll) ();	/* this is set to the best scroll available */
int	(*term_insert) ();	/* this is set to the best insert available */
int	(*term_delete) ();	/* this is set to the best delete available */
int	(*term_cursor_left) ();	/* this is set to the best left available */
int	(*term_cursor_right) ();/* this is set to the best right available */
int	(*term_clear_to_eol) ();/* this is set... figure it out */

/* The termcap variables */
char	*CM,
	*CE,
	*CL,
	*CR,
	*NL,
	*AL,
	*DL,
	*CS,
	*DC,
	*IC,
	*IM,
	*EI,
	*SO,
	*SE,
	*US,
	*UE,
	*MB,
	*MD,
	*ME,
	*SF,
	*SR,
	*ND,
	*LE,
	*BL,
	*AF,
	*AB;
int	CO = 79,
	LI = 24,
	SG;

/*
 * term_reset_flag: set to true whenever the terminal is reset, thus letter
 * the calling program work out what to do 
 */
int	term_reset_flag = 0;

static	int	term_echo_flag = 1;

static	int	li,
		co;

#endif /* STTY_ONLY */
#ifdef TIOCLSET
static	int	old_local_modes,
		new_local_modes;
#endif

#ifndef STTY_ONLY
void
term_intr(c)
	char	c;
{
#if defined(TIOCSETC) && !defined(USE_TERMIO)
	newtchars.t_intrc = c;
	ioctl(tty_des, TIOCSETC, &newtchars);
#endif /* TIOCSETC */
}

void
term_susp(c)
	char	c;
{
#ifndef USE_TERMIO
	newltchars.t_suspc = c;
	ioctl(tty_des, TIOCSLTC, &newltchars);
#endif /* USE_TERMIO */
}

/*
 * term_echo: if 0, echo is turned off (all characters appear as blanks), if
 * non-zero, all is normal.  The function returns the old value of the
 * term_echo_flag 
 */
int
term_echo(flag)
	int	flag;
{
	int	echo;

	echo = term_echo_flag;
	term_echo_flag = flag;
	return (echo);
}

/*
 * term_putchar: puts a character to the screen, and displays control
 * characters as inverse video uppercase letters.  NOTE:  Dont use this to
 * display termcap control sequences!  It won't work! 
 */
void
term_putchar(c)
	unsigned char	c;
{
	if (term_echo_flag)
	{
		if (translation)
			c = transToClient[c];
		if (c < 32)
		{
			term_standout_on();
			c = (c & 127) | 64;
			fputc(c, (current_screen?current_screen->fpout:stdout));
			term_standout_off();
		}
		else if (c == '\177')
		{
			term_standout_on();
			c = '?';
			fputc(c, (current_screen?current_screen->fpout:stdout));
			term_standout_off();
		}
		else
			fputc(c, (current_screen?current_screen->fpout:stdout));
	}
	else
	{
		c = ' ';
		fputc(c, (current_screen?current_screen->fpout:stdout));
	}
}

/* term_puts: uses term_putchar to print text */
int
term_puts(str, len)
	char	*str;
	int	len;
{
	int	i;

	for (i = 0; *str && (i < len); str++, i++)
		term_putchar(*str);
	return (i);
}

/* putchar_x: the putchar function used by tputs */
int
putchar_x(c)
	int	c;
{
	return fputc(c, (current_screen?current_screen->fpout:stdout));
}

void
term_flush()
{
	fflush((current_screen?current_screen->fpout:stdout));
}

/*
 * term_reset: sets terminal attributed back to what they were before the
 * program started 
 */
void
term_reset()
{
#ifndef USE_TERMIO
	ioctl(tty_des, TIOCSLTC, &oldltchars);
	ioctl(tty_des, TIOCSETC, &oldtchars);
	ioctl(tty_des, TIOCSETP, &oldb);
#else
# ifdef TCSETA
	ioctl(tty_des, TCSETA, &oldb);
# else
	tcsetattr(tty_des, TCSADRAIN, &oldb);
# endif /* TCSETA */
#endif /* USE_TERMIO */

#if (defined(mips) && !defined(ultrix)) || defined(ISC22) || defined(_HPUX_SOURCE)
	new_stty("cooked");
#endif /* mips */

#ifdef TIOCLSET
	ioctl(tty_des, TIOCLSET, &old_local_modes);
#endif
	if   (CS)
		tputs_x(tgoto(CS, LI - 1, 0));
	term_move_cursor(0, LI - 1);
	term_reset_flag = 1;
	term_flush();
}

/*
 * term_cont: sets the terminal back to IRCII stuff when it is restarted
 * after a SIGSTOP.  Somewhere, this must be used in a signal() call 
 */
RETSIGTYPE
term_cont()
{
#ifdef SYSVSIGNALS
	(void) MY_SIGNAL(SIGCONT, term_cont, 0);	/* sysv has dumb signals */
#endif /* SYSVSIGNALS */

#if defined(SIGSTOP) && defined(SIGTSTP)	/* munix doesn't have sigstop sigtstp */
# ifdef USE_TERMIO
#  ifdef TCSETA
	ioctl(tty_des, TCSETA, &newb);
#  else
	tcsetattr(tty_des, TCSADRAIN, &newb);
#  endif /* TCSETA */
# else
	ioctl(tty_des, TIOCSLTC, &newltchars);
	ioctl(tty_des, TIOCSETC, &newtchars);
	ioctl(tty_des, TIOCSETP, &newb);
# endif /* USE_TERMIO */
# if defined(_HPUX_SOURCE) || defined(ISC22)
	new_stty("opost");
# endif /* HPUX || ISC22 */
# if defined(mips) && !defined(ultrix) /*ultrix/mips silliness*/
	new_stty("raw -echo");
# endif /* mips */
#endif /* SIGSTOP && SIGTSTP */

#ifdef TIOCLSET
	ioctl(tty_des, TIOCLSET, &new_local_modes);
#endif
}

/*
 * term_pause: sets terminal back to pre-program days, then SIGSTOPs itself. 
 */
void
term_pause()
{
#if !defined(SIGSTOP) || !defined(SIGTSTP) || defined(_RT) || defined(ESIX)
	say("The STOP_IRC function does not work on this system type.");
#else
	term_reset();
	kill(getpid(), SIGSTOP);
#endif /* MUNIX */
}
#endif /* STTY_ONLY */

/*
 * term_init: does all terminal initialization... reads termcap info, sets
 * the terminal to CBREAK, no ECHO mode.   Chooses the best of the terminal
 * attributes to use ..  for the version of this function that is called for
 * wserv, we set the termial to RAW, no ECHO, so that all the signals are
 * ignored.. fixes quite a few problems...  -phone, jan 1993..
 */
void
term_init()
{
#ifndef	STTY_ONLY
	char	bp[TERMCAPSIZE],
		*term,
		*ptr;

	if ((term = getenv("TERM")) == (char *) 0)
	{
		fprintf(stderr, "irc: No TERM variable set!\n");
		fprintf(stderr,"irc: You may still run irc by using the -d switch\n");
		exit(1);
	}
	if (tgetent(bp, term) < 1)
	{
		fprintf(stderr, "irc: No termcap entry for %s.\n", term);
		fprintf(stderr,"irc: You may still run irc by using the -d switch\n");
		exit(1);
	}
#endif
	if (getenv("IRC_DEBUG")|| (tty_des = open("/dev/tty", O_RDWR, 0)) == -1)
		tty_des = 0;

#ifndef USE_TERMIO
	ioctl(tty_des, TIOCGLTC, &oldltchars);
	ioctl(tty_des, TIOCGETC, &oldtchars);
	ioctl(tty_des, TIOCGETP, &oldb);

	newb = oldb;
	newb.sg_flags &= ~CRMOD;

# ifdef TIOCLSET
	ioctl(tty_des, TIOCLGET, &old_local_modes);
	new_local_modes = old_local_modes | LDECCTQ | LLITOUT | LNOFLSH;
	ioctl(tty_des, TIOCLSET, &new_local_modes);
# endif

# ifndef STTY_ONLY
	if (use_flow_control)
	{
		newtchars.t_startc = oldtchars.t_startc;
		newtchars.t_stopc = oldtchars.t_stopc;
	}
	newb.sg_flags |= CBREAK;
# else
	newb.sg_flags |= RAW;
# endif /* STTY_ONLY */

# if !defined(_HPUX_SOURCE)
	newb.sg_flags &= (~ECHO);
# endif /* _HPUX_SOURCE */

	ioctl(tty_des, TIOCSLTC, &newltchars);
	ioctl(tty_des, TIOCSETC, &newtchars);
	ioctl(tty_des, TIOCSETP, &newb);

#else /* USE_TERMIO */

# ifdef TCGETA
	ioctl(tty_des, TCGETA, &oldb);
# else
	tcgetattr(tty_des, &oldb);
# endif
	newb = oldb;
	newb.c_lflag &= ~(ICANON | ECHO);	/* set equivalent of
						 * CBREAK and * no
						 * ECHO */
	newb.c_cc[VMIN] = 1;	/* read() satified after 1 char */
	newb.c_cc[VTIME] = 0;	/* No timer */
#ifndef _POSIX_VDISABLE
# define _POSIX_VDISABLE 0
#endif

	newb.c_cc[VQUIT] = _POSIX_VDISABLE;
# ifdef VDSUSP
	newb.c_cc[VDSUSP] = _POSIX_VDISABLE;
# endif
# ifdef VSUSP
	newb.c_cc[VSUSP] = _POSIX_VDISABLE;
# endif

# ifndef STTY_ONLY
	if (!use_flow_control)
		newb.c_iflag &= ~IXON;	/* No XON/XOFF */
# endif /* STTY_ONLY */

# ifdef TCSETA
	ioctl(tty_des, TCSETA, &newb);
# else
	tcsetattr(tty_des, TCSADRAIN, &newb);
# endif

#endif /* USE_TERMIO */

#ifndef STTY_ONLY
#if defined(mips) && !defined(ultrix)
	new_stty("raw -echo");
#endif /* mips */
#endif /* STTY_ONLY */

#ifndef STTY_ONLY
	if ((co = tgetnum("co")) == -1)
		co = 80;
	if ((li = tgetnum("li")) == -1)
		li = 24;
	ptr = termcap;

	/*
	 * Thanks to Max Bell (mbell@cie.uoregon.edu) for info about TVI
	 * terminals and the sg terminal capability 
	 */
	SG = tgetnum("sg");
	CM = tgetstr("cm", &ptr);
	CL = tgetstr("cl", &ptr);
	if ((CM == (char *) 0) ||
	    (CL == (char *) 0))
	{
		fprintf(stderr, "This terminal does not have the necessary capabilities to run IRCII\nin full screen mode. You may still run irc by using the -d switch\n");
		exit(1);
	}
	if ((CR = tgetstr("cr", &ptr)) == (char *) 0)
		CR = "\r";
	if ((NL = tgetstr("nl", &ptr)) == (char *) 0)
		NL = "\n";

	if ((CE = tgetstr("ce", &ptr)) != NULL)
		term_clear_to_eol = term_CE_clear_to_eol;
	else
		term_clear_to_eol = term_null_function;


	/* if ((ND = tgetstr("nd", &ptr)) || (ND = tgetstr("kr", &ptr))) */
	if ((ND = tgetstr("nd", &ptr)) != NULL)
		term_cursor_right = term_ND_cursor_right;
	else
		term_cursor_right = term_null_function;

	/* if ((LE = tgetstr("le", &ptr)) || (LE = tgetstr("kl", &ptr))) */
	if ((LE = tgetstr("le", &ptr)) != NULL)
		term_cursor_left = term_LE_cursor_left;
	else if (tgetflag("bs"))
		term_cursor_left = term_BS_cursor_left;
	else
		term_cursor_left = term_null_function;

	SF = tgetstr("sf", &ptr);
	SR = tgetstr("sr", &ptr);

	if ((CS = tgetstr("cs", &ptr)) != NULL)
		term_scroll = term_CS_scroll;
	else if ((AL = tgetstr("AL", &ptr)) && (DL = tgetstr("DL", &ptr)))
		term_scroll = term_param_ALDL_scroll;
	else if ((AL = tgetstr("al", &ptr)) && (DL = tgetstr("dl", &ptr)))
		term_scroll = term_ALDL_scroll;
	else
		term_scroll = term_null_function;

	if ((IC = tgetstr("ic", &ptr)) != NULL)
		term_insert = term_IC_insert;
	else
	{
		if ((IM = tgetstr("im", &ptr)) && (EI = tgetstr("ei", &ptr)))
			term_insert = term_IMEI_insert;
		else
			term_insert = term_null_function;
	}

	if ((DC = tgetstr("dc", &ptr)) != NULL)
		term_delete = term_DC_delete;
	else
		term_delete = term_null_function;

	SO = tgetstr("so", &ptr);
	SE = tgetstr("se", &ptr);
	if ((SO == (char *) 0) || (SE == (char *) 0))
	{
		SO = empty_string;
		SE = empty_string;
	}
	US = tgetstr("us", &ptr);
	UE = tgetstr("ue", &ptr);
	if ((US == (char *) 0) || (UE == (char *) 0))
	{
		US = empty_string;
		UE = empty_string;
	}

	/* Start blinking */
	MB = tgetstr("mb", &ptr);
	if (MB == (char *) 0)
	{
		MB = empty_string;
	}

	/* Start bold mode */
	MD = tgetstr("md", &ptr);

	/* End all mode like so, us, mb, md and mr */
	ME = tgetstr("me", &ptr);

	if ((MD == (char *) 0) || (ME == (char *) 0))
	{
		MB = empty_string;
		MD = empty_string;
		ME = empty_string;
	}

	/* Bell */
	if ((BL = tgetstr("bl", &ptr)) == (char *) 0)
		BL = "\007";

	/* ANSI foreground color */
	if ((AF = tgetstr("AF", &ptr)) == (char *) 0)
		AF = empty_string;

	/* ANSI background color */
	if ((AB = tgetstr("AB", &ptr)) == (char *) 0)
		AB = empty_string;

#endif /* STTY_ONLY */
}

#ifndef STTY_ONLY
/*
 * term_resize: gets the terminal height and width.  Trys to get the info
 * from the tty driver about size, if it can't... uses the termcap values. If
 * the terminal size has changed since last time term_resize() has been
 * called, 1 is returned.  If it is unchanged, 0 is returned. 
 */
int
term_resize()
{
	static	int	old_li = -1,
			old_co = -1;

#ifndef TIOCGWINSZ

	LI = li;
	CO = co;

#else
	struct	winsize window;

	if (ioctl(tty_des, TIOCGWINSZ, &window) < 0)
	{
		LI = li;
		CO = co;
	}
	else
	{
		if ((LI = window.ws_row) == 0)
			LI = li;
		if ((CO = window.ws_col) == 0)
			CO = co;
	}
#endif /* TIOCGWINSZ */
	CO--;
	if ((old_li != LI) || (old_co != CO))
	{
		old_li = LI;
		old_co = CO;
		return (1);
	}
	return (0);
}

/*
 * term_null_function: used when a terminal is missing a particulary useful
 * feature, such as scrolling, to warn the calling program that no such
 * function exists 
 */
static	int
term_null_function()
{
	return (1);
}

/* term_CE_clear_to_eol(): the clear to eol function, right? */
static	int
term_CE_clear_to_eol()
{
	tputs_x(CE);
	return (0);
}

/* * term_space_erase: this can be used if term_CE_clear_to_eol() returns 1.
 * This will erase from x to the end of the screen uses space.  Actually, it
 * doesn't reposition the cursor at all, so the cursor must be in the correct
 * spot at the beginning and you must move it back afterwards 
 */
void
term_space_erase(x)
	int	x;
{
	char	c = ' ';
	int	i,
		cnt;

	cnt = CO - x;
	for (i = 0; i < cnt; i++)
		fputc(c, (current_screen?current_screen->fpout:stdout));
}

/*
 * term_CS_scroll: should be used if the terminal has the CS capability by
 * setting term_scroll equal to it 
 */
static	int
term_CS_scroll(line1, line2, n)
	int	line1,
		line2,
		n;
{
	int	i;
	char	*thing;

	if (n > 0)
		thing = SF ? SF : NL;
	else if (n < 0)
	{
		if (SR)
			thing = SR;
		else
			return 1;
	}
	else
		return 0;
	tputs_x(tgoto(CS, line2, line1));  /* shouldn't do this each time */
	if (n < 0)
	{
		term_move_cursor(0, line1);
		n = -n;
	}
	else
		term_move_cursor(0, line2);
	for (i = 0; i < n; i++)
		tputs_x(thing);
	tputs_x(tgoto(CS, LI - 1, 0));	/* shouldn't do this each time */
	return (0);
}

/*
 * term_ALDL_scroll: should be used for scrolling if the term has AL and DL
 * by setting the term_scroll function to it 
 */
static	int
term_ALDL_scroll(line1, line2, n)
	int	line1,
		line2,
		n;
{
	int	i;

	if (n > 0)
	{
		term_move_cursor(0, line1);
		for (i = 0; i < n; i++)
			tputs_x(DL);
		term_move_cursor(0, line2 - n + 1);
		for (i = 0; i < n; i++)
			tputs_x(AL);
	}
	else if (n < 0)
	{
		n = -n;
		term_move_cursor(0, line2-n+1);
		for (i=0; i < n; i++)
			tputs_x(DL);
		term_move_cursor(0, line1);
		for (i=0; i < n; i++)
			tputs_x(AL);
	}
	return (0);
}

/*
 * term_param_ALDL_scroll: Uses the parameterized version of AL and DL 
 */
static	int
term_param_ALDL_scroll(line1, line2, n)
	int	line1,
		line2,
		n;
{
	if (n > 0)
	{
		term_move_cursor(0, line1);
		tputs_x(tgoto(DL, n, n));
		term_move_cursor(0, line2 - n + 1);
		tputs_x(tgoto(AL, n, n));
	}
	else if (n < 0)
	{
		n = -n;
		term_move_cursor(0, line2-n+1);
		tputs_x(tgoto(DL, n, n));
		term_move_cursor(0, line1);
		tputs_x(tgoto(AL, n, n));
	}
	return (0);
}

/*
 * term_IC_insert: should be used for character inserts if the term has IC by
 * setting term_insert to it. 
 */
static	int
term_IC_insert(c)
	char	c;
{
	tputs_x(IC);
	term_putchar(c);
	return (0);
}

/*
 * term_IMEI_insert: should be used for character inserts if the term has IM
 * and EI by setting term_insert to it 
 */
static	int
term_IMEI_insert(c)
	char	c;
{
	tputs_x(IM);
	term_putchar(c);
	tputs_x(EI);
	return (0);
}

/*
 * term_DC_delete: should be used for character deletes if the term has DC by
 * setting term_delete to it 
 */
static	int
term_DC_delete()
{
	tputs_x(DC);
	return (0);
}

/* term_ND_cursor_right: got it yet? */
static	int
term_ND_cursor_right()
{
	tputs_x(ND);
	return (0);
}

/* term_LE_cursor_left:  shouldn't you move on to something else? */
static	int
term_LE_cursor_left()
{
	tputs_x(LE);
	return (0);
}

static	int
term_BS_cursor_left()
{
	char	c = '\010';

	fputc(c, (current_screen ? current_screen->fpout : stdout));
	return (0);
}

extern	void
copy_window_size(lines, columns)
	int	*lines,
		*columns;
{
	*lines = li;
	*columns = co;
}

extern	int
term_eight_bit()
{
#if defined(USE_TERMIO)
	return (((oldb.c_cflag) & CSIZE) == CS8) ? 1 : 0;
#else
	return (old_local_modes & LPASS8) ? 1 : 0;
#endif /* USE_TERMIO */
}
#endif /* STTY_ONLY */

extern	void
set_term_eight_bit(value)
	int	value;
{
#if defined(USE_TERMIO)
	if (value == ON)
	{
		newb.c_cflag |= CS8;
		newb.c_iflag &= ~ISTRIP;
	}
	else
	{
		newb.c_cflag &= ~CS8;
		newb.c_iflag |= ISTRIP;
	}
# ifdef TCSETA
	ioctl(tty_des, TCSETA, &newb);
# else
	tcsetattr(tty_des, TCSADRAIN, &newb);
# endif /* TCSETA */
#else
	if (value == ON)
		new_local_modes |= LPASS8;
	else
		new_local_modes &= ~LPASS8;
	ioctl(tty_des, TIOCLSET, &new_local_modes);
#endif /* USE_TERMIO */
}
