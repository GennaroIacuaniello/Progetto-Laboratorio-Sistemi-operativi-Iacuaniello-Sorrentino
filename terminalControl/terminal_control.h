#ifndef TC_H
#define TC_H

#include "color.h"

void print_color(char* msg, color_t fg_color, color_t bg_color);

void echo_off();
void echo_on();
void get_cols_rows(int *cols, int *rows);
void set_cursor(int x, int y);
void clear_terminal();
void enter_screen();
void exit_screen();

void set_raw_input_mode();
void set_cooked_input_mode();

#endif

