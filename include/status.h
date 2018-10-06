/*
 * status.h: header for status.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: status.h,v 1.1.1.1 1999/07/16 21:21:14 toast Exp $
 */

#ifndef _STATUS_H_
#define _STATUS_H_

extern	void	make_status();
extern	void	refresh_status();
extern	void	set_alarm();
extern	char	*update_clock();
extern	void	reset_clock();
extern	void	build_status();
extern	void	status_update();

#define GET_TIME 1
#define RESET_TIME 2

#endif /* _STATUS_H_ */
