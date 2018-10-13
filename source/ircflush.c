/*
 * Flush: A little program that tricks another program into line buffering
 * its output. 
 *
 * By Michael Sandrof 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: ircflush.c,v 1.2 2001/12/09 03:35:25 toast Exp $";
#endif

#include "irc.h"
#include <sys/wait.h>

#ifndef __linux__
# ifdef __svr4__
#  include <sys/termios.h>
# else
#  include <sgtty.h>	/* SVR4 => sgtty = yuk */
# endif /* SOLARIS */
#endif /* __linux__ */


#define BUFFER_SIZE 1024

/* descriptors of the tty and pty */
	int	master,
		slave;
	pid_t	pid;

RETSIGTYPE death();

RETSIGTYPE
death()
{
	close(0);
	close(master);
	kill(pid, SIGKILL);
	wait(0);
	exit(0);
}

/*
 * setup_master_slave: this searches for an open tty/pty pair, opening the
 * pty as the master device and the tty as the slace device 
 */
void
setup_master_slave()
{
	char	line[16];
	char	linec;
	int	linen;

	for (linec = 'p'; linec <= 's'; linec++)
	{
		sprintf(line, "/dev/pty%c0", linec);
		if (access(line, 0) != 0)
			break;
		for (linen = 0; linen < 16; linen++)
		{
			snprintf(line, sizeof(line), "/dev/pty%c%1x", linec, linen);
			if ((master = open(line, O_RDWR)) >= 0)
			{
				snprintf(line, sizeof(line), "/dev/tty%c%1x", linec, linen);
				if (access(line, R_OK | W_OK) == 0)
				{
					if ((slave = open(line, O_RDWR)) >= 0)
						return;
				}
				close(master);
			}
		}
	}
	fprintf(stderr, "flush: Can't find a pty\n");
	exit(0);
}

/*
 * What's the deal here?  Well, it's like this.  First we find an open
 * tty/pty pair.  Then we fork three processes.  The first reads from stdin
 * and sends the info to the master device.  The next process reads from the
 * master device and sends stuff to stdout.  The last processes is the rest
 * of the command line arguments exec'd.  By doing all this, the exec'd
 * process is fooled into flushing each line of output as it occurs.  
 */
int
main(argc, argv)
	int	argc;
	char	**argv;
{
	char	buffer[BUFFER_SIZE];
	int	cnt;
	fd_set	rd;

	if (argc < 2)
	{
	    fprintf(stderr, "Usage: %s [program] [arguments to program]\n", argv[0]);
	    exit(1);
	}
	pid = open("/dev/tty", O_RDWR);
#ifdef HAVE_SETSID
	setsid();
#else
	ioctl(pid, TIOCNOTTY, 0);
#endif /* HAVE_SETSID */
	setup_master_slave();
	switch (pid = fork())
	{
	case -1:
		fprintf(stderr, "flush: Unable to fork process!\n");
		exit(1);
	case 0:
		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);
		close(master);
		if (setuid(getuid()) == -1)
			perror("setuid");
		if (setgid(getgid()) == -1)
			perror("setgid");
		execvp(argv[1], &(argv[1]));
		fprintf(stderr, "flush: Error exec'ing process!\n");
		exit(1);
		break;
	default:
		(void) MY_SIGNAL(SIGCHLD, death, 0);
		close(slave);
		while (1)
		{
			FD_ZERO(&rd);
			FD_SET(master, &rd);
			FD_SET(0, &rd);
			switch (select(NFDBITS, &rd, 0, 0, 0))
			{
			case -1:
			case 0:
				break;
			default:
				if (FD_ISSET(0, &rd))
				{
				    if ((cnt = read(0, buffer,BUFFER_SIZE)) > 0)
					if (write(master, buffer, cnt) == -1)
						perror("write");
				    else
					death();
				}
				if (FD_ISSET(master, &rd))
				{
					if ((cnt = read(master, buffer,
							BUFFER_SIZE)) > 0)
					{
						if (write(1, buffer, cnt) == -1)
						{
							perror("write");
						}
					}
					else
					{
						death();
					}
				}
			}
		}
		break;
	}
	return 0;
}
