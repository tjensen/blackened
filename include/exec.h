/*
 * exec.h: header for exec.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: exec.h,v 1.1.1.1 1999/07/16 21:21:13 toast Exp $
 */

#ifndef _EXEC_H_
#define _EXEC_H_

#include <sys/types.h>

#if defined(NeXT)		/* lameness for configure/NeXT -phone */
# if !defined(_POSIX_SOURCE) && !defined(BSDWAIT)
#  define BSDWAIT
# endif /* !_POSIX_SOURCE && !BSDWAIT */
#else /* !NeXT */
# ifndef WAITSTUFF_DECLARED
#  ifdef BSDWAIT
#   ifndef WAIT3_DECLARED
struct rusage;
union wait;
extern int   wait3 _((union wait *, int, struct rusage *));
#   endif /* WAIT3_DECLARED */
#  else /* BSDWAIT */
#   ifndef WAITPID_DECLARED
extern short waitpid _((int, int *, int));
#   endif /* WAITPID_DECLARED */
#  endif /* BSDWAIT */
# endif /* WAITSTUFF_DECLARED */
#endif /* NeXT */

#ifndef WTERMSIG
# ifndef BSDWAIT /* if wait is NOT a union: */
#  define WTERMSIG(status) ((status) & 0177)
# else
#  define WTERMSIG(status) status.w_T.w_Termsig
# endif
#endif

#ifndef WEXITSTATUS
# ifndef BSDWAIT
#  define WEXITSTATUS(status) ((status) & 0xff00) >> 8		/* dgux 5.4.1 */
# else
#  define WEXITSTATUS(status) status.w_T.w_Retcode
# endif
#endif

extern	int	get_child_exit _((int));
extern	int	check_wait_status _((int));
extern	void	check_process_list();
extern	void	check_process_limits();
extern	void	do_processes();
extern	void	set_process_bits();
extern	int	text_to_process();
extern	char	*signals[];
extern	void	clean_up_processes();
extern	int	is_process();
extern	int	get_process_index();
extern	void	exec_server_delete();
extern	int	is_process_running();
extern	void	add_process_wait();
extern	void	set_wait_process();
extern	void	close_all_exec();
extern	int	logical_to_index();
extern	void	execcmd _((char *, char *, char *));

#endif /* _EXEC_H_ */
