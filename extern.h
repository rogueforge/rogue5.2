/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h	4.3 (Berkeley) 4/2/82
 */

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 */

#define MAXSTR		80	/* maximum length of strings */
#define MAXLINES	32	/* maximum number of screen lines used */
#define MAXCOLS		80	/* maximum number of screen columns used */

#define RN		(((seed = seed*11109+13849) >> 16) & 0xffff)

/*
 * Now all the global variables
 */

extern bool	after, amulet, askme, door_stop, fight_flush,
		firstmove, in_shell, jump, noscore, p_know[], passgo,
		playing, r_know[], running, s_know[], save_msg,
		slow_invent, terse, wizard, ws_know[];

extern char	_flags[], _level[], *a_names[], file_name[], fruit[],
		home[], huh[], outbuf[], *p_colors[], *p_guess[],
		prbuf[], *r_guess[], *r_stones[], *release, runch,
		*s_guess[], *s_names[], take, *w_names[], whoami[],
		*ws_guess[], *ws_made[], *ws_type[];

extern int	a_chances[], a_class[], count, dnum, food_left,
		fung_hit, fd, group, hungry_state, inpack, lastscore,
		level, max_level, mpos, no_command, no_food, no_move,
		ntraps, purse, quiet, total;

extern long	seed;

extern WINDOW	*hw;

/*
 * Function types
 */

char	*brk(), *charge_str(), *ctime(), *getenv(), *inv_name(),
	*killname(), *malloc(), *nothing(), *num(), *ring_num(),
	*sbrk(), *sprintf(), *strcat(), *strcpy(), *tr_name(),
	*unctrl(), *vowelstr();

int	auto_save(), doctor(), endit(), leave(), nohaste(), quit(),
	rollwand(), runners(), sight(), stomach(), swander(), tstp(),
	turn_see(), unconfuse(), unsee();

#ifdef CHECKTIME
int	checkout();
#endif

long	lseek(), time();
