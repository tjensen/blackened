/*
 * ircserv.c: A quaint little program to make irc life PING free 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: ircserv.c,v 1.2 2001/12/09 03:35:25 toast Exp $";
#endif

#include "defs.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#include <sys/file.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

#include "irc_std.h"

/* machines we don't want to use <unistd.h> on 'cause its broken */
#if defined(pyr) || defined(_SEQUENT_)
# undef HAVE_UNISTD_H
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
int	connect_to_unix();
#endif /* HAVE_SYS_UN_H */

#undef NON_BLOCKING

extern	void	set_socket_options _((int));
extern	int	new_select();
extern	int	dgets();

char	*
new_malloc(size)
	int	size;
{
	char	*ptr;

	if ((ptr = (char *) malloc(size)) == (char *) 0)
	{
		printf("-1 0\n");
		exit(1);
	}
	return (ptr);
}

/*
 * new_free:  Why do this?  Why not?  Saves me a bit of trouble here and
 * there 
 */
void
new_free(ptr)
	char	**ptr;
{
	if (*ptr)
	{
		free(*ptr);
		*ptr = (char *) 0;
	}
}

/*
 * Connect_By_Number Performs a connecting to socket 'service' on host
 * 'host'.  Host can be a hostname or ip-address.  If 'host' is null, the
 * local host is assumed.   The parameter full_hostname will, on return,
 * contain the expanded hostname (if possible).  Note that full_hostname is a
 * pointer to a char *, and is allocated by connect_by_numbers() 
 *
 * Errors: 
 *
 * -1 get service failed 
 *
 * -2 get host failed 
 *
 * -3 socket call failed 
 *
 * -4 connect call failed 
 */
int
connect_by_number(service, host)
	int	service;
	char	*host;
{
	int	s;
	char	buf[100];
	struct	sockaddr_in	server;
	struct	hostent	*hp;

	if (host == (char *) 0)
	{
		gethostname(buf, sizeof(buf));
		host = buf;
	}
	if ((server.sin_addr.s_addr = inet_addr(host)) == -1)
	{
		if ((hp = gethostbyname(host)) != NULL)
		{
			bzero((char *) &server, sizeof(server));
			bcopy(hp->h_addr, (char *) &server.sin_addr,
				hp->h_length);
			server.sin_family = hp->h_addrtype;
		}
		else
			return (-2);
	}
	else
		server.sin_family = AF_INET;
	server.sin_port = (unsigned short) htons(service);
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return (-3);
	set_socket_options(s);
#ifdef PRIV_PORT
	if (geteuid() == 0)
	{
		struct	sockaddr_in	localaddr;
		int	portnum;

		localaddr = server;
		localaddr.sin_addr.s_addr = INADDR_ANY;
		for (portnum = 1023; portnum > PRIV_PORT; portnum--)
		{
			localaddr.sin_port = htons(portnum);
			if (bind(s, (struct sockaddr *) &localaddr,
					sizeof(localaddr)) != -1)
				break;
		}
	}
#endif /*PRIV_PORT*/
	if (connect(s, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		close(s);
		return (-4);
	}
	return (s);
}

/*
 * ircserv: This little program connects to the server (given as arg 1) on
 * the given port (given as arg 2).  It then accepts input from stdin and
 * sends it to that server. Likewise, it reads stuff sent from the server and
 * sends it to stdout.  Simple?  Yes, it is.  But wait!  There's more!  It
 * also intercepts server PINGs and automatically responds to them.   This
 * frees up the process that starts ircserv (such as IRCII) to pause without
 * fear of being pooted off the net. 
 *
 * Future enhancements: No-blocking io.  It will either discard or dynamically
 * buffer anything that would block. 
 */
int
main(argc, argv)
	int	argc;
	char	**argv;
{
	int	des;
	fd_set	rd;
	int	done = 0,
		c;
	char	*ptr,
		buffer[BUFSIZ + 1],
		pong[BUFSIZ + 1];
#ifdef NON_BLOCKING
	char	block_buffer[BUFSIZ + 1];
	fd_set	*wd_ptr = (fd_set *) 0,
		wd;
	int	wrote;
#endif /* NON_BLOCKING */

	if (argc < 3)
		exit(1);
#ifdef HAVE_SYS_UN_H
	if (*argv[1] == '/')
		des = connect_to_unix(argv[1]);
	else
#endif /* HAVE_SYS_UN_H */
		des = connect_by_number(atoi(argv[2]), argv[1]);
	if (des < 0)
		exit(des);
	fflush(stdout);

	(void) MY_SIGNAL(SIGTERM, SIG_IGN, 0);
	(void) MY_SIGNAL(SIGSEGV, SIG_IGN, 0);
	(void) MY_SIGNAL(SIGPIPE, SIG_IGN, 0);
#ifdef SIGWINCH
	(void) MY_SIGNAL(SIGWINCH, SIG_IGN, 0);
#endif
#ifdef SIGBUS
	(void) MY_SIGNAL(SIGBUS, SIG_IGN, 0);
#endif
#ifdef NON_BLOCKING
	if (fcntl(1, F_SETFL, FNDELAY))
		exit(1);
#endif /* NON_BLOCKING */
	while (!done)
	{
		fflush(stderr);
		FD_ZERO(&rd);
		FD_SET(0, &rd);
		FD_SET(des, &rd);
#ifdef NON_BLOCKING
		if (wd_ptr)
		{
			FD_ZERO(wd_ptr);
			FD_SET(1, wd_ptr);
		}
		switch (new_select(&rd, wd_ptr, NULL))
		{
#else
		switch (new_select(&rd, (fd_set *) 0, NULL))
		{
#endif /* NON_BLOCKING */
		case -1:
		case 0:
			break;
		default:
#ifdef NON_BLOCKING
			if (wd_ptr)
			{
				if (FD_ISSET(1, wd_ptr))
				{
					c = strlen(block_buffer);
					if ((wrote = write(1, block_buffer,
							c)) == -1)
					{
						wd_ptr = &wd;
					}
					else if (wrote < c)
					{
						strcpy(block_buffer,
							&(block_buffer[wrote]));
						wd_ptr = &wd;
					}
					else
						wd_ptr = (fd_set *) 0;
				}
			}
#endif /* NON_BLOCKING */
		if (FD_ISSET(0, &rd))
			{
				if (0 != (c = dgets(buffer, BUFSIZ, 0,
							(char *) 0)))
					if (write(des, buffer, c) == -1)
						perror("write");
				else
					done = 1;
			}
			if (FD_ISSET(des, &rd))
			{
				if (0 != (c = dgets(buffer, BUFSIZ, des,
							(char *) 0)))
				{
					if (strncmp(buffer, "PING ", 5) == 0)
					{
						if ((ptr = (char *) 
						    index(buffer, ' ')) != NULL)
						{
							sprintf(pong, "PONG user@host %s\n", ptr + 1);
							if (write(des, pong, strlen(pong)) == -1)
								perror("write");
						}
					}
					else
					{
#ifdef NON_BLOCKING
						if ((wrote = write(1, buffer,
								c)) == -1)
							wd_ptr = &wd;
						else if (wrote < c)
						{
							strcpy(block_buffer,
							    &(buffer[wrote]));
							wd_ptr = &wd;
						}
#else
						if (write(1, buffer, c) == -1)
							perror("write");
#endif /* NON_BLOCKING */
					}
				}
				else
					done = 1;
			}
		}
	}
	return 0;
}


#ifdef HAVE_SYS_UN_H
/*
 * Connect to a UNIX domain socket. Only works for servers.
 * submitted by Avalon for use with server 2.7.2 and beyond.
 */
int
connect_to_unix(path)
	char	*path;
{
	struct	sockaddr_un	un;
	int	sock;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);

	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, path);
	if (connect(sock, (struct sockaddr *)&un, strlen(path)+2) == -1)
	{
		close(sock);
		return -1;
	}
	return sock;
}
#endif /* HAVE_SYS_UN_H */
