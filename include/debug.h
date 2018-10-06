/*
 * debug.h - header file for phone's debug routine..
 *
 * Copyright (C) 1993, Matthew Green.
 *
 * @(#)$Id: debug.h,v 1.1.1.1 1999/07/16 21:21:13 toast Exp $
 */

#ifndef _DEBUG_H_
# define _DEBUG_H_ 

#ifdef DEBUG
# define Debug(x) debug x

# ifdef USE_STDARG_H
extern  void    debug( int, char *, ... );
# else
extern        void    debug();
# endif

extern	int	debuglevel;
extern	int	setdlevel();

# else
# define Debug(x)
# endif

#endif /* _DEBUG_H_ */
