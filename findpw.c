/*
 * print out an encrypted password on the standard output
 *
 * @(#)findpw.c	1.1 (Berkeley) 12/20/81
 */
#include <stdio.h>

main()
{
    char buf[80];

    fprintf(stderr, "Password: ");
    fgets(buf, 80, stdin);
    buf[strlen(buf) - 1] = '\0';
    printf("%s\n", crypt(buf, "mT"));
}
