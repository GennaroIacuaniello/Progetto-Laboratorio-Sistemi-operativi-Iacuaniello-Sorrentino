#include <stdio.h>
#include <unistd.h>

#include "simple_terminal_menu.h"
#include "terminal_control.h"

void STM_print_menu(char **menu, int menu_size, int current_position, int starting_line){

    for(int i = 0; i < menu_size; i++){
        TC_set_cursor(0, starting_line+i);
        printf(" %s\n", menu[i]);
    }
    
    /*Shows current position in the menu*/
    TC_set_cursor(0, current_position + starting_line);
    printf(">");

}

int STM_navigate_menu(char **menu, int menu_size, int *current_position, int starting_line){

    STM_print_menu(menu, menu_size, *current_position, starting_line);

    switch (TC_read_movement_input())
    {
        case MOVE_UP:
            *current_position  <= 0 ? *current_position = 0 : (*current_position)--;
            break;
        case MOVE_DOWN:
            *current_position  >= menu_size-1 ? *current_position = menu_size-1 : (*current_position)++;
            break;
        case ENTER:
            return 1;
            break;
        default:
            break;
    }

    usleep(10000); //keep ~<50000 to avoid stuttering issues with holding arrow keys

    return 0;
}
