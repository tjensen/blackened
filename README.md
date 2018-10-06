# Blackened

Note: This project is no longer being actively maintained!

## What is it?

Blackened is an IRC client derived from
[ircii-2.8.2](http://www.irchelp.org/clients/unix/ircii/). More precisely, it
is a fork of a fork of ircii by
[Chris "Comstud" Behrens](https://github.com/comstud).

## History

In the late 1990s, I was encouraged by my friend,
[Matthew "LarZ" Ramsey](https://www.linkedin.com/in/matthewjramsey/), to help
him shrink his `.ircrc` file as much as possible by taking Comstud's fork of
ircii and enhancing it with more built-in commands and other functionality.

After I graduated from university and started my first full-time job, I wasn't
able to devote the time and energy to continue  maintaining the project and it
was eventually abandoned, sometime after I released version 1.8.1.

Unfortunately, I hosted my own CVS repository for keeping the Blackened source
code instead of putting it up on a free service, like Sourceforge. When the
server running the CVS repository went down sometime during the 2000s, the
source code history became lost to the ether. Luckily for me, several open
source mirrors still have the version 1.8.1 release tarball available for
download, so I have uploaded that snapshot of the code, here.

## What now?

I have no plans to resume maintaining Blackened. I put the code here because I
think it's important to remember and learn from our past.

To my surprise, the code still compiles successfully (albeit with warnings)
with GCC 7 on modern Linux in 2018, but the executable segfaults during
startup. I might try to fix the segfault, someday, but that's probably it.
