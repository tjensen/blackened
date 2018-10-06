/*
 * ircsig.c: has a `my_signal()' that uses sigaction().
 *
 * written by matthew green, 1993.
 *
 * i stole bits of this from w. richard stevens' `advanced programming
 * in the unix environment' -mrg
 */

#ifndef lint
static	char	rcsid[] = "@(#)$Id: ircsig.c,v 1.1.1.1 1999/07/16 21:21:43 toast Exp $";
#endif

#include "irc.h"
#include "irc_std.h"

#ifdef USE_SIGACTION
sigfunc *
my_signal(sig_no, sig_handler, misc_flags)
	int sig_no;
	sigfunc *sig_handler;
	int misc_flags;
{
        /*
         * misc_flags is unused currently.  it's planned to be used
         * to use some of the doovier bits of sigaction(), if at
         * some point we need them, -mrg
         */

        struct sigaction sa, osa;

        sa.sa_handler = sig_handler;

        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, sig_no);

        /* this is ugly, but the `correct' way.  i hate c. -mrg */
        sa.sa_flags = 0;
#if defined(SA_RESTART) || defined(SA_INTERRUPT)
        if (SIGALRM == sig_no)
        {
# if defined(SA_INTERRUPT)
                sa.sa_flags |= SA_INTERRUPT;
# endif /* SA_INTERRUPT */
        }
        else
        {
# if defined(SA_RESTART)
                sa.sa_flags |= SA_RESTART;
# endif /* SA_RESTART */
        }
#endif /* SA_RESTART || SA_INTERRUPT */

        if (0 > sigaction(sig_no, &sa, &osa))
                return (SIG_ERR);

        return (osa.sa_handler);
}
#endif /* USE_SIGACTION */
