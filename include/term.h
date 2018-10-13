/*
 * term.h: header file for term.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: term.h,v 1.2 2001/09/01 01:11:08 toast Exp $
 */

#ifndef _TERM_H_
# define _TERM_H_

#ifdef MUNIX
# include <sys/ttold.h>
#endif

#include <termcap.h>

extern	int	term_reset_flag;
extern	char	*CM,
		*DO,
		*CE,
		*CL,
		*CR,
		*NL,
		*SO,
		*SE,
		*US,
		*UE,
		*MB,
		*MD,
		*ME,
		*BL;
extern	int	CO,
		LI,
		SG;

extern	int	putchar_x(int c);
#define tputs_x(s)		(tputs(s, 0, putchar_x))

#define term_underline_on()	(tputs_x(US))
#define term_underline_off()	(tputs_x(UE))
#define term_standout_on()	(tputs_x(SO))
#define term_standout_off()	(tputs_x(SE))
#define term_clear_screen()	(tputs_x(CL))
#define term_move_cursor(c, r)	(tputs_x(tgoto(CM, (c), (r))))
#define term_cr()		(tputs_x(CR))
#define term_newline()		(tputs_x(NL))
#define term_beep()		(tputs_x(BL),fflush(current_screen ? \
					current_screen->fpout : stdout))
#define	term_bold_on()		(tputs_x(MD))
#define	term_bold_off()		(tputs_x(ME))
#define	term_blink_on()		(tputs_x(MB))
#define	term_blink_off()	(tputs_x(ME))

extern	RETSIGTYPE	term_cont();
extern	int	term_echo();
extern	void	term_init();
extern	int	term_resize();
extern	void	term_pause();
extern	void	term_putchar();
extern	int	term_puts();
extern	void	term_flush();
extern	int	(*term_scroll)();
extern	int	(*term_insert)();
extern	int	(*term_delete)();
extern	int	(*term_cursor_right)();
extern	int	(*term_cursor_left)();
extern	int	(*term_clear_to_eol)();
extern	void	term_space_erase();
extern	void	term_reset();

extern  void    copy_window_size();
extern	int	term_eight_bit();
extern	void	set_term_eight_bit _((int));

#if defined(ISC22) || defined(MUNIX)
/* Structure for terminal special characters */
struct	tchars
{
	char	t_intrc;	/* Interrupt			*/
	char	t_quitc;	/* Quit 			*/
	char	t_startc;	/* Start output 		*/
	char	t_stopc;	/* Stop output			*/
	char	t_eofc;		/* End-of-file (EOF)		*/
	char	t_brkc;		/* Input delimiter (like nl)	*/
}

struct ltchars
{
	char	t_suspc;	/* stop process signal		*/
	char	t_dsuspc;	/* delayed stop process signal	*/
	char	t_rprntc;	/* reprint line			*/
	char	t_flushc;	/* flush output (toggles)	*/
	char	t_werasc;	/* word erase			*/
	char	t_lnextc;	/* literal next character	*/
};
#endif /* ISC22 || MUNIX */

#if defined(_HPUX_SOURCE)

#ifndef _TTY_CHARS_ST_
#define _TTY_CHARS_ST_

/* Structure for terminal special characters */
struct tchars
{
	char	t_intrc;	/* Interrupt			*/
	char	t_quitc;	/* Quit 			*/
	char	t_startc;	/* Start output 		*/
	char	t_stopc;	/* Stop output			*/
	char	t_eofc;		/* End-of-file (EOF)		*/
	char	t_brkc;		/* Input delimiter (like nl)	*/
};

#endif /* _TTY_CHARS_ST_ */

#ifndef TIOCSETC
# define TIOCSETC	_IOW('t', 17, struct tchars)	/* set special chars */
#endif /* TIOCSETC */

#ifndef TIOCGETC
# define TIOCGETC	_IOR('t', 18, struct tchars)	/* get special chars */
#endif /* TIOCGETC */

#ifndef CBREAK
# define CBREAK		0x02	/* Half-cooked mode */
#endif /* CBREAK */

#ifndef SIGWINCH
# define    SIGWINCH    SIGWINDOW
#endif /* SIGWINCH */

#endif /* _HPUX_SOURCE */

/* well, it works */
#ifdef mips
# define fputc(c,f) write(1,&(c),1)
# define fwrite(buffer,len,cnt,f) write(1,buffer,len)
#endif /*mips*/

#endif /* _TERM_H_ */
