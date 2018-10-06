/* define if allow sys/time.h with time.h */
#undef TIME_WITH_SYS_TIME

/* define this if you are using BSD wait union thigs */
#undef BSDWAIT

/* define this if you are using -ltermcap */
#undef USING_TERMCAP

/* define this if you are using -lcurses */
#undef USING_CURSES

/* define this if you are using -lxtermcap */
#undef USING_XTERMCAP

/* define this if you are using -ltermlib */
#undef USING_TERMLIB

/* define this if signal's return void */
#undef SIGVOID

/* define this if you are using sigaction() instead of signal() */
#undef USE_SIGACTION

/* define this if you are using sigset() instead of signal() */
#undef USE_SIGSET

/* define this if you are using system V (unreliable) signals */
#undef SYSVSIGNALS

/* define this if wait3() is declared */
#undef WAIT3_DECLARED

/* define this if waitpid() is declared */
#undef WAITPID_DECLARED

/* define this if waitpid() is unavailable */
#undef NEED_WAITPID

/* define this if -lnls exists */
#undef HAVE_LIB_NLS

/* define this if -lnsl exists */
#undef HAVE_LIB_NSL

/* define his if -lPW exists */
#undef HAVE_LIB_PW

/* define this to the mail spool */
#undef MAIL_DIR

/* define this if you have scandir() */
#undef HAVE_SCANDIR

/* define this if you have memmove() */
#undef HAVE_MEMMOVE

/* define this if you have setsid() */
#undef HAVE_SETSID

/* define this if you have getsid() */
#undef HAVE_GETSID

/* define this if you have getpgid() */
#undef HAVE_GETPGID

/* define this if your getpgrp() doesn't take a pid argument */
#undef BROKEN_GETPGRP

/* define this if you have sys/select.h */
#undef HAVE_SYS_SELECT_H

/* define this if you have sys/fcntl.h */
#undef HAVE_SYS_FCNTL_H

/* define this if you have fcntl.h */
#undef HAVE_FCNTL_H

/* define this if you have sys/file.h */
#undef HAVE_SYS_FILE_H

/* define this if you have sys/time.h */
#undef HAVE_SYS_TIME_H

/* define this if you have sys/wait.h */
#undef HAVE_SYS_WAIT_H

/* define this if you have string.h */
#undef HAVE_STRING_H

/* define this if you have memory.h */
#undef HAVE_MEMORY_H

/* define this if you have netdb.h */
#undef HAVE_NETDB_H

/* define this if you have sys/ptem.h */
#undef HAVE_SYS_PTEM_H

/* define this if you need getcwd() */
#undef NEED_GETCWD 

/* define this if you have hpux version 7 */
#undef HPUX7

/* define this if you have hpux version 8 */
#undef HPUX8

/* define this if you have an unknown hpux version (pre ver 7) */
#undef HPUXUNKNOWN

/* define this if a pointer is 64 bits */
#undef LONGLONG_POINTER

/* define this if an unsigned long is 32 bits */
#undef UNSIGNED_LONG32

/* define this if an unsigned int is 32 bits */
#undef UNSIGNED_INT32

/* define this if you are unsure what is is 32 bits */
#undef UNKNOWN_32INT

/* define this if you are on a svr4 derivative */
#undef SVR4

/* define this if you are on solaris 2.x */
#undef __solaris__

/* define this if you don't have struct linger */
#undef NO_STRUCT_LINGER

/* define this if you are on svr3/twg */
#undef WINS

/* define this if you need fchmod */
#undef NEED_FCHMOD

/* define this to the location of normal unix mail */
#undef UNIX_MAIL

/* define this if your header files declare sys_errlist */
#undef SYS_ERRLIST_DECLARED

/* define this if you have uname(2) */
#undef HAVE_UNAME

/* define this if you need strerror(3) */
#undef NEED_STRERROR

/* define this if you need strlcpy(3) */
#undef NEED_STRLCPY

/* define this if you need strlcat(3) */
#undef NEED_STRLCAT

/* define this if you need strpbrk(3) */
#undef NEED_STRPBRK

/* define this if you have stdarg.h */
#undef HAVE_STDARG_H
