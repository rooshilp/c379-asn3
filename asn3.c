/*
 * Rooshil Patel
 * CMPUT 379
 * Assignment 3
 */

/*
 * This code is highly adapted from Bob Beck's
 * tanimate.c code which is adapted from
 * kent.edu
 *
 */

#include <stdio.h>
#include <curses.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAXSAU 15
#define MAXROC 10
#define MAXTHR 26
#define TUNIT 20000		/* timeunits in microseconds */


struct	object {
	char *str;	/* the message */
	int type;	/* 2 for turret, 1 for rocket, 0 for saucer */
	int alive; 	/* is alive? */
	int row;	/* the row     */
	int col;	/* the column */
	int delay;  /* delay in time units */
	int lives;	/* lives remaining */
};


int move_launcher(int i);
void *animate(void *arg);
int setup(struct object objects[]);
int draw(void *arg);
int fire();
int draw_rocket(void *arg);
int draw_saucer(void *arg);
int draw_launcher(void *arg);
int clear_rocket(void *arg, int len);
int clear_saucer(void *arg, int len);
int check_lost_status();
int print_stats();
int reset_saucer(void *arg);
int reset_rocket(void *arg);
int saucer_collision(int index);
int check_collision();
int rocket_collision(int index);
int end();

pthread_mutex_t launcher_move = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t saucer_move = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rocket_move = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t collision = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t draw_object = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t score_count = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t loss_count = PTHREAD_MUTEX_INITIALIZER;

struct object objects[MAXTHR];
int score = 0;
int losses = 0;
int rocket = 10;
pthread_t thrds[MAXTHR];	/* the threads		*/


int main(int ac, char *av[])
{
	int c;		/* user input		*/
	void *animate();	/* the function		*/
	int i;
	initscr();
	crmode();
	noecho();
	clear();
	setup(&objects);
	/* create all the threads */
	for(i=0 ; i<MAXTHR; i++)
		if ( pthread_create(&thrds[i], NULL, animate, &objects[i])){
			fprintf(stderr,"error creating thread");
			endwin();
			exit(0);
		}
	move_launcher('s');
	/* process user input */
	while(1) {
		c = getch();
		if ( c == 'Q' ) break;
		if ( c == ' ' )
			fire();
		if ( c == 'n' || c == 'm')
			move_launcher(c);	
	}

	/* cancel all the threads */
	end();
}

int end()
{
	int i;
	pthread_mutex_lock(&launcher_move);
	for (i=0; i<MAXTHR; i++ ) 
		pthread_cancel(thrds[i]);
	endwin();
	return 0;
}

int setup(struct object objects[])
{
	int i;
	srand(getpid());
	int endRocket = MAXROC + 1;
	/* assign rows and velocities to each string */

	for(i=0 ; i<MAXTHR; i++) {
		if (i == 0) {
			objects[i].str = "|";
			objects[i].type = 2;
			objects[i].alive = 1;
			objects[i].row = LINES-2;
			objects[i].col = 1;
			objects[i].delay = 1;
		}
		else if (i < endRocket) {
			objects[i].str = "^";
			objects[i].type = 1;
			objects[i].alive = 0;
			objects[i].row = 0;
			objects[i].col = 0;
			objects[i].delay = 5;
		}
		else {
			objects[i].str = "<--->";
			objects[i].type = 0;
			objects[i].alive = 0;
			objects[i].row = 1+(rand()%10);
			objects[i].col = 0;
			objects[i].delay = 1+(rand()%15);
			objects[i].lives = 1+(rand()%15);
		}
	}
	/* set up curses */

	for (i=0; i<COLS; i++) {
		mvprintw(LINES-2, i, "-");
	}

	return 0;
}

/* the code that runs in each thread */
void *animate(void *arg)
{
	struct object *info = arg;		/* point to info block	*/
	int len = strlen(info->str)+2;	/* +2 for padding	*/
	int col = rand()%(COLS-len-3);	/* space for padding	*/
	int i;
	while( 1 )
	{
		usleep(info->delay*TUNIT);
		check_collision();
		check_lost_status();
		if (info->alive == 1) {
			if (info->type == 2) {
				print_stats();
				pthread_mutex_lock(&launcher_move);
				draw_launcher(arg);
				pthread_mutex_unlock(&launcher_move);
			}
			else if (info->type == 1) {
				clear_rocket(arg, len);
				pthread_mutex_lock(&rocket_move);
				draw_rocket(arg);
				pthread_mutex_unlock(&rocket_move);
				if (info->row <= 0) {
					clear_rocket(arg, len);
					reset_rocket(arg);	
				}

			}
			else {
				pthread_mutex_lock(&saucer_move);
				draw_saucer(arg);
				info->col++;
				if (info->col+len >= COLS) {
					losses++;
					pthread_mutex_unlock(&saucer_move);
					clear_saucer(arg, len);
					reset_saucer(arg);
				}
				pthread_mutex_unlock(&saucer_move);
			}
		}

		else {
			if (info->type == 0) {
				clear_saucer(arg, len);
				info->alive=1;
				info->lives--;
			}
			else if (info->type == 1) {
				clear_rocket(arg, len);
			}
		}
	}
}

int fire() 
{
	int i;
	pthread_mutex_lock(&rocket_move);
	for (i=0; i<MAXROC; i++) {
		if (objects[i].alive == 0 && objects[i].type == 1) {
			objects[i].row = LINES-4;
			objects[i].col = objects[0].col;
			objects[i].alive = 1;
			rocket--;
			break;
		}
	}
	pthread_mutex_unlock(&rocket_move);
	return 0;
}

int print_stats()
{
	pthread_mutex_lock(&draw_object);
	mvprintw(LINES-1, 0,"Press 'Q' to quit. Score: '%d', Rockets: '%d', Lost: '%d'", score, rocket, losses);
	pthread_mutex_unlock(&draw_object);
	return 0;
}

int check_lost_status()
{
	if (rocket == 0) {
		end();
	}

	if (losses == 10) {
		end();
	}
	return 0;
}

int move_launcher(int i)
{
	pthread_mutex_lock(&launcher_move);
	objects[0].row = LINES - 3;
	if (i == 'n' && objects[0].col > 0) {
		objects[0].col--;
	}
	else if (i == 'm' && objects[0].col+3 < COLS) {
		objects[0].col++;
	}
	else if (i == 's') {
		objects[0].col = COLS/2;
	}
	pthread_mutex_unlock(&launcher_move);
	return 0;
}

int draw_launcher(void *arg)
{
	struct object *info = arg;
	pthread_mutex_lock(&draw_object);
	move( info->row, info->col );
	addch(' ');
	addstr( info->str );
	addch(' ');
	move(LINES-1,COLS-1);
	refresh();
	pthread_mutex_unlock(&draw_object);
	return 0;
}

int draw_saucer(void *arg)
{
	struct object *info = arg;
	pthread_mutex_lock(&draw_object);
	move( info->row, info->col );
	addch(' ');
	addstr( info->str );
	addch(' ');
	move(LINES-1,COLS-1);
	refresh();
	pthread_mutex_unlock(&draw_object);
	return 0;
}

int draw_rocket(void *arg)
{
	struct object *info = arg;
	pthread_mutex_lock(&draw_object);
	move( info->row, info->col );
	addch(' ');
	addstr( info->str );
	addch(' ');
	move(LINES-1,COLS-1);
	refresh();
	pthread_mutex_unlock(&draw_object);
	return 0;
}

int clear_saucer(void *arg, int len)
{
	struct object *info = arg;
	int i;
	pthread_mutex_lock(&draw_object);
	move(info->row, info->col);
	for (i=0; i<len; i++)
		addch(' ');
	move(LINES-1, COLS-1);
	refresh();
	pthread_mutex_unlock(&draw_object);
	return 0;	
}

int clear_rocket(void *arg, int len)
{
	struct object *info = arg;
	int i;
	pthread_mutex_lock(&draw_object);
	move(info->row, info->col);
	for (i=0; i<len; i++)
		addch(' ');
	move(LINES-1, COLS-1);
	refresh();
	info->row--;
	pthread_mutex_unlock(&draw_object);
	return 0;	
}

int reset_rocket(void *arg)
{
	struct object *info = arg;
	info->alive = 0;
	info->row = 0;
	info->col = 0;
	return 0;
}

int reset_saucer(void *arg)
{
	struct object *info = arg;
	info->alive = 0;
	info->row = 1+(rand()%10);
	info->col = 0;
	info->delay = 1+(rand()%15);
	return 0;
}

int check_collision()
{
	int i;
	int j;
	for (i=1; i<MAXROC+1; i++) {
		if (objects[i].alive == 1) {
			for (j=MAXROC+1; j<MAXTHR; j++) {
				if (objects[j].alive == 1) {
					if (objects[i].row == objects[j].row) {
						if ((objects[j].col <= objects[i].col) && (objects[j].col+5 >= objects[i].col)) {
							rocket_collision(i);
							saucer_collision(j);
							pthread_mutex_lock(&score_count);
							score++;
							pthread_mutex_unlock(&score_count);
							pthread_mutex_lock(&rocket_move);
							rocket++;
							pthread_mutex_unlock(&rocket_move);

							return 0;
						}
					}
				}
			}
		}
	}
	return 0;
}

int rocket_collision(int index)
{
	clear_rocket(&objects[index], strlen(objects[index].str)+2);
	reset_rocket(&objects[index]);
	return 0;
}

int saucer_collision(int index)
{
	clear_saucer(&objects[index], strlen(objects[index].str)+2);
	reset_saucer(&objects[index]);
	return 0;
}


