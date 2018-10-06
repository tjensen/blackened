/* To use this test program, compile it with "cc -otest-suid test-suid.c",
 * change its ownership to root and mode 4711, and run it as a
 * mortal user. IF the results show that it was able to set the
 * EUID back to 0, you can use the PRIV_PORT_ULC version of the
 * privileged port code, which is trivially proven to be secure.
 */
#include <stdio.h>

main()
{

	printf("Starting EUID = %d\n", geteuid());
	seteuid(getuid());
	printf("After seteuid to %d is %d\n", getuid(), geteuid());
	seteuid(0);
	printf("After seteuid to 0 is %d\n", geteuid());
}
