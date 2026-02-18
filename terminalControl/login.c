#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "terminal_control.h"
#include "simple_terminal_menu.h"

#define BUFFER_SIZE 64

#define START_MENU_SIZE 3
#define START_MENU_LOGIN 0
#define START_MENU_REGISTER 1
#define START_MENU_EXIT 2

#define MAIN_MENU_SIZE 3
#define MAIN_MENU_JOIN_LOBBY 0
#define MAIN_MENU_CREATE_LOBBY 1
#define MAIN_MENU_EXIT 2

void start_menu();
void handle_start_menu_choice(int choice);

void main_menu(char* username);

int main(void){
    
    /*Terminal set-up sequence*/
    TC_enter_screen();
    TC_set_echo_off();
    TC_set_raw_input_mode();

    /*Starting the program*/
    start_menu();

    /*Exit sequence*/
    TC_clear_screen();
    TC_set_cooked_input_mode();
    TC_set_echo_on();

    TC_exit_screen();
    return 0;
}

void start_menu(){
    int starting_line = 1;
    int current_menu_position = 0;
    int made_choice = 0;
    
    char *start_menu_options[START_MENU_SIZE];
    start_menu_options[START_MENU_LOGIN] = "Login";
    start_menu_options[START_MENU_REGISTER] = "Registrati";
    start_menu_options[START_MENU_EXIT] = "Esci";

    do{
        TC_clear_screen();
        made_choice = STM_navigate_menu(start_menu_options, START_MENU_SIZE, &current_menu_position, starting_line);
    }while(!made_choice);

    handle_start_menu_choice(current_menu_position);

}

void handle_start_menu_choice(int choice){
   
    char username[BUFFER_SIZE], password[BUFFER_SIZE];
    int size;

    switch (choice)
    {
        case START_MENU_LOGIN:
            
            int login_success = 0;
            TC_clear_screen();
            TC_set_cooked_input_mode();
            
            printf("Username: "); fflush(stdout);
            TC_set_echo_on();
            size = read(STDIN_FILENO, username, BUFFER_SIZE);
            username[size - 1] = '\0';

            while(!login_success){
                printf("Password: "); fflush(stdout);
                TC_set_echo_off(); //Hide the password
                size = read(STDIN_FILENO, password, BUFFER_SIZE);
                password[size - 1] = '\0';
                printf("\n");

                if(!strcmp(username, "a") && !strcmp(password, "a")){
                    login_success = 1;
                }
            }
            
            TC_clear_screen();
            TC_set_raw_input_mode();
            main_menu(username);

            break;
        case START_MENU_REGISTER:
            printf("SELECTED REGISTER\n");
            usleep(500000);
            break;
        case START_MENU_EXIT:
            printf("SELECTED EXIT\n");
            usleep(500000);
            break;
        default:
            break;
    }
}

void main_menu(char *username){
    int starting_line = 1;
    int current_menu_position = 0;
    int made_choice = 0;
    
    char *main_menu_options[MAIN_MENU_SIZE];
    main_menu_options[MAIN_MENU_JOIN_LOBBY] = "Unisciti a una lobby";
    main_menu_options[MAIN_MENU_CREATE_LOBBY] = "Crea lobby";
    main_menu_options[MAIN_MENU_EXIT] = "Esci";
    
    do{
        TC_clear_screen();
        made_choice = STM_navigate_menu(main_menu_options, MAIN_MENU_SIZE, &current_menu_position, starting_line);
    }while(!made_choice);

    switch (current_menu_position)
    {
        case MAIN_MENU_JOIN_LOBBY:
            printf("SELECTED JOIN LOBBBY\n"); 
            usleep(500000);
            break;
        case MAIN_MENU_CREATE_LOBBY:
            printf("SELECTED CREATE LOBBBY\n"); 
            usleep(500000);
            break;
        case MAIN_MENU_EXIT:
            printf("SELECTED EXIT\n"); 
            usleep(500000);
    
    default:
        break;
    }

}
