/*
 * #     #
 * #    #   #    #  #   ##  #    #   # 
 *		         #
 *
 * Exploring the dungeons of doom
 * Copyright (C) 1981 by Michael Toy, Ken Arnold, and Glenn Wichman
 * All rights reserved
 *
 * @(#)main.c	4.26 (Berkeley) 2/4/82
 */

#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pwd.h>
#include <unistd.h>
#include <wait.h>
#include <time.h>
#include "rogue.h"

/*
 * main:
 *	The main program, of course
 */
int
main(argc, argv, envp)
int argc;
char **argv;
char **envp;
{
    register char *env;
    register struct passwd *pw;
    struct passwd *getpwuid();
    char *getpass();
    int lowtime;

#ifdef __DJGPP__
	_fmode = O_BINARY;
#endif

#ifndef DUMP
    signal(SIGQUIT, exit);
    signal(SIGILL, exit);
    signal(SIGTRAP, exit);
#ifdef SIGIOT
    signal(SIGIOT, exit);
#endif
#ifdef SIGEMT
    signal(SIGEMT, exit);
#endif
    signal(SIGFPE, exit);
#ifdef SIGBUS
    signal(SIGBUS, exit);
#endif
    signal(SIGSEGV, exit);
#ifdef SIGSYS
    signal(SIGSYS, exit);
#endif
#endif

#ifdef WIZARD
    /*
     * Check to see if he is a wizard
     */
    if (argc >= 2 && argv[1][0] == '\0')
	if (strcmp(PASSWD, xcrypt(getpass("Wizard's password: "), "mT")) == 0)
	{
	    wizard = TRUE;
	    player.t_flags |= SEEMONST;
	    argv++;
	    argc--;
	}
#endif

    /*
     * get home and options from environment
     */
    if ((env = getenv("HOME")) != NULL)
	strcpy(home, env);
    else if ((pw = getpwuid(getuid())) != NULL)
	strcpy(home, pw->pw_dir);
    else
	home[0] = '\0';
    strcat(home, "/");

    strcpy(file_name, home);
    strcat(file_name, "rogue.save");

    if ((env = getenv("ROGUEOPTS")) != NULL)
	parse_opts(env);
    if (env == NULL || whoami[0] == '\0')
	if ((pw = getpwuid(getuid())) == NULL)
	{
	    printf("Say, who the hell are you?\n");
	    exit(1);
	}
	else
	    strucpy(whoami, pw->pw_name, strlen(pw->pw_name));
    if (env == NULL || fruit[0] == '\0')
	strcpy(fruit, "slime-mold");

    /*
     * check for print-score option
     */
    open_score();
    if (argc == 2 && strcmp(argv[1], "-s") == 0)
    {
	noscore = TRUE;
	score(0, -1, 0);
	exit(0);
    }
    init_check();			/* check for legal startup */
    if (argc == 2)
	if (!restore(argv[1], envp))	/* Note: restore will never return */
	    exit(1);
    lowtime = (int) time(NULL);
#ifdef WIZARD
    if (wizard && getenv("SEED") != NULL)
	dnum = atoi(getenv("SEED"));
    else
#endif
	dnum = lowtime + getpid();
#ifdef WIZARD
    if (wizard)
	printf("Hello %s, welcome to dungeon #%d", whoami, dnum);
    else
#endif
	printf("Hello %s, just a moment while I dig the dungeon...", whoami);
    fflush(stdout);
    seed = dnum;

    init_player();			/* Set up initial player stats */
    init_things();			/* Set up probabilities of things */
    init_names();			/* Set up names of scrolls */
    init_colors();			/* Set up colors of potions */
    init_stones();			/* Set up stone settings of rings */
    init_materials();			/* Set up materials of wands */
    initscr();				/* Start up cursor package */

    if (COLS < 76)
    {
        endwin();
        printf("\n\nSorry, %s, but your terminal window has too few columns.\n", whoami);
        printf("Your terminal has %d columns, needs 76.\n",COLS);
        exit(1);
    }

    if (LINES < 24)
    {
        endwin();
        printf("\n\nSorry, %s, but your terminal window has too few lines.\n", whoami);
        printf("Your terminal has %d lines, needs 24.\n",LINES);
        exit(1);
    }

    setup();
    /*
     * Set up windows
     */
    hw = newwin(LINES, COLS, 0, 0);
#ifdef WIZARD
    noscore = wizard;
#endif
    new_level();			/* Draw current level */
    /*
     * Start up daemons and fuses
     */
    start_daemon(doctor, 0, AFTER);
    fuse(swander, 0, WANDERTIME, AFTER);
    start_daemon(stomach, 0, AFTER);
    start_daemon(runners, 0, AFTER);
    playit();
}

/*
 * endit:
 *	Exit the program abnormally.
 */
void
endit(sig)
int sig;
{
    fatal("Ok, if you want to exit that badly, I'll have to allow it\n");
}

/*
 * fatal:
 *	Exit the program, printing a message.
 */
void
fatal(s)
char *s;
{
    clear();
    move(LINES-2, 0);
    printw("%s", s);
    refresh();
    endwin();
    exit(0);
}

/*
 * rnd:
 *	Pick a very random number.
 */
int
rnd(range)
register int range;
{
    return range == 0 ? 0 : abs((int) RN) % range;
}

/*
 * roll:
 *	Roll a number of dice
 */
int
roll(number, sides)
register int number, sides;
{
    register int dtotal = 0;

    while (number--)
	dtotal += rnd(sides)+1;
    return dtotal;
}
#ifdef SIGTSTP
/*
 * tstp:
 *	Handle stop and start signals
 */
void
tstp(sig)
int sig;
{
    register int y, x;
    register int oy, ox;

    getyx(curscr, oy, ox);
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
    fflush(stdout);
    kill(0, SIGTSTP);
    signal(SIGTSTP, tstp);
    crmode();
    noecho();
    clearok(curscr, TRUE);
    wrefresh(curscr);
    getyx(curscr, y, x);
    mvcur(y, x, oy, ox);
    fflush(stdout);
    curscr->_cury = oy;
    curscr->_curx = ox;
}
#endif

/*
 * playit:
 *	The main loop of the program.  Loop until the game is over,
 *	refreshing things and looking at the proper times.
 */
void
playit()
{
    register char *opts;

    /*
     * set up defaults for slow terminals
     */

#if !defined(_XOPEN_CURSES) && !defined(__NCURSES_H)
    if (_tty.sg_ospeed <= B1200)
#else
    if (baudrate() <= 1200)
#endif
    {
	terse = TRUE;
	jump = TRUE;
    }

    /*
     * parse environment declaration of options
     */
    if ((opts = getenv("ROGUEOPTS")) != NULL)
	parse_opts(opts);


    oldpos = hero;
    oldrp = roomin(&hero);
    while (playing)
	command();			/* Command execution */
    endit(0);
}

/*
 * quit:
 *	Have player make certain, then exit.
 */
void
quit(int sig)
{
    register int oy, ox;

    /*
     * Reset the signal in case we got here via an interrupt
     */
    if (signal(SIGINT, quit) != quit)
	mpos = 0;
    getyx(curscr, oy, ox);
    msg("really quit?");
    if (readchar() == 'y')
    {
	signal(SIGINT, leave);
	clear();
	mvprintw(LINES - 2, 0, "You quit with %d gold pieces", purse);
	move(LINES - 1, 0);
	refresh();
	score(purse, 1, 0);
	exit(0);
    }
    else
    {
	move(0, 0);
	clrtoeol();
	status();
	move(oy, ox);
	refresh();
	mpos = 0;
	count = 0;
    }
}

/*
 * leave:
 *	Leave quickly, but curteously
 */
void
leave(int sig)
{
#if !defined(_XOPEN_CURSES) && !defined(__NCURSES_H)
    if (!_endwin)
#else
    if (1)
#endif
    {
	mvcur(0, COLS - 1, LINES - 1, 0);
	endwin();
    }
    putchar('\n');
    exit(0);
}

/*
 * shell:
 *	Let him escape for a while
 */
void
shell()
{
    register int pid;
    register char *sh;
    int ret_status;

    /*
     * Set the terminal back to original mode
     */
    move(LINES-1, 0);
    refresh();
    endwin();
    putchar('\n');
    in_shell = TRUE;
    after = FALSE;
    sh = getenv("SHELL");
    fflush(stdout);
    /*
     * Fork and do a shell
     */
    while ((pid = fork()) < 0)
	sleep(1);
    if (pid == 0)
    {
	execl(sh == NULL ? "/bin/sh" : sh, "shell", "-i", NULL);
	perror("No shelly");
	exit(-1);
    }
    else
    {
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while (wait(&ret_status) != pid)
	    continue;
	signal(SIGINT, quit);
	signal(SIGQUIT, endit);
	printf("\n[Press return to continue]");
	noecho();
	crmode();
	in_shell = FALSE;
	wait_for('\n');
	clearok(stdscr, TRUE);
    }
}
