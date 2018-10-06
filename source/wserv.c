/*
 * wserv.c - little program to be a pipe between a screen or
 * xterm window to the calling ircII process.
 *
 * Copyright Troy Rollo, 1992.
 * 
 * Finished by Matthew Green, 1993.
 *
 * Works by opening up the unix domain socket that ircII bind's
 * before calling wserv, and which ircII also deleted after the
 * connection has been made.
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: wserv.c,v 1.2 2001/12/09 03:35:26 toast Exp $";
#endif

#include "defs.h"

#ifdef HAVE_SYS_UN_H

# include "irc.h"
# include "term.h"

# include <sys/un.h>

/*
 * Holds the parent process id, as taken from the command line arguement.
 * We can't use getppid() here, because the parent for screen is the
 * underlying screen process, and for xterm's, it the xterm itself.
 */
static	pid_t	ircIIpid;

/* declare the signal handler */
# if !defined(_RT) && defined(SIGWINCH)
void	got_sigwinch();
# endif /* _RT */

int
main(argc, argv)
	int	argc;
	char	**argv;
{
	char	buffer[1024];
	struct	sockaddr_un *addr = (struct sockaddr_un *) buffer;
	int	nread;
	fd_set	reads;
	int	s;
	char	*path,
		*tmp;

	/* Set up the signal hander to pass SIGWINCH to ircII */
# if !defined(_RT) && defined(SIGWINCH)
	(void) MY_SIGNAL(SIGWINCH, got_sigwinch, 0);
# endif /* _RT */

	if (2 != argc)    /* no socket is passed */
		exit(0);
	/*
	 * First thing we do here is grab the parent pid from the command
	 * line arguements, because getppid() returns the wrong pid in
	 * all cases.. is comes in via the command line arguement in the
	 * for of irc_xxxxxxxx .. as an 8 digit decimal number..  if we
	 * can't get the pid from the command line arg, then we set it
	 * to -1, which is used later to ignore them..
	 */

	path = (char *) malloc(strlen(argv[1]) + 1);
	strcpy(path, argv[1]);
	if ((char *) 0 != (tmp = (char *) index(path, '_')))
		ircIIpid = atoi(++tmp);
	else
		ircIIpid = -1;

	/*
	 * Set up the socket, from the path passed, connect it.. all that
	 * stuff..  And initalise the term settings for the window.
	 */
	addr->sun_family = AF_UNIX;
	strcpy(addr->sun_path, argv[1]);
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (0 > connect(s, (struct sockaddr *) addr, sizeof(addr->sun_family) +
						strlen(addr->sun_path)))
		exit(0);

	/*
	 * first line to for a wserv program is the tty.  this is so ircii
	 * can grab the size of the tty, and have it changed.
	 */

	tmp = ttyname(0);
	write(s, tmp, strlen(tmp));
	write(s, "\n", 1);
	perror(tmp);

	term_init();

	/*
	 * The select call..  reads from the socket, and from the window..
	 * and pipes the output from out to the other..  nice and simple
	 */
	while (1)
	{
		FD_ZERO(&reads);
		FD_SET(0, &reads);
		FD_SET(s, &reads);
		select(s + 1, &reads, (fd_set *) 0, (fd_set *) 0,
			(struct timeval *) 0);
		if (FD_ISSET(0, &reads))
		{
			if (0 != (nread = read(0, buffer, sizeof(buffer))))
				write(s, buffer, nread);
			else
				exit(0);
		}
		if (FD_ISSET(s, &reads))
		{
			if (0 != (nread = read(s, buffer, sizeof(buffer))))
				write(0, buffer, nread);
			else
				exit(0);
		}
	}
	return 0;
}

/* got_sigwinch: we got a SIGWINCH, so we send it back to ircII */
# if !defined(_RT) && defined(SIGWINCH)
void
got_sigwinch()
{
#  ifdef SYSVSIGNALS
	(void) MY_SIGNAL(SIGWINCH, got_sigwinch, 0);
#  endif
	if (-1 != ircIIpid)
		kill(ircIIpid, SIGWINCH);
}
# endif /* _RT */

#else

void
main()
{
	exit(0);
}

#endif /* HAVE_SYS_UH_H */
