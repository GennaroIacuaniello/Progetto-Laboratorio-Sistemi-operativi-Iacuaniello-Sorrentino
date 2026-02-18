#ifndef STM_H
#define STM_H

/*
 *Prints a simple menu, starting_line is the line at which the menu starts printing
 *e.g. if you have a one-line title starting_line = 2
 */
void STM_print_menu(char **menu, int menu_size, int current_position, int starting_line);

/*
 *Prints a simple menu, starting_line is the line at which the menu starts printing
 *e.g. if you have a one-line title starting_line = 2
 */
int STM_navigate_menu(char **menu, int menu_size, int *current_position, int starting_line);

#endif