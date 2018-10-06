/*
 * flood.h: header file for flood.c
 *
 * @(#)$Id: flood.h,v 1.1.1.1 1999/07/16 21:21:13 toast Exp $
 */

#ifndef _FLOOD_H_
#define _FLOOD_H_

extern	int	check_flooding();

#define MSG_FLOOD 0
#define PUBLIC_FLOOD 1
#define NOTICE_FLOOD 2
#define WALL_FLOOD 3
#define WALLOP_FLOOD 4
#define CTCP_FLOOD 5
#define INVITE_FLOOD 6
#define NICK_FLOOD 7
#define NUMBER_OF_FLOODS 8

#endif /* _FLOOD_H_ */
