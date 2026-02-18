#include <stdio.h>
#include <stdint.h>
#include "game.h"

void move_player(map_t *game, int x, int y, int prev_x, int prev_y){
	game->map[prev_x][prev_y] = (1 << 1);
	game->map[x][y] = (1 << 1) | (cell_t)(cell_t)1 << (8 * sizeof(cell_t) - 1);
}

void init_map(map_t *game, int size){

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
		init_map(game, game->size);
		for(int i = 0; i < game->size; i++){
			for(int j = 0; j < game->size; j++){
				game->map[i][j] = map_preset1[i][j];
			}
		}
		break;
	case 2:
		game->size = 64;
		init_map(game, game->size);
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
	
	color_t wall_color = COLOR_WHITE;
	color_t empty_cell_color = COLOR_BLACK;

	for(int i = 0; i < game.size; i++){
		set_cursor(0, i+1);
		for(int j = 0; j < game.size; j++){
			color = (color_t)COLOR_BLACK;
			if(is_wall(game.map[i][j])) {
				color = wall_color;

			}
			else if(is_empty(game.map[i][j])){
				color = empty_cell_color;
			}
			else{
				color = get_cell_color(game.map[i][j]);
			}
			
			color_print("  ", color, color);
		}
		printf("\n");
	}
}

int is_wall(cell_t cell){
	return cell % 2;
}

int is_empty(cell_t cell){
	return (cell == 0);
}

color_t get_cell_color(cell_t cell){
	//0000 0000 0000 0000 0000 0001 1111 1110
	uint64_t player = cell & 0x000001FE;
	int is_current_position = 1 && (cell & (cell_t)(cell_t)1 << (8 * sizeof(cell_t) - 1));
	switch (player)
	{
		case 1 << 1:
			return darken_color(COLOR_RED, 25 * is_current_position);
		case 1 << 2:
			return darken_color(COLOR_GREEN, 25 * is_current_position);
		case 1 << 3:
			return darken_color(COLOR_BLUE, 25 * is_current_position);
		case 1 << 4:
			return darken_color(COLOR_MAGENTA, 25 * is_current_position);
		case 1 << 5:
			return darken_color(COLOR_YELLOW, 25 * is_current_position);
		case 1 << 6:
			return darken_color(COLOR_CYAN, 25 * is_current_position);
		case 1 << 7:
			return COLOR_ORANGE;
		case 1 << 8:
			return COLOR_LIME;
		case 1 << 9:
			return COLOR_LIGHT_BLUE;
		case 1 << 10:
			return COLOR_MAGENTA;
		case 1 << 11:
			return COLOR_YELLOW;
		case 1 << 12:
			return COLOR_CYAN;
		default:
			return COLOR_BLACK;
	}
}

void init_player(map_t *game, int *x, int *y){

	srand(time(NULL));

	do{
		*x = rand() % game->size;
		*y = rand() % game->size;
	} while(is_wall(game->map[*x][*y]));

	game->map[*x][*y] = (1 << 1) | (cell_t)(cell_t)1 << (8 * sizeof(cell_t) - 1);
	
}
int main(void){

	int input, x, y;
	int prev_x, prev_y;
	int target_fps = 60;

	TC_enter_screen();
	set_echo_off();
	TC_clear_screen();

	set_raw_input_mode();

	map_t new_game;

	map_choose(&new_game, 1);

	init_player(&new_game, &x, &y);

	do {
		//rendering
		map_draw(new_game);

		//processing input
		input = getchar();

		if(input >= 'a' && input <= 'z'){
			input = input - 32;
		}
		prev_x = x;
		prev_y = y;

		switch (input)
		{
			case LEFT:
				if(y > 0 && !is_wall(new_game.map[x][y-1])){
					y--;
				}
				break;
			case RIGHT:
				if(y < new_game.size-1 && !is_wall(new_game.map[x][y+1])){
					y++;
				}
				break;
			case UP:
				if(x > 0 && !is_wall(new_game.map[x-1][y])){
					x--;
				}
				break;
			case DOWN:
				if(x < new_game.size-1 && !is_wall(new_game.map[x+1][y])){
					x++;
				}
				break;
			default:
				break;
		}

		move_player(&new_game, x, y, prev_x, prev_y);

		//limit the actions per minute
		usleep(1000000 / target_fps);
		
	} while(input != EXIT_SCREEN);
	
	TC_clear_screen();

	set_cooked_input_mode();
	set_echo_on();
	exit_screen();
	
	return 0;
}