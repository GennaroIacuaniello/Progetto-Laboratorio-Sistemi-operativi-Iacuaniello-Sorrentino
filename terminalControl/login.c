#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "terminal_control.h"

#define BUFFER_SIZE 64

#define ARROW_UP    65
#define ARROW_DOWN  66
#define ARROW_RIGHT 67
#define ARROW_LEFT  68
#define ENTER       13

#define START_MENU_SIZE 3
#define START_MENU_REGISTER 1
#define START_MENU_LOGIN 2
#define START_MENU_EXIT 3

#define MAIN_MENU_SIZE 3
#define MAIN_MENU_JOIN_LOBBY 1
#define MAIN_MENU_CREATE_LOBBY 1
#define MAIN_MENU_EXIT 1

void print_menu(char **menu, int menu_size);
int navigate_menu(char **menu, int menu_size, int *cursor_y);
int read_input();

void start_menu();
void handle_start_menu_choice(int choice);

void main_menu(char* username);

int main(void){
    
    /*Terminal set-up sequence*/
    enter_screen();
    set_echo_off();
    set_raw_input_mode();

    /*Starting the program*/
    start_menu();

    /*Exit sequence*/
    clear_screen();
    set_cooked_input_mode();
    set_echo_on();

    exit_screen();
    return 0;
}

void print_menu(char **menu, int menu_size){

    clear_screen();
    for(int i = 0; i < menu_size; i++){
        set_cursor(0, i+1);
        printf(" %s\n", menu[i]);
    }

}

int navigate_menu(char **menu, int menu_size, int *cursor_y){

    print_menu(menu, menu_size);
    set_cursor(0, *cursor_y);
    printf(">");

    switch (read_input())
    {
        case ARROW_UP:
            *cursor_y  <= 1 ? *cursor_y = 1 : (*cursor_y)--;
            break;
        case ARROW_DOWN:
            *cursor_y  >= menu_size ? *cursor_y = menu_size : (*cursor_y)++;
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

int read_input(){
    
    char c;
    
    if ((c = getchar()) == '\x1B') {
        getchar(); //skip the [
        return getchar(); //the real arrow value
    }

    return c;
}

void start_menu(){
    int cursor_y = 1;
    int made_choice = 0;
    
    char *start_menu_options[START_MENU_SIZE];
    start_menu_options[START_MENU_REGISTER - 1] = "Registrati";
    start_menu_options[START_MENU_LOGIN - 1] = "Login";
    start_menu_options[START_MENU_EXIT - 1] = "Esci";

    do{
        made_choice = navigate_menu(start_menu_options, START_MENU_SIZE, &cursor_y);
    }while(!made_choice);

    handle_start_menu_choice(cursor_y);

}

//TODO: gestisci la scelta di registrarsi o di fare il login senza riempire lo stack
void handle_start_menu_choice(int choice){
   
    char username[BUFFER_SIZE], password[BUFFER_SIZE];
    int size;

    switch (choice)
    {
        case START_MENU_REGISTER:
            
            break;
        case START_MENU_LOGIN:
            
            int login_success = 0;
            clear_screen();
            set_cooked_input_mode();
            
            printf("Username: "); fflush(stdout);
            set_echo_on();
            size = read(STDIN_FILENO, username, BUFFER_SIZE);
            username[size - 1] = '\0';

            while(!login_success){
                printf("Password: "); fflush(stdout);
                set_echo_off();
                size = read(STDIN_FILENO, password, BUFFER_SIZE);
                password[size - 1] = '\0';
                printf("\n");

                if(!strcmp(username, "grralw") && !strcmp(password, "mypassword")){
                    login_success = 1;
                }
            }
            
            clear_screen();
            set_raw_input_mode();
            main_menu(username);

            break;
        case START_MENU_EXIT:
            break;
        default:
            break;
    }
}

void main_menu(char *username){
    int cursor_y = 1;
    int made_choice = 0;
    
    char *main_menu_options[START_MENU_SIZE];
    main_menu_options[MAIN_MENU_JOIN_LOBBY - 1] = "Unisciti a una lobby";
    main_menu_options[MAIN_MENU_CREATE_LOBBY - 1] = "Crea lobby";
    main_menu_options[MAIN_MENU_EXIT - 1] = "Esci";
    
    do{
        made_choice = navigate_menu(main_menu_options, MAIN_MENU_SIZE, &cursor_y);
    }while(!made_choice);

}
