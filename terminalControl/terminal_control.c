#include <stdio.h>
#include "terminal_control.h"
#include "color.h"
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void color_print(char *msg, color_t fg_color, color_t bg_color);

typedef struct {
	uint64_t **map;
	int size;
} map_t;

void map_init(map_t *game, int size);
void map_choose(map_t *game, int type);
void map_draw(map_t game);


int main(void){

	echo_off();
	clear_terminal();

	//set_raw_input_mode();

	map_t new_game;

	map_choose(&new_game, 2);
	map_draw(new_game);

	//set_cooked_input_mode();
	echo_on();
	return 0;
}

void color_print(char *msg, color_t fg_color, color_t bg_color){

	printf("\x1B[38;2;%d;%d;%dm\x1B[48;2;%d;%d;%dm%s\x1b[0m",
		fg_color.r, fg_color.g, fg_color.b, 
		bg_color.r, bg_color.g, bg_color.b, msg);
}

void clear_terminal(){
	write(STDOUT_FILENO, "\x1b[H\x1b[2J\x1b[3J", 11);
}

void get_cols_rows(int *cols, int *rows){

	char bash_cmd[256] = "tput cols lines";
	FILE *pipe;
	int len; 

	pipe = popen(bash_cmd, "r");

	if (NULL == pipe) {
		perror("pipe");
		exit(1);
	} 
	
	fscanf(pipe, "%d", cols);
	fscanf(pipe, "%d", rows);

	pclose(pipe);
}

void echo_off(){

	system("stty -echo");
	printf("\e[?25l");
}
void echo_on(){

	system("stty echo");
	printf("\e[?25h");
}

void set_cursor(int x, int y){
	printf("\033[%d;%dH", y, x);
}

void set_raw_input_mode(){
	system("/bin/stty raw");
}
void set_cooked_input_mode(){
	system("/bin/stty cooked");
}

void map_init(map_t *game, int size){

	game->map = (uint64_t**)malloc(sizeof(*(game->map)) * size);
	
	for(int i = 0; i < size; i++){
		game->map[i] = (uint64_t*)malloc(sizeof(**(game->map)) * size);
	}

	game->size = size;
}

void map_choose(map_t *game, int type){

	uint64_t map_preset1[16][16] = {
		{0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0},
		{0,0,0,1,0,1,0,1,1,1,0,1,0,1,0,0},
		{0,0,0,1,0,0,0,1,0,1,0,0,0,1,0,0},
		{0,0,0,1,0,0,1,0,0,0,1,0,0,1,0,0},
		{0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0},
		{0,0,1,0,0,1,1,0,0,0,1,1,0,0,1,0},
		{0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
		{0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1},
		{0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
		{0,1,0,0,0,0,0,1,1,1,0,0,0,0,0,1},
		{0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
		{0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
		{0,1,0,0,0,1,0,0,1,0,0,1,0,0,0,1},
		{0,0,1,0,0,0,1,1,0,1,1,0,0,0,1,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0}
	};

	switch (type)
	{
	case 1:
		game->size = 16;
		map_init(game, game->size);
		for(int i = 0; i < game->size; i++){
			for(int j = 0; j < game->size; j++){
				game->map[i][j] = map_preset1[i][j];
			}
		}
		break;
	case 2:
		game->size = 64;
		map_init(game, game->size);
		for(int i = 0; i < game->size; i++){
			for(int j = 0; j < game->size; j++){
				game->map[i][j] = rand() % 2;
			}
		}
	default:
		break;
	}
}

void map_draw(map_t game){
	
	color_t color;
	char *msg = malloc(sizeof(char) * 16);

	for(int i = 0; i < game.size; i++){
		for(int j = 0; j < game.size; j++){
			color = (color_t)COLOR_BLACK;
			if(game.map[i][j] % 2 != 0) {
				color = COLOR_WHITE;

			}
			else if (game.map[i][j] & (1 << 1)){
				color = COLOR_BLACK;
			}
			sprintf(msg, "%d ", i);
			color_print(msg, COLOR_RED, color);
		}
		printf("\n");
	}
}