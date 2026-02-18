#ifndef TC_H
#define TC_H

#include "color.h"

#define ENTER       13

#define MOVE_UP    65
#define MOVE_DOWN  66
#define MOVE_RIGHT 67
#define MOVE_LEFT  68

void TC_clear_screen(); //clears the terminal

void TC_enter_screen(); //enters a temporary alternative screen
void TC_exit_screen(); //exits the temporary alternative screen

void TC_set_cursor(int x, int y); //positions the cursor to x column, y row. Values grow going to the right and down

void TC_set_echo_off(); //hides characters and cursor while typing 
void TC_set_echo_on(); //shows characters and cursor while typing

void TC_set_raw_input_mode(); //does not wait for ENTER to be pressed to read the stdin
void TC_set_cooked_input_mode(); //cooked input should be normal input mode

void TC_get_cols_rows(int *cols, int *rows);
void TC_color_print(char* msg, color_t fg_color, color_t bg_color);

int TC_read_movement_input(); //reads arrow and WASD, returns MOVE_UP, MOVE_DOWN, MOVE_RIGHT or MOVE_LEFT

#endif

