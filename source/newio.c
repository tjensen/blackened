/*
 * newio.c: This is some handy stuff to deal with file descriptors in a way
 * much like stdio's FILE pointers 
 *
 * IMPORTANT NOTE:  If you use the routines here-in, you shouldn't switch to
 * using normal reads() on the descriptors cause that will cause bad things
 * to happen.  If using any of these routines, use them all 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: newio.c,v 1.1.1.1 1999/07/16 21:21:43 toast Exp $";
#endif

#include "irc.h"
#include "ircaux.h"

#ifdef ISC22
# include <sys/bsdtypes.h>
#endif /* ISC22 */

#ifdef ESIX
# include <lan/net_types.h>
#endif /* ESIX */

#include "irc_std.h"

#define IO_BUFFER_SIZE 512

#define	WAIT_NL ((unsigned) 0x0001)

#ifdef FDSETSIZE
# define IO_ARRAYLEN FDSETSIZE
#else
# define IO_ARRAYLEN NFDBITS
#endif

typedef	struct	myio_struct
{
	char	buffer[IO_BUFFER_SIZE + 1];
	unsigned int	read_pos,
			write_pos;
	unsigned	misc_flags;
#ifdef ESIX
	unsigned	flags;
#endif /* ESIX */
}           MyIO;

#define IO_SOCKET 1

static	struct	timeval	right_away = { 0L, 0L };
static	MyIO	*io_rec[IO_ARRAYLEN];

static	struct	timeval	dgets_timer;
static	struct	timeval	*timer;
	int	dgets_errno = 0;

#ifdef ESIX
/* Esix must know if it is a socket or not. */
void	mark_socket(int des)
{

	if (first)
	{
		int	c;
		for (c = 0; c < FD_SETSIZE; c++)
			io_rec[c] = (MyIO *) 0;
		first = 0;
	}
	if (io_rec[des] == (MyIO *) 0)
	{
		io_rec[des] = (MyIO *) new_malloc(sizeof(MyIO));
		io_rec[des]->read_pos = 0;
		io_rec[des]->write_pos = 0;
		io_rec[des]->flags = 0;
	}
	io_rec[des]->flags |= IO_SOCKET;
}

void	unmark_socket(int des)
{

	if (first)
	{
		int	c;
		for (c = 0; c < FD_SETSIZE; c++)
			io_rec[c] = (MyIO *) 0;
		first = 0;
	}
	if (io_rec[des] == (MyIO *) 0)
	{
		io_rec[des] = (MyIO *) new_malloc(sizeof(MyIO));
		io_rec[des]->read_pos = 0;
		io_rec[des]->write_pos = 0;
		io_rec[des]->flags = 0;
	}
	io_rec[des]->flags &= ~IO_SOCKET;
}
#endif /* ESIX */

/*
 * dgets_timeout: does what you'd expect.  Sets a timeout in seconds for
 * dgets to read a line.  if second is -1, then make it a poll.
 */
extern	time_t
dgets_timeout(sec)
	int	sec;
{
	time_t	old_timeout = dgets_timer.tv_sec;

	if (sec)
	{
		dgets_timer.tv_sec = (sec == -1) ? 0 : sec;
		dgets_timer.tv_usec = 0;
		timer = &dgets_timer;
	}
	else
		timer = (struct timeval *) 0;
	return old_timeout;
}

static	void	init_io()
{
	static	int	first = 1;

	if (first)
	{
		int	c;

		for (c = 0; c < IO_ARRAYLEN; c++)
			io_rec[c] = (MyIO *) 0;
		(void) dgets_timeout(-1);
		first = 0;
	}
}

/*
 * dgets: works much like fgets except on descriptor rather than file
 * pointers.  Returns the number of character read in.  Returns 0 on EOF and
 * -1 on a timeout (see dgets_timeout()) 
 */
int	dgets(str, len, des, specials)
char	*str;
int	len;
int	des;
char	*specials;
{
	char	*ptr, ch;
	int	cnt = 0,
		c;
	fd_set	rd;
	int	WantNewLine = 0;
	int	BufferEmpty;
	int	i,
		j;

	init_io();
	if (io_rec[des] == (MyIO *) 0)
	{
		io_rec[des] = (MyIO *) new_malloc(sizeof(MyIO));
		io_rec[des]->read_pos = 0;
		io_rec[des]->write_pos = 0;
		io_rec[des]->misc_flags = 0;
#ifdef ESIX
		io_rec[des]->flags = 0;
#endif /* ESIX */	
	}
	if (len < 0)
	{
		WantNewLine = 1;
		len = (-len);
		io_rec[des]->misc_flags |= WAIT_NL;
	}
	while (1)
	{
		if ((BufferEmpty = (io_rec[des]->read_pos ==
				io_rec[des]->write_pos)) || WantNewLine)
		{
			if(BufferEmpty)
			{
				io_rec[des]->read_pos = 0;
				io_rec[des]->write_pos = 0;
			}
			FD_ZERO(&rd);
			FD_SET(des, &rd);
			switch (select(des + 1, &rd, 0, 0, timer))
			{
			case 0:
				str[cnt] = (char) 0;
				dgets_errno = 0;
				return (-1);
			default:
#ifdef ESIX
				if (io_rec[des]->flags & IO_SOCKET)
					c = recv(des, io_rec[des]->buffer +
					  io_rec[des]->write_pos,
					  IO_BUFFER_SIZE-io_rec[des]->write_pos,
					  0);
				else
#endif /* ESIX */
					c = read(des, io_rec[des]->buffer +
					 io_rec[des]->write_pos,
					 IO_BUFFER_SIZE-io_rec[des]->write_pos);
				if (c <= 0)
				{
					if (c == 0)
						dgets_errno = -1;
					else
						dgets_errno = errno;
					return 0;
				}
				if (WantNewLine && specials)
				{
					ptr = io_rec[des]->buffer;
					for (i = io_rec[des]->write_pos;
					    i < io_rec[des]->write_pos+c;i++)
	/* This section re-indented - phone, jan 1993 */
			{
				if((ch = ptr[i]) == specials[0])
				{
					if (i > 0)
					{
						bcopy(ptr + i - 1, ptr + i + 1,
						    io_rec[des]->write_pos +
						    c - i - 1);
						i -= 2;
						c -= 2;
					}
					else
					{
						bcopy(ptr, ptr + 1,
						    io_rec[des]->write_pos +
						    c - 1);
						i--;
						c--;
					}
				}
				else if (ch == specials[2])
				{
					for (j = i - 1; j >= 0 &&
							isspace(ptr[j]); j--)
						;
					for (;j >= 0 && !isspace(ptr[j]); j--)
						;
					bcopy(ptr + j + 1, ptr + i + 1,
					    io_rec[des]->write_pos + c - i - 1);
					c -= (i - j);
					i = j;
				}
				else if (ch == specials[1])
				{
					for (j = i - 1;
						j >= 0 && ptr[j] != '\n'; j--);
					bcopy(ptr + j + 1, ptr + i + 1,
					    io_rec[des]->write_pos + c - i - 1);
					c -= (i-j);
					i = j;
				}
			}
				}
				io_rec[des]->write_pos += c;
				break;
			}
		}
		ptr = io_rec[des]->buffer;
		if (WantNewLine)
		{
			for (cnt = io_rec[des]->write_pos; cnt > 0;cnt--,ptr++)
			{
				if (*ptr == '\n' || cnt == len-1)
				{
					*ptr = '\0';
					(void) strcpy(str, io_rec[des]->buffer);
					io_rec[des]->write_pos=cnt-1;
					bcopy(io_rec[des]->buffer, ptr, cnt);
					dgets_errno = 0;
					return 1;
				}
			}
			return -2;
		}
		while (io_rec[des]->read_pos < io_rec[des]->write_pos)
		{
			if (((str[cnt++] = ptr[(io_rec[des]->read_pos)++])
				== '\n') || (cnt == len))
			{
				dgets_errno = 0;
				str[cnt] = (char) 0;
				return (cnt);
			}
		}
	}
}

/*
 * new_select: works just like select(), execpt I trimmed out the excess
 * parameters I didn't need.  
 */
int	new_select(rd, wd, timeout)
fd_set	*rd,
	*wd;
struct	timeval	*timeout;
{
	int	i,
		set = 0;
		fd_set new;
	struct	timeval	*newtimeout,
			thetimeout;
	int	max_fd = -1;

	if (timeout)
	{
		newtimeout = &thetimeout;
		bcopy(timeout, newtimeout, sizeof(struct timeval));
	}
	else
		newtimeout = NULL;
	init_io();
	FD_ZERO(&new);
	for (i = 0; i < IO_ARRAYLEN; i++)
	{
		if (i > max_fd && (rd && FD_ISSET(i, rd) || wd && FD_ISSET(i, wd)))
			max_fd = i;
		if (io_rec[i] && !(io_rec[i]->misc_flags&WAIT_NL))
		{
			if (io_rec[i]->read_pos < io_rec[i]->write_pos)
			{
				FD_SET(i, &new);
				set = 1;
			}
		}
	}
	if (set)
	{
		set = 0;
		if (!(select(max_fd + 1, rd, wd, NULL, &right_away) > 0))
			FD_ZERO(rd);
		for (i = 0; i < IO_ARRAYLEN; i++)
		{
			if ((FD_ISSET(i, rd)) || (FD_ISSET(i, &new)))
			{
				set++;
				FD_SET(i, rd);
			}
			else
				FD_CLR(i, rd);
		}
		return (set);
	}
	return (select(max_fd + 1, rd, wd, NULL, newtimeout));
}

/* new_close: works just like close */
void	new_close(des)
int	des;
{
#ifdef ESIX
	if (io_rec[des]->flags & IO_SOCKET)
		t_close(des);
#endif /* ESIX */
	new_free(&(io_rec[des]));
	close(des);
}

/* set's socket options */
extern	void
set_socket_options(s)
	int	s;
{
#ifdef	ESIX
	mark_socket(Client->read);
#else
#ifndef NO_STRUCT_LINGER
	struct linger	lin;
#endif
	int	opt = 1;
	int	optlen = sizeof(opt);

	(void) setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, optlen);
	opt = 1;
	(void) setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, optlen);
#ifndef NO_STRUCT_LINGER
	lin.l_onoff = lin.l_linger = 0;
	(void) setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&lin, optlen);
#endif /* NO_STRUCT_LINGER */
#endif /* ESIX */
}
